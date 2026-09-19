#include <algorithm>
#include <atomic>
#include <ctex/exec/cpu_reference.hpp>
#include <exception>
#include <limits>
#include <mutex>
#include <stdexcept>
#include <thread>
#include <utility>
#include <vector>

namespace ctex::exec {
namespace {

struct MemoryRequirement {
    std::size_t bytes{};
    bool overflowed{};
};

MemoryRequirement required_memory(const CpuWorkPlan& plan, std::size_t worker_count) {
    constexpr std::size_t maximum = std::numeric_limits<std::size_t>::max();
    if (worker_count != 0 && plan.working_memory_bytes_per_worker > maximum / worker_count) {
        return {maximum, true};
    }
    const std::size_t worker_bytes = worker_count * plan.working_memory_bytes_per_worker;
    if (worker_bytes > maximum - plan.shared_working_memory_bytes) {
        return {maximum, true};
    }
    return {plan.shared_working_memory_bytes + worker_bytes, false};
}

std::string memory_refusal(std::string_view operation, std::size_t required, std::size_t ceiling) {
    return "CPU operation '" + std::string(operation) + "' requires " + std::to_string(required) +
           " working bytes, exceeding memory ceiling " + std::to_string(ceiling) + " bytes";
}

class CallbackState {
public:
    explicit CallbackState(const ExecutionControl& control) : control_(control) {}

    [[nodiscard]] bool cancelled() {
        std::lock_guard lock(mutex_);
        return control_.is_cancelled != nullptr && control_.is_cancelled(control_.user_data);
    }

    void begin(std::size_t total) {
        std::lock_guard lock(mutex_);
        if (control_.report_progress != nullptr) {
            control_.report_progress(control_.user_data, {0, total});
        }
    }

    void work_item_completed(std::size_t total) {
        std::lock_guard lock(mutex_);
        ++completed_;
        if (completed_ % control_.progress_interval == 0 || completed_ == total) {
            report_locked(total);
        }
    }

    void finish(std::size_t total) {
        std::lock_guard lock(mutex_);
        if (last_reported_ != completed_) {
            report_locked(total);
        }
    }

    [[nodiscard]] std::size_t completed() const {
        std::lock_guard lock(mutex_);
        return completed_;
    }

private:
    void report_locked(std::size_t total) {
        if (control_.report_progress != nullptr) {
            control_.report_progress(control_.user_data, {completed_, total});
        }
        last_reported_ = completed_;
    }

    const ExecutionControl& control_;
    mutable std::mutex mutex_;
    std::size_t completed_{};
    std::size_t last_reported_{};
};

}  // namespace

ExecutionOutcome CpuReferenceExecutor::execute_bounded(CpuBoundedOperation& operation,
                                                       ExecutionControl control) const {
    const std::string identifier(operation.identifier());
    if (identifier.empty()) {
        throw std::invalid_argument("bounded CPU operation identifier must not be empty");
    }
    if (control.maximum_workers == 0 || control.progress_interval == 0) {
        throw std::invalid_argument(
            "bounded CPU execution requires non-zero worker and progress bounds");
    }

    const CpuWorkPlan plan = operation.plan();
    const std::size_t worker_count = std::min(control.maximum_workers, plan.work_item_count);
    const MemoryRequirement memory = required_memory(plan, worker_count);
    if (memory.overflowed || memory.bytes > control.memory_ceiling_bytes) {
        return {.operation = identifier,
                .status = ExecutionStatus::memory_ceiling_exceeded,
                .completed_work_items = 0,
                .total_work_items = plan.work_item_count,
                .required_memory_bytes = memory.bytes,
                .worker_count = 0,
                .message = memory_refusal(identifier, memory.bytes, control.memory_ceiling_bytes)};
    }

    CallbackState callbacks(control);
    callbacks.begin(plan.work_item_count);
    if (callbacks.cancelled()) {
        return {.operation = identifier,
                .status = ExecutionStatus::cancelled,
                .completed_work_items = 0,
                .total_work_items = plan.work_item_count,
                .required_memory_bytes = memory.bytes,
                .worker_count = 0,
                .message = "CPU operation '" + identifier + "' cancelled before work began"};
    }

    std::vector<std::byte> working_memory(memory.bytes);
    std::span<std::byte> shared(working_memory.data(), plan.shared_working_memory_bytes);
    std::atomic_size_t next_work_item{};
    std::atomic_bool stop{};
    std::atomic_bool cancellation_observed{};
    std::mutex failure_mutex;
    std::exception_ptr failure;

    const auto run_worker = [&](std::size_t worker) {
        const std::size_t scratch_offset =
            plan.shared_working_memory_bytes + worker * plan.working_memory_bytes_per_worker;
        std::span<std::byte> scratch;
        if (plan.working_memory_bytes_per_worker != 0) {
            scratch = {working_memory.data() + scratch_offset,
                       plan.working_memory_bytes_per_worker};
        }
        while (!stop.load(std::memory_order_relaxed)) {
            if (callbacks.cancelled()) {
                cancellation_observed.store(true, std::memory_order_relaxed);
                stop.store(true, std::memory_order_relaxed);
                return;
            }
            const std::size_t work_item = next_work_item.fetch_add(1, std::memory_order_relaxed);
            if (work_item >= plan.work_item_count) {
                return;
            }
            try {
                operation.execute_work_item(*this, work_item, shared, scratch);
            } catch (...) {
                std::lock_guard lock(failure_mutex);
                if (failure == nullptr) {
                    failure = std::current_exception();
                }
                stop.store(true, std::memory_order_relaxed);
                return;
            }
            callbacks.work_item_completed(plan.work_item_count);
        }
    };

    {
        std::vector<std::jthread> workers;
        workers.reserve(worker_count);
        for (std::size_t worker = 0; worker < worker_count; ++worker) {
            workers.emplace_back(run_worker, worker);
        }
    }
    if (failure != nullptr) {
        std::rethrow_exception(failure);
    }

    const std::size_t completed = callbacks.completed();
    const bool cancelled =
        cancellation_observed.load(std::memory_order_relaxed) || callbacks.cancelled();
    if (cancelled) {
        callbacks.finish(plan.work_item_count);
        return {.operation = identifier,
                .status = ExecutionStatus::cancelled,
                .completed_work_items = completed,
                .total_work_items = plan.work_item_count,
                .required_memory_bytes = memory.bytes,
                .worker_count = worker_count,
                .message = "CPU operation '" + identifier + "' cancelled after " +
                           std::to_string(completed) + " of " +
                           std::to_string(plan.work_item_count) + " work items"};
    }

    operation.commit(shared);
    return {.operation = identifier,
            .status = ExecutionStatus::completed,
            .completed_work_items = completed,
            .total_work_items = plan.work_item_count,
            .required_memory_bytes = memory.bytes,
            .worker_count = worker_count,
            .message = "CPU operation '" + identifier + "' completed"};
}

}  // namespace ctex::exec
