#include <algorithm>
#include <cstdlib>
#include <ctex/exec/executor.hpp>
#include <optional>
#include <string>
#include <tuple>
#include <utility>

namespace ctex::exec {
namespace {

bool is_available(const ExecutorDescriptor& descriptor) {
    return descriptor.availability == ExecutorAvailability::available;
}

std::optional<std::string> process_environment_value(const char* name) {
#if defined(_WIN32)
    char* value{};
    std::size_t size{};
    if (_dupenv_s(&value, &size, name) != 0 || value == nullptr) {
        return std::nullopt;
    }
    std::string result(value);
    std::free(value);
    return result;
#else
    const char* value = std::getenv(name);
    return value == nullptr ? std::nullopt : std::optional<std::string>{value};
#endif
}

int automatic_rank(ExecutorRoute route) {
    switch (route) {
        case ExecutorRoute::owned_gpu:
            return 0;
        case ExecutorRoute::cpu_reference:
            return 1;
        case ExecutorRoute::host_executed:
            return 2;
    }
    return 3;
}

void validate_descriptor(const ExecutorDescriptor& descriptor) {
    if (descriptor.identifier.empty() || descriptor.display_name.empty() ||
        descriptor.device_name.empty()) {
        throw ExecutorRegistryError(
            "executor requires an identifier, display name, and device name");
    }
    if (descriptor.features.binding_budget < 2 ||
        descriptor.features.maximum_texture_dimension == 0 ||
        descriptor.features.supported_texture_formats.empty()) {
        throw ExecutorRegistryError("executor requires valid shader-emission features: " +
                                    descriptor.identifier);
    }
}

ExecutorSelection selected(std::shared_ptr<const Executor> executor, ExecutorSelectionSource source,
                           std::string requested, std::string message) {
    return {std::move(executor), source, std::move(requested), std::move(message)};
}

}  // namespace

void ExecutorRegistry::add(std::shared_ptr<const Executor> executor) {
    if (executor == nullptr) {
        throw ExecutorRegistryError("cannot register a null executor");
    }
    validate_descriptor(executor->descriptor());
    const std::string& identifier = executor->descriptor().identifier;
    const auto position = std::lower_bound(executors_.begin(), executors_.end(), identifier,
                                           [](const auto& candidate, std::string_view sought) {
                                               return candidate->descriptor().identifier < sought;
                                           });
    if (position != executors_.end() && (*position)->descriptor().identifier == identifier) {
        throw ExecutorRegistryError("executor identifier is already registered: " + identifier);
    }
    executors_.insert(position, std::move(executor));
}

std::vector<ExecutorDescriptor> ExecutorRegistry::enumerate() const {
    std::vector<ExecutorDescriptor> result;
    result.reserve(executors_.size());
    for (const auto& executor : executors_) {
        result.push_back(executor->descriptor());
    }
    return result;
}

std::vector<ExecutorDescriptor> ExecutorRegistry::available() const {
    std::vector<ExecutorDescriptor> result;
    for (const auto& executor : executors_) {
        if (is_available(executor->descriptor())) {
            result.push_back(executor->descriptor());
        }
    }
    return result;
}

const Executor* ExecutorRegistry::find(std::string_view identifier) const noexcept {
    const auto position = std::lower_bound(executors_.begin(), executors_.end(), identifier,
                                           [](const auto& candidate, std::string_view sought) {
                                               return candidate->descriptor().identifier < sought;
                                           });
    return position == executors_.end() || (*position)->descriptor().identifier != identifier
               ? nullptr
               : position->get();
}

std::shared_ptr<const Executor> ExecutorRegistry::require(std::string_view identifier) const {
    const auto position = std::lower_bound(executors_.begin(), executors_.end(), identifier,
                                           [](const auto& candidate, std::string_view sought) {
                                               return candidate->descriptor().identifier < sought;
                                           });
    if (position == executors_.end() || (*position)->descriptor().identifier != identifier) {
        throw ExecutorRegistryError("executor is not compiled in: " + std::string(identifier));
    }
    return *position;
}

ExecutorSelection ExecutorRegistry::select_automatic() const {
    if (pinned_default_) {
        return selected(require(*pinned_default_), ExecutorSelectionSource::explicit_request,
                        *pinned_default_, "using the pinned process default");
    }
    const auto position = std::min_element(
        executors_.begin(), executors_.end(), [](const auto& left, const auto& right) {
            const ExecutorDescriptor& lhs = left->descriptor();
            const ExecutorDescriptor& rhs = right->descriptor();
            const auto left_key =
                std::tuple{!is_available(lhs), automatic_rank(lhs.route), lhs.identifier};
            const auto right_key =
                std::tuple{!is_available(rhs), automatic_rank(rhs.route), rhs.identifier};
            return left_key < right_key;
        });
    if (position == executors_.end() || !is_available((*position)->descriptor())) {
        throw ExecutorRegistryError("no executor is currently available");
    }
    return selected(*position, ExecutorSelectionSource::automatic, {},
                    "selected the highest-priority available executor");
}

ExecutorSelection ExecutorRegistry::select(std::string_view identifier) const {
    return selected(require(identifier), ExecutorSelectionSource::explicit_request,
                    std::string(identifier), "selected the requested compiled-in executor");
}

ExecutorSelection ExecutorRegistry::select_from_environment(
    std::optional<std::string_view> value) const {
    if (!value || value->empty()) {
        return select_automatic();
    }
    if (find(*value) != nullptr) {
        return selected(require(*value), ExecutorSelectionSource::environment, std::string(*value),
                        "selected by " + std::string(executor_environment_variable));
    }
    ExecutorSelection automatic = select_automatic();
    automatic.requested_identifier = *value;
    automatic.message = std::string(executor_environment_variable) +
                        " requested unknown executor '" + std::string(*value) +
                        "'; automatic selection remains in place";
    return automatic;
}

ExecutorSelection ExecutorRegistry::select_process_default() const {
    const std::optional<std::string> value =
        process_environment_value(executor_environment_variable.data());
    return select_from_environment(value ? std::optional<std::string_view>{*value} : std::nullopt);
}

void ExecutorRegistry::pin_default(std::string_view identifier) {
    static_cast<void>(require(identifier));
    pinned_default_ = identifier;
}

void ExecutorRegistry::clear_pinned_default() noexcept { pinned_default_.reset(); }

std::optional<std::string_view> ExecutorRegistry::pinned_default() const noexcept {
    if (!pinned_default_) {
        return std::nullopt;
    }
    return *pinned_default_;
}

std::string_view executor_route_name(ExecutorRoute route) noexcept {
    switch (route) {
        case ExecutorRoute::host_executed:
            return "host-executed";
        case ExecutorRoute::cpu_reference:
            return "cpu-reference";
        case ExecutorRoute::owned_gpu:
            return "owned-gpu";
    }
    return "unknown";
}

std::string_view executor_availability_name(ExecutorAvailability availability) noexcept {
    switch (availability) {
        case ExecutorAvailability::available:
            return "available";
        case ExecutorAvailability::device_unavailable:
            return "device-unavailable";
        case ExecutorAvailability::host_not_attached:
            return "host-not-attached";
    }
    return "unknown";
}

ExecutorFallbackReport make_fallback_report(std::string failed_executor,
                                            ExecutionFailureCode failure,
                                            std::string failure_detail,
                                            FallbackDisposition disposition,
                                            std::string fallback_executor, bool recovery_restored) {
    if (failed_executor.empty() || failure_detail.empty()) {
        throw ExecutorRegistryError("fallback report requires a failed executor and detail");
    }
    if (disposition == FallbackDisposition::cpu_fallback &&
        (fallback_executor.empty() || !recovery_restored)) {
        throw ExecutorRegistryError(
            "CPU fallback requires a named executor after recovery was restored");
    }
    if (disposition != FallbackDisposition::cpu_fallback && !fallback_executor.empty()) {
        throw ExecutorRegistryError("fallback executor is present when no fallback is scheduled");
    }
    std::string message = "executor '" + failed_executor + "' failed: " + failure_detail;
    if (disposition == FallbackDisposition::cpu_fallback) {
        message.append("; restored recovery state and fell back to '");
        message.append(fallback_executor);
        message.push_back('\'');
    } else if (disposition == FallbackDisposition::recovery_required) {
        message.append("; recovery is required before execution can continue");
    } else {
        message.append("; no fallback was attempted");
    }
    return {.failed_executor = std::move(failed_executor),
            .failure = failure,
            .failure_detail = std::move(failure_detail),
            .disposition = disposition,
            .fallback_executor = std::move(fallback_executor),
            .recovery_restored = recovery_restored,
            .message = std::move(message)};
}

}  // namespace ctex::exec
