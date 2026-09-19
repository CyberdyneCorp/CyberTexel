#include <algorithm>
#include <atomic>
#include <cstddef>
#include <ctex/exec/cpu_reference.hpp>
#include <iostream>
#include <limits>
#include <string_view>
#include <vector>

namespace {

using ctex::exec::CpuBoundedOperation;
using ctex::exec::CpuReferenceExecutor;
using ctex::exec::CpuWorkPlan;
using ctex::exec::ExecutionControl;
using ctex::exec::ExecutionOutcome;
using ctex::exec::ExecutionProgress;
using ctex::exec::ExecutionStatus;

bool expect(bool condition, std::string_view message) {
    if (!condition) {
        std::cerr << message << '\n';
    }
    return condition;
}

class StagedByteOperation final : public CpuBoundedOperation {
public:
    StagedByteOperation(std::vector<std::byte>& document, CpuWorkPlan plan)
        : document_(document), plan_(plan) {}

    [[nodiscard]] std::string_view identifier() const noexcept override { return "staged-fill"; }
    [[nodiscard]] CpuWorkPlan plan() const override { return plan_; }

    void execute_work_item(const CpuReferenceExecutor& executor, std::size_t work_item,
                           std::span<std::byte> shared,
                           std::span<std::byte> worker_memory) override {
        saw_cpu_.store(executor.descriptor().identifier == "cpu", std::memory_order_relaxed);
        ++executed_;
        if (!worker_memory.empty()) {
            worker_memory.front() = std::byte{0x5a};
        }
        if (work_item < shared.size()) {
            shared[work_item] = std::byte{static_cast<unsigned char>((work_item % 251) + 1)};
        }
    }

    void commit(std::span<const std::byte> shared) noexcept override {
        std::copy_n(shared.begin(), document_.size(), document_.begin());
        ++commits_;
    }

