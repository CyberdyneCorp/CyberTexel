#ifndef CTEX_EXEC_EXECUTOR_HPP
#define CTEX_EXEC_EXECUTOR_HPP

#include <cstdint>
#include <memory>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace ctex::exec {

inline constexpr std::string_view executor_environment_variable = "CTEX_EXECUTOR";

enum class ExecutorRoute : std::uint8_t { host_executed, cpu_reference, owned_gpu };
enum class ExecutorAvailability : std::uint8_t {
    available,
    device_unavailable,
    host_not_attached,
};

struct ExecutorDescriptor {
    std::string identifier;
    std::string display_name;
    std::string device_name;
    ExecutorRoute route{};
    ExecutorAvailability availability{};
    friend bool operator==(const ExecutorDescriptor&, const ExecutorDescriptor&) = default;
};

class Executor {
public:
    virtual ~Executor() = default;
    [[nodiscard]] virtual const ExecutorDescriptor& descriptor() const noexcept = 0;
};

enum class ExecutorSelectionSource : std::uint8_t { automatic, explicit_request, environment };

struct ExecutorSelection {
    std::shared_ptr<const Executor> executor;
    ExecutorSelectionSource source{};
    std::string requested_identifier;
    std::string message;
};

enum class ExecutionFailureCode : std::uint8_t {
    device_unavailable,
    device_lost,
    operation_failed,
    cancelled,
};

enum class FallbackDisposition : std::uint8_t {
    no_fallback,
    cpu_fallback,
    recovery_required,
};

struct ExecutorFallbackReport {
    std::string failed_executor;
    ExecutionFailureCode failure{};
    std::string failure_detail;
    FallbackDisposition disposition{};
    std::string fallback_executor;
    bool recovery_restored{};
    std::string message;
    friend bool operator==(const ExecutorFallbackReport&, const ExecutorFallbackReport&) = default;
};

class ExecutorRegistryError final : public std::invalid_argument {
public:
    using std::invalid_argument::invalid_argument;
};

class ExecutorRegistry {
public:
    void add(std::shared_ptr<const Executor> executor);

    [[nodiscard]] std::vector<ExecutorDescriptor> enumerate() const;
    [[nodiscard]] std::vector<ExecutorDescriptor> available() const;
    [[nodiscard]] const Executor* find(std::string_view identifier) const noexcept;

    [[nodiscard]] ExecutorSelection select_automatic() const;
    [[nodiscard]] ExecutorSelection select(std::string_view identifier) const;
    [[nodiscard]] ExecutorSelection select_from_environment(
        std::optional<std::string_view> value) const;
    [[nodiscard]] ExecutorSelection select_process_default() const;

    void pin_default(std::string_view identifier);
    void clear_pinned_default() noexcept;
    [[nodiscard]] std::optional<std::string_view> pinned_default() const noexcept;

private:
    [[nodiscard]] std::shared_ptr<const Executor> require(std::string_view identifier) const;

    std::vector<std::shared_ptr<const Executor>> executors_;
    std::optional<std::string> pinned_default_;
};

[[nodiscard]] std::string_view executor_route_name(ExecutorRoute route) noexcept;
[[nodiscard]] std::string_view executor_availability_name(
    ExecutorAvailability availability) noexcept;
[[nodiscard]] ExecutorFallbackReport make_fallback_report(std::string failed_executor,
                                                          ExecutionFailureCode failure,
                                                          std::string failure_detail,
                                                          FallbackDisposition disposition,
                                                          std::string fallback_executor = {},
                                                          bool recovery_restored = false);

}  // namespace ctex::exec

#endif
