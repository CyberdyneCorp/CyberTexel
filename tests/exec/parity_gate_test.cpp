#include <ctex/exec/parity_gate.hpp>
#include <iostream>
#include <memory>
#include <string_view>
#include <utility>

namespace {

using ctex::exec::ExecutorAvailability;
using ctex::exec::ExecutorDescriptor;
using ctex::exec::ExecutorRoute;
using ctex::exec::ParityExecutorBinding;
using ctex::exec::ParityFixture;
using ctex::exec::ParityMeasurementStatus;
using ctex::exec::ParityRenderedChannel;
using ctex::exec::ParityRenderedFixture;

bool expect(bool condition, std::string_view message) {
    if (!condition) {
        std::cerr << message << '\n';
    }
    return condition;
}

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

FixtureExecutor executor(std::string identifier, ExecutorRoute route,
                         ExecutorAvailability availability) {
    return FixtureExecutor({.identifier = std::move(identifier),
                            .display_name = "Parity fixture",
                            .device_name = "Fixture device",
                            .route = route,
                            .availability = availability,
                            .features = features()});
}

ParityFixture fixture() {
    return {.identifier = "paint-basic",
            .document = "document:v1;width=1;height=1",
            .stroke = "stroke:v1;x=0.5;y=0.5;radius=1",
            .camera = "camera:v1;identity",
            .material = "material:v1;blend=normal",
            .channels = {{.semantic = "base_color",
                          .value_class = ctex::exec::ParityValueClass::unorm8,
                          .filtered = false,
                          .component_count = 4}}};
}

ParityRenderedFixture rendered(double red = 0.25) {
    return {.width = 1,
            .height = 1,
            .channels = {
                ParityRenderedChannel{.semantic = "base_color", .values = {red, 0.5, 0.75, 1.0}}}};
}

bool available_executors_are_measured_and_unavailable_ones_are_reported() {
    auto cpu = executor("cpu", ExecutorRoute::cpu_reference, ExecutorAvailability::available);
    auto gpu = executor("gpu", ExecutorRoute::owned_gpu, ExecutorAvailability::available);
    auto host =
        executor("host", ExecutorRoute::host_executed, ExecutorAvailability::host_not_attached);
    bool unavailable_called = false;
    const std::vector<ParityExecutorBinding> bindings{
        {.executor = &host,
         .render =
             [&](const ParityFixture&) {
                 unavailable_called = true;
                 return rendered();
             }},
        {.executor = &gpu, .render = [](const ParityFixture&) { return rendered(0.252); }},
        {.executor = &cpu, .render = [](const ParityFixture&) { return rendered(); }},
    };
    const std::vector<ParityFixture> fixtures{fixture()};
    const auto report = ctex::exec::run_parity_gate(fixtures, bindings);
    return expect(report.passed() && report.executors.size() == 3 &&
                      report.executors[0].executor == "cpu" &&
                      report.executors[0].status == ParityMeasurementStatus::reference &&
                      report.executors[1].executor == "gpu" &&
                      report.executors[1].status == ParityMeasurementStatus::passed &&
                      report.executors[2].executor == "host" &&
                      report.executors[2].status == ParityMeasurementStatus::unmeasured,
                  "parity gate did not classify reference, measured, and unavailable executors") &&
           expect(!unavailable_called &&
                      report.executors[2].message.find("host-not-attached") != std::string::npos,
                  "unavailable executor was rendered or silently counted as passing");
}

bool backend_drift_names_case_channel_and_deviation() {
    auto cpu = executor("cpu", ExecutorRoute::cpu_reference, ExecutorAvailability::available);
    auto gpu = executor("gpu", ExecutorRoute::owned_gpu, ExecutorAvailability::available);
    const std::vector<ParityExecutorBinding> bindings{
        {.executor = &cpu, .render = [](const ParityFixture&) { return rendered(); }},
        {.executor = &gpu, .render = [](const ParityFixture&) { return rendered(0.35); }},
    };
    const std::vector<ParityFixture> fixtures{fixture()};
    const auto report = ctex::exec::run_parity_gate(fixtures, bindings);
    const auto& measurement = report.executors[1];
    return expect(!report.passed() && measurement.status == ParityMeasurementStatus::failed &&
                      measurement.failures.size() == 1 &&
                      measurement.failures[0].fixture == "paint-basic" &&
                      measurement.failures[0].channel == "base_color" &&
                      measurement.failures[0].measured_deviation >
                          measurement.failures[0].allowed_deviation &&
                      measurement.message.find("paint-basic") != std::string::npos &&
                      measurement.message.find("base_color") != std::string::npos,
                  "backend drift did not fail with case, channel, and measured deviation");
}

bool available_executor_without_renderer_fails_the_gate() {
    auto cpu = executor("cpu", ExecutorRoute::cpu_reference, ExecutorAvailability::available);
    auto gpu = executor("gpu", ExecutorRoute::owned_gpu, ExecutorAvailability::available);
    const std::vector<ParityExecutorBinding> bindings{
        {.executor = &cpu, .render = [](const ParityFixture&) { return rendered(); }},
        {.executor = &gpu, .render = {}},
    };
    const std::vector<ParityFixture> fixtures{fixture()};
    const auto report = ctex::exec::run_parity_gate(fixtures, bindings);
    return expect(!report.passed() &&
                      report.executors[1].status == ParityMeasurementStatus::failed &&
                      report.executors[1].message.find("no parity renderer") != std::string::npos,
                  "available executor without a renderer was treated as unmeasured or passed");
}

bool malformed_corpus_and_reference_are_refused() {
    auto cpu = executor("cpu", ExecutorRoute::cpu_reference, ExecutorAvailability::available);
    ParityFixture malformed = fixture();
    malformed.material.clear();
    bool corpus_refused = false;
    try {
        const std::vector<ParityFixture> fixtures{malformed};
        const std::vector<ParityExecutorBinding> bindings{
            {.executor = &cpu, .render = [](const ParityFixture&) { return rendered(); }}};
        static_cast<void>(ctex::exec::run_parity_gate(fixtures, bindings));
    } catch (const ctex::exec::ParityGateError&) {
        corpus_refused = true;
    }

    bool reference_refused = false;
    try {
        const std::vector<ParityFixture> fixtures{fixture()};
        const std::vector<ParityExecutorBinding> bindings{
            {.executor = &cpu, .render = [](const ParityFixture&) {
                 auto result = rendered();
                 result.channels[0].values.pop_back();
                 return result;
             }}};
        static_cast<void>(ctex::exec::run_parity_gate(fixtures, bindings));
    } catch (const ctex::exec::ParityGateError&) {
        reference_refused = true;
    }
    return expect(corpus_refused && reference_refused,
                  "malformed parity corpus or CPU reference output was accepted");
}

}  // namespace

int main() {
    return available_executors_are_measured_and_unavailable_ones_are_reported() &&
                   backend_drift_names_case_channel_and_deviation() &&
                   available_executor_without_renderer_fails_the_gate() &&
                   malformed_corpus_and_reference_are_refused()
               ? 0
               : 1;
}
