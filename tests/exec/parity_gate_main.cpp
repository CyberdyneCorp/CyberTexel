#include <ctex/exec/host_execution.hpp>
#include <iostream>

#include "../fixtures/executor_parity/corpus.hpp"

namespace {

ctex::emit::DeviceFeatureSet host_features() {
    return {.binding_budget = 16,
            .maximum_texture_dimension = 8192,
            .supported_texture_formats = {ctex::emit::TextureFormat::rgba8_unorm,
                                          ctex::emit::TextureFormat::rgba16_unorm,
                                          ctex::emit::TextureFormat::rgba32_float,
                                          ctex::emit::TextureFormat::depth32_float},
            .floating_point_filtering = true,
            .compute_available = true};
}

}  // namespace

int main() {
    ctex::exec::CpuReferenceExecutor cpu;
    ctex::exec::HostExecutedExecutor host("CI host device", host_features(), false);
    const std::vector<ctex::exec::ParityFixture> fixtures =
        ctex::test::executor_parity::fixture_descriptors();
    const std::vector<ctex::exec::ParityExecutorBinding> executors{
        {.executor = &cpu, .render = ctex::test::executor_parity::render_committed_fixture},
        {.executor = &host, .render = {}},
    };
    const ctex::exec::ParityGateReport report = ctex::exec::run_parity_gate(fixtures, executors);
    for (const auto& measurement : report.executors) {
        std::cout << ctex::exec::parity_measurement_status_name(measurement.status) << ' '
                  << measurement.executor << " (" << measurement.device
                  << "): " << measurement.message << '\n';
    }
    return report.passed() ? 0 : 1;
}
