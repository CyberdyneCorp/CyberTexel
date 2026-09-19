#include <ctex/exec/executor.hpp>
#include <iostream>
#include <memory>
#include <string_view>
#include <utility>

namespace {

using ctex::exec::ExecutorAvailability;
using ctex::exec::ExecutorDescriptor;
using ctex::exec::ExecutorRoute;

ctex::emit::DeviceFeatureSet features() {
    return {.binding_budget = 8,
            .maximum_texture_dimension = 4096,
            .supported_texture_formats = {ctex::emit::TextureFormat::rgba8_unorm},
            .floating_point_filtering = true,
            .compute_available = false};
}

class FixtureExecutor final : public ctex::exec::Executor {
public:
    explicit FixtureExecutor(ExecutorDescriptor descriptor) : descriptor_(std::move(descriptor)) {}

    [[nodiscard]] const ExecutorDescriptor& descriptor() const noexcept override {
        return descriptor_;
    }

private:
    ExecutorDescriptor descriptor_;
};

std::shared_ptr<const ctex::exec::Executor> executor(
    std::string identifier, ExecutorRoute route,
    ExecutorAvailability availability = ExecutorAvailability::available) {
    return std::make_shared<FixtureExecutor>(ExecutorDescriptor{
        .identifier = identifier,
        .display_name = identifier + " display",
        .device_name = identifier + " device",
        .route = route,
        .availability = availability,
        .features = features(),
    });
}

bool expect(bool condition, std::string_view message) {
    if (!condition) {
        std::cerr << message << '\n';
    }
    return condition;
}

ctex::exec::ExecutorRegistry registry() {
    ctex::exec::ExecutorRegistry result;
    result.add(
        executor("host", ExecutorRoute::host_executed, ExecutorAvailability::host_not_attached));
    result.add(executor("cpu", ExecutorRoute::cpu_reference));
    result.add(executor("vulkan", ExecutorRoute::owned_gpu));
    result.add(
        executor("metal", ExecutorRoute::owned_gpu, ExecutorAvailability::device_unavailable));
    return result;
}

bool enumeration_is_stable_and_availability_is_explicit() {
    const auto fixture = registry();
    const auto all = fixture.enumerate();
    const auto available = fixture.available();
    return expect(all.size() == 4 && all[0].identifier == "cpu" && all[1].identifier == "host" &&
                      all[2].identifier == "metal" && all[3].identifier == "vulkan",
                  "executor enumeration was not stable by identifier") &&
           expect(available.size() == 2 && available[0].identifier == "cpu" &&
                      available[1].identifier == "vulkan" &&
                      all[1].availability == ExecutorAvailability::host_not_attached &&
                      all[2].availability == ExecutorAvailability::device_unavailable,
                  "runtime-unavailable executors were hidden or counted as available") &&
           expect(ctex::exec::executor_route_name(all[0].route) == "cpu-reference" &&
                      ctex::exec::executor_availability_name(all[2].availability) ==
                          "device-unavailable",
                  "executor route or availability names changed");
}

bool automatic_explicit_and_pinned_selection_are_distinct() {
    auto fixture = registry();
    const auto automatic = fixture.select_automatic();
    const auto explicit_cpu = fixture.select("cpu");
    const auto explicit_unavailable = fixture.select("metal");
    fixture.pin_default("cpu");
    const auto pinned = fixture.select_automatic();
    fixture.clear_pinned_default();
    return expect(automatic.executor->descriptor().identifier == "vulkan" &&
                      automatic.source == ctex::exec::ExecutorSelectionSource::automatic,
                  "automatic selection did not prefer an available owned-GPU executor") &&
           expect(
               explicit_cpu.executor->descriptor().identifier == "cpu" &&
                   explicit_cpu.source == ctex::exec::ExecutorSelectionSource::explicit_request &&
                   explicit_unavailable.executor->descriptor().availability ==
                       ExecutorAvailability::device_unavailable,
               "explicit selection did not preserve the requested compiled-in executor") &&
           expect(pinned.executor->descriptor().identifier == "cpu" &&
                      fixture.pinned_default() == std::nullopt,
                  "pinned default was not used or cleared");
}

bool environment_selection_is_non_destructive() {
    const auto fixture = registry();
    const auto selected = fixture.select_from_environment("cpu");
    const auto unknown = fixture.select_from_environment("does-not-exist");
    const auto empty = fixture.select_from_environment(std::nullopt);
    return expect(ctex::exec::executor_environment_variable == "CTEX_EXECUTOR" &&
                      selected.executor->descriptor().identifier == "cpu" &&
                      selected.source == ctex::exec::ExecutorSelectionSource::environment,
                  "recognized environment selection was not applied") &&
           expect(unknown.executor->descriptor().identifier == "vulkan" &&
                      unknown.source == ctex::exec::ExecutorSelectionSource::automatic &&
                      unknown.requested_identifier == "does-not-exist" &&
                      unknown.message.find("automatic selection remains") != std::string::npos,
                  "unknown environment selection replaced or obscured automatic choice") &&
           expect(empty.executor->descriptor().identifier == "vulkan",
                  "empty environment selection did not use automatic choice");
}

bool invalid_registrations_and_selections_are_named() {
    ctex::exec::ExecutorRegistry fixture;
    fixture.add(executor("cpu", ExecutorRoute::cpu_reference));
    bool duplicate_named = false;
    try {
        fixture.add(executor("cpu", ExecutorRoute::cpu_reference));
    } catch (const ctex::exec::ExecutorRegistryError& error) {
        duplicate_named = std::string_view(error.what()).find("cpu") != std::string_view::npos;
    }
    bool missing_named = false;
    try {
        static_cast<void>(fixture.select("vulkan"));
    } catch (const ctex::exec::ExecutorRegistryError& error) {
        missing_named = std::string_view(error.what()).find("vulkan") != std::string_view::npos;
    }
    return expect(duplicate_named && missing_named,
                  "duplicate registration or missing selection did not name its executor");
}

bool fallback_reports_require_recovery_before_cpu() {
    const auto report = ctex::exec::make_fallback_report(
        "vulkan", ctex::exec::ExecutionFailureCode::device_lost, "device removed",
        ctex::exec::FallbackDisposition::cpu_fallback, "cpu", true);
    bool unsafe_refused = false;
    try {
        static_cast<void>(ctex::exec::make_fallback_report(
            "vulkan", ctex::exec::ExecutionFailureCode::device_lost, "device removed",
            ctex::exec::FallbackDisposition::cpu_fallback, "cpu", false));
    } catch (const ctex::exec::ExecutorRegistryError& error) {
        unsafe_refused = std::string_view(error.what()).find("recovery") != std::string_view::npos;
    }
    const auto unrecoverable = ctex::exec::make_fallback_report(
        "vulkan", ctex::exec::ExecutionFailureCode::operation_failed, "unpinned input",
        ctex::exec::FallbackDisposition::recovery_required);
    return expect(report.failed_executor == "vulkan" && report.fallback_executor == "cpu" &&
                      report.recovery_restored &&
                      report.message.find("device removed") != std::string::npos &&
                      report.message.find("cpu") != std::string::npos,
                  "fallback report omitted failure, recovery, or fallback identity") &&
           expect(unsafe_refused &&
                      unrecoverable.disposition ==
                          ctex::exec::FallbackDisposition::recovery_required &&
                      unrecoverable.fallback_executor.empty(),
                  "CPU fallback bypassed recovery or unrecoverable work named a fallback");
}

}  // namespace

int main(int argc, char** argv) {
    if (argc == 2 && std::string_view(argv[1]) == "--process-environment") {
        const auto selected = registry().select_process_default();
        return expect(selected.executor->descriptor().identifier == "cpu" &&
                          selected.source == ctex::exec::ExecutorSelectionSource::environment,
                      "process environment did not pin the requested executor")
                   ? 0
                   : 1;
    }
    return enumeration_is_stable_and_availability_is_explicit() &&
                   automatic_explicit_and_pinned_selection_are_distinct() &&
                   environment_selection_is_non_destructive() &&
                   invalid_registrations_and_selections_are_named() &&
                   fallback_reports_require_recovery_before_cpu()
               ? 0
               : 1;
}