    [[nodiscard]] std::size_t executed() const noexcept { return executed_.load(); }
    [[nodiscard]] std::size_t commits() const noexcept { return commits_.load(); }
    [[nodiscard]] bool saw_cpu() const noexcept { return saw_cpu_.load(); }

private:
    std::vector<std::byte>& document_;
    CpuWorkPlan plan_;
    std::atomic_size_t executed_{};
    std::atomic_size_t commits_{};
    std::atomic_bool saw_cpu_{};
};

struct ProgressState {
    std::atomic_size_t latest{};
    std::size_t cancel_after{std::numeric_limits<std::size_t>::max()};
    std::vector<ExecutionProgress> reports;
};

void record_progress(void* user_data, ExecutionProgress progress) noexcept {
    auto& state = *static_cast<ProgressState*>(user_data);
    state.latest.store(progress.completed_work_items, std::memory_order_relaxed);
    state.reports.push_back(progress);
}

bool cancel_at_threshold(void* user_data) noexcept {
    const auto& state = *static_cast<ProgressState*>(user_data);
    return state.latest.load(std::memory_order_relaxed) >= state.cancel_after;
}

bool worker_bounds_are_real_and_results_are_reproducible() {
    std::vector<std::byte> serial_document(64, std::byte{0});
    std::vector<std::byte> parallel_document(64, std::byte{0});
    StagedByteOperation serial(serial_document, {64, 64, 8});
    StagedByteOperation parallel(parallel_document, {64, 64, 8});
    ProgressState progress;
    const CpuReferenceExecutor executor;
    const ExecutionOutcome serial_result = executor.execute_bounded(
        serial, {.maximum_workers = 1, .memory_ceiling_bytes = 72, .progress_interval = 16});
    const ExecutionOutcome parallel_result =
        executor.execute_bounded(parallel, {.maximum_workers = 4,
                                            .memory_ceiling_bytes = 96,
                                            .progress_interval = 10,
                                            .user_data = &progress,
                                            .report_progress = record_progress});
    const bool monotonic =
        std::ranges::is_sorted(progress.reports, {}, &ExecutionProgress::completed_work_items);
    return expect(serial_result.status == ExecutionStatus::completed &&
                      parallel_result.status == ExecutionStatus::completed,
                  "bounded CPU executions did not complete") &&
           expect(serial_result.worker_count == 1 && parallel_result.worker_count == 4,
                  "CPU executor did not respect the requested worker bounds") &&
           expect(serial_result.required_memory_bytes == 72 &&
                      parallel_result.required_memory_bytes == 96,
                  "CPU working-memory accounting did not include per-worker storage") &&
           expect(serial_document == parallel_document && serial.commits() == 1 &&
                      parallel.commits() == 1 && serial.saw_cpu() && parallel.saw_cpu(),
                  "single-worker and multi-worker results differed") &&
           expect(monotonic && progress.reports.front().completed_work_items == 0 &&
                      progress.reports.back().completed_work_items == 64 &&
                      progress.reports.back().total_work_items == 64,
                  "bounded CPU progress was incomplete or non-monotonic");
}

bool cancellation_discards_all_staged_output() {
    std::vector<std::byte> document(4096, std::byte{0x7f});
    const std::vector<std::byte> before = document;
    StagedByteOperation operation(document, {4096, 4096, 16});
    ProgressState progress{.latest = {}, .cancel_after = 32, .reports = {}};
    const ExecutionOutcome result =
        CpuReferenceExecutor{}.execute_bounded(operation, {.maximum_workers = 4,
                                                           .memory_ceiling_bytes = 4160,
                                                           .progress_interval = 8,
                                                           .user_data = &progress,
                                                           .is_cancelled = cancel_at_threshold,
                                                           .report_progress = record_progress});
    return expect(result.status == ExecutionStatus::cancelled,
                  "cancelled CPU work reported completion") &&
           expect(result.completed_work_items >= 32 && result.completed_work_items < 4096 &&
                      operation.executed() == result.completed_work_items,
                  "CPU cancellation was not observed at a work-item boundary") &&
           expect(operation.commits() == 0 && document == before,
                  "cancelled CPU work exposed staged document changes") &&
           expect(progress.reports.back().completed_work_items == result.completed_work_items,
                  "cancelled CPU work did not report its final processed count");
}

bool existing_cancellation_starts_no_workers() {
    std::vector<std::byte> document(64, std::byte{0x3c});
    const std::vector<std::byte> before = document;
    StagedByteOperation operation(document, {64, 64, 8});
    ProgressState progress{.latest = {}, .cancel_after = 0, .reports = {}};
    const ExecutionOutcome result =
        CpuReferenceExecutor{}.execute_bounded(operation, {.maximum_workers = 4,
                                                           .memory_ceiling_bytes = 96,
                                                           .progress_interval = 8,
                                                           .user_data = &progress,
                                                           .is_cancelled = cancel_at_threshold,
                                                           .report_progress = record_progress});
    return expect(result.status == ExecutionStatus::cancelled && result.worker_count == 0 &&
                      result.completed_work_items == 0,
                  "an existing cancellation started CPU workers") &&
           expect(operation.executed() == 0 && operation.commits() == 0 && document == before,
                  "an existing cancellation performed or committed work") &&
           expect(
               progress.reports.size() == 1 && progress.reports.front() == ExecutionProgress{0, 64},
               "an existing cancellation did not report initial zero progress");
}

bool memory_is_refused_before_work_and_names_both_limits() {
    std::vector<std::byte> document(100, std::byte{0});
    StagedByteOperation refused(document, {100, 100, 8});
    const ExecutionOutcome refusal = CpuReferenceExecutor{}.execute_bounded(
        refused, {.maximum_workers = 4, .memory_ceiling_bytes = 131});
    const bool diagnostic = refusal.message.find("132") != std::string::npos &&
                            refusal.message.find("131") != std::string::npos &&
                            refusal.message.find("staged-fill") != std::string::npos;

    StagedByteOperation admitted(document, {100, 100, 8});
    const ExecutionOutcome exact = CpuReferenceExecutor{}.execute_bounded(
        admitted, {.maximum_workers = 4, .memory_ceiling_bytes = 132});
    return expect(refusal.status == ExecutionStatus::memory_ceiling_exceeded &&
                      refusal.required_memory_bytes == 132 && refusal.worker_count == 0 &&
                      refused.executed() == 0 && refused.commits() == 0,
                  "over-budget CPU work was not refused before execution") &&
           expect(diagnostic, "memory refusal did not name the operation, request, and ceiling") &&
           expect(exact.status == ExecutionStatus::completed && exact.required_memory_bytes == 132,
                  "an exact CPU memory ceiling did not admit the operation");
}

bool invalid_and_overflowing_limits_are_refused() {
    std::vector<std::byte> document(1, std::byte{0});
    StagedByteOperation operation(document, {2, std::numeric_limits<std::size_t>::max(), 1});
    const ExecutionOutcome overflow =
        CpuReferenceExecutor{}.execute_bounded(operation, {.maximum_workers = 2});
    bool zero_workers_refused = false;
    try {
        static_cast<void>(CpuReferenceExecutor{}.execute_bounded(
            operation, {.maximum_workers = 0, .progress_interval = 1}));
    } catch (const std::invalid_argument&) {
        zero_workers_refused = true;
    }
    return expect(overflow.status == ExecutionStatus::memory_ceiling_exceeded &&
                      operation.executed() == 0,
                  "overflowing CPU working storage was not refused") &&
           expect(zero_workers_refused, "a zero CPU worker bound was accepted");
}

bool zero_storage_work_is_supported() {
    std::vector<std::byte> document;
    StagedByteOperation operation(document, {8, 0, 0});
    const ExecutionOutcome result =
        CpuReferenceExecutor{}.execute_bounded(operation, {.maximum_workers = 2});
    return expect(result.status == ExecutionStatus::completed &&
                      result.required_memory_bytes == 0 && result.worker_count == 2 &&
                      result.completed_work_items == 8 && operation.executed() == 8 &&
                      operation.commits() == 1,
                  "zero-storage CPU work did not complete safely");
}

}  // namespace

int main() {
    return worker_bounds_are_real_and_results_are_reproducible() &&
                   cancellation_discards_all_staged_output() &&
                   existing_cancellation_starts_no_workers() &&
                   memory_is_refused_before_work_and_names_both_limits() &&
                   invalid_and_overflowing_limits_are_refused() && zero_storage_work_is_supported()
               ? 0
               : 1;
}
