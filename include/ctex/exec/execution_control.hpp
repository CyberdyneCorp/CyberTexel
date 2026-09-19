#ifndef CTEX_EXEC_EXECUTION_CONTROL_HPP
#define CTEX_EXEC_EXECUTION_CONTROL_HPP

#include <cstddef>
#include <cstdint>
#include <limits>
#include <string>
#include <string_view>

namespace ctex::exec {

struct ExecutionProgress {
    std::size_t completed_work_items{};
    std::size_t total_work_items{};
    friend constexpr bool operator==(ExecutionProgress, ExecutionProgress) noexcept = default;
};

using ExecutionCancellationCallback = bool (*)(void* user_data) noexcept;
using ExecutionProgressCallback = void (*)(void* user_data, ExecutionProgress progress) noexcept;

struct ExecutionControl {
    std::size_t maximum_workers{1};
    std::size_t memory_ceiling_bytes{std::numeric_limits<std::size_t>::max()};
    std::size_t progress_interval{256};
    void* user_data{};
    ExecutionCancellationCallback is_cancelled{};
    ExecutionProgressCallback report_progress{};
};

enum class ExecutionStatus : std::uint8_t { completed, cancelled, memory_ceiling_exceeded };

struct ExecutionOutcome {
    std::string operation;
    ExecutionStatus status{};
    std::size_t completed_work_items{};
    std::size_t total_work_items{};
    std::size_t required_memory_bytes{};
    std::size_t worker_count{};
    std::string message;
};

[[nodiscard]] std::string_view execution_status_name(ExecutionStatus status) noexcept;

}  // namespace ctex::exec

#endif
