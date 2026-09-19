#include <ctex/exec/execution_control.hpp>

namespace ctex::exec {

std::string_view execution_status_name(ExecutionStatus status) noexcept {
    switch (status) {
        case ExecutionStatus::completed:
            return "completed";
        case ExecutionStatus::cancelled:
            return "cancelled";
        case ExecutionStatus::memory_ceiling_exceeded:
            return "memory-ceiling-exceeded";
    }
    return "unknown";
}

}  // namespace ctex::exec
