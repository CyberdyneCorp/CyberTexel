#ifndef CTEX_EXEC_PARITY_GATE_HPP
#define CTEX_EXEC_PARITY_GATE_HPP

#include <cstddef>
#include <cstdint>
#include <ctex/exec/executor.hpp>
#include <ctex/exec/parity.hpp>
#include <functional>
#include <span>
#include <stdexcept>
#include <string>
#include <vector>

namespace ctex::exec {

struct ParityFixtureChannel {
    std::string semantic;
    ParityValueClass value_class{};
    bool filtered{};
    std::uint8_t component_count{1};
};

struct ParityFixture {
    std::string identifier;
    std::string document;
    std::string stroke;
    std::string camera;
    std::string material;
    std::vector<ParityFixtureChannel> channels;
};

struct ParityRenderedChannel {
    std::string semantic;
    std::vector<double> values;
};

struct ParityRenderedFixture {
    std::uint32_t width{};
    std::uint32_t height{};
    std::vector<ParityRenderedChannel> channels;
};

using ParityRenderCallback = std::function<ParityRenderedFixture(const ParityFixture& fixture)>;

struct ParityExecutorBinding {
    const Executor* executor{};
    ParityRenderCallback render;
};

enum class ParityMeasurementStatus : std::uint8_t { reference, passed, failed, unmeasured };

struct ParityGateFailure {
    std::string fixture;
    std::string channel;
    std::size_t value_index{};
    double measured_deviation{};
    double allowed_deviation{};
    std::string message;
};

struct ParityExecutorMeasurement {
    std::string executor;
    std::string device;
    ParityMeasurementStatus status{};
    std::size_t measured_fixtures{};
    std::vector<ParityGateFailure> failures;
    std::string message;
};

struct ParityGateReport {
    std::vector<ParityExecutorMeasurement> executors;
    [[nodiscard]] bool passed() const noexcept;
};

class ParityGateError final : public std::invalid_argument {
public:
    using std::invalid_argument::invalid_argument;
};

[[nodiscard]] std::string_view parity_measurement_status_name(
    ParityMeasurementStatus status) noexcept;
[[nodiscard]] ParityGateReport run_parity_gate(std::span<const ParityFixture> fixtures,
                                               std::span<const ParityExecutorBinding> executors);

}  // namespace ctex::exec

#endif
