#include <algorithm>
#include <ctex/exec/parity_gate.hpp>
#include <limits>
#include <set>
#include <utility>

namespace ctex::exec {
namespace {

std::size_t checked_value_count(const ParityRenderedFixture& rendered,
                                const ParityFixtureChannel& channel) {
    if (rendered.width == 0 || rendered.height == 0 || channel.component_count == 0) {
        throw ParityGateError("parity fixture dimensions and component counts must be non-zero");
    }
    constexpr std::size_t maximum = std::numeric_limits<std::size_t>::max();
    if (rendered.width > maximum / rendered.height ||
        static_cast<std::size_t>(rendered.width) * rendered.height >
            maximum / channel.component_count) {
        throw ParityGateError("parity fixture value count exceeds addressable storage");
    }
    return static_cast<std::size_t>(rendered.width) * rendered.height * channel.component_count;
}

void validate_fixture(const ParityFixture& fixture) {
    if (fixture.identifier.empty() || fixture.document.empty() || fixture.stroke.empty() ||
        fixture.camera.empty() || fixture.material.empty() || fixture.channels.empty()) {
        throw ParityGateError(
            "parity fixture requires an identifier, document, stroke, camera, material, and "
            "channels");
    }
    std::set<std::string_view> semantics;
    for (const ParityFixtureChannel& channel : fixture.channels) {
        if (channel.semantic.empty() || channel.component_count == 0 ||
            !semantics.insert(channel.semantic).second) {
            throw ParityGateError("parity fixture '" + fixture.identifier +
                                  "' has an empty, duplicate, or zero-component channel");
        }
    }
}

const ParityRenderedChannel* find_channel(const ParityRenderedFixture& rendered,
                                          std::string_view semantic) {
    const auto found =
        std::ranges::find(rendered.channels, semantic, &ParityRenderedChannel::semantic);
    return found == rendered.channels.end() ? nullptr : &*found;
}

void validate_rendered(const ParityFixture& fixture, const ParityRenderedFixture& rendered,
                       std::string_view executor) {
    if (rendered.channels.size() != fixture.channels.size()) {
        throw ParityGateError("executor '" + std::string(executor) + "' fixture '" +
                              fixture.identifier + "' returned the wrong channel count");
    }
    std::set<std::string_view> semantics;
    for (const ParityRenderedChannel& channel : rendered.channels) {
        if (channel.semantic.empty() || !semantics.insert(channel.semantic).second) {
            throw ParityGateError("executor '" + std::string(executor) + "' fixture '" +
                                  fixture.identifier + "' returned duplicate channels");
        }
    }
    for (const ParityFixtureChannel& channel : fixture.channels) {
        const ParityRenderedChannel* actual = find_channel(rendered, channel.semantic);
        if (actual == nullptr || actual->values.size() != checked_value_count(rendered, channel)) {
            throw ParityGateError("executor '" + std::string(executor) + "' fixture '" +
                                  fixture.identifier + "' returned malformed channel '" +
                                  channel.semantic + "'");
        }
    }
}

std::vector<ParityRenderedFixture> render_reference(std::span<const ParityFixture> fixtures,
                                                    const ParityExecutorBinding& reference) {
    std::vector<ParityRenderedFixture> rendered;
    rendered.reserve(fixtures.size());
    for (const ParityFixture& fixture : fixtures) {
        rendered.push_back(reference.render(fixture));
        validate_rendered(fixture, rendered.back(), reference.executor->descriptor().identifier);
    }
    return rendered;
}

ParityGateFailure dimension_failure(const ParityFixture& fixture,
                                    const ParityRenderedFixture& reference,
                                    const ParityRenderedFixture& measured) {
    return {.fixture = fixture.identifier,
            .channel = "<dimensions>",
            .value_index = 0,
            .measured_deviation = 0.0,
            .allowed_deviation = 0.0,
            .message = "fixture '" + fixture.identifier + "' dimensions " +
                       std::to_string(measured.width) + "x" + std::to_string(measured.height) +
                       " differ from CPU reference " + std::to_string(reference.width) + "x" +
                       std::to_string(reference.height)};
}

std::vector<ParityGateFailure> compare_fixture(const ParityFixture& fixture,
                                               const ParityRenderedFixture& reference,
                                               const ParityRenderedFixture& measured) {
    if (reference.width != measured.width || reference.height != measured.height) {
        return {dimension_failure(fixture, reference, measured)};
    }
    std::vector<ParityGateFailure> failures;
    for (const ParityFixtureChannel& channel : fixture.channels) {
        const auto* expected = find_channel(reference, channel.semantic);
        const auto* actual = find_channel(measured, channel.semantic);
        const ParityComparison comparison =
            compare_parity(expected->values, actual->values, channel.value_class, channel.filtered);
        if (!comparison.matches) {
            const ParityFailure& failure = *comparison.first_failure;
            failures.push_back({.fixture = fixture.identifier,
                                .channel = channel.semantic,
                                .value_index = failure.value_index,
                                .measured_deviation = failure.absolute_deviation,
                                .allowed_deviation = failure.allowed_deviation,
                                .message = "fixture '" + fixture.identifier + "' channel '" +
                                           channel.semantic + "': " + failure.message});
        }
    }
    return failures;
}

ParityExecutorMeasurement measure_executor(std::span<const ParityFixture> fixtures,
                                           std::span<const ParityRenderedFixture> reference_results,
                                           const ParityExecutorBinding& binding) {
    const ExecutorDescriptor& descriptor = binding.executor->descriptor();
    if (descriptor.availability != ExecutorAvailability::available) {
        return {.executor = descriptor.identifier,
                .device = descriptor.device_name,
                .status = ParityMeasurementStatus::unmeasured,
                .measured_fixtures = 0,
                .failures = {},
                .message = "executor unavailable: " +
                           std::string(executor_availability_name(descriptor.availability))};
    }
    if (!binding.render) {
        return {.executor = descriptor.identifier,
                .device = descriptor.device_name,
                .status = ParityMeasurementStatus::failed,
                .measured_fixtures = 0,
                .failures = {},
                .message = "available executor has no parity renderer"};
    }
    ParityExecutorMeasurement measurement{.executor = descriptor.identifier,
                                          .device = descriptor.device_name,
                                          .status = ParityMeasurementStatus::passed,
                                          .measured_fixtures = 0,
                                          .failures = {},
                                          .message = {}};
    try {
        for (std::size_t index = 0; index < fixtures.size(); ++index) {
            const ParityRenderedFixture rendered = binding.render(fixtures[index]);
            validate_rendered(fixtures[index], rendered, descriptor.identifier);
            auto failures = compare_fixture(fixtures[index], reference_results[index], rendered);
            measurement.failures.insert(measurement.failures.end(), failures.begin(),
                                        failures.end());
            ++measurement.measured_fixtures;
        }
    } catch (const std::exception& error) {
        measurement.failures.push_back({.fixture = "<runner>",
                                        .channel = "<runner>",
                                        .value_index = 0,
                                        .measured_deviation = 0.0,
                                        .allowed_deviation = 0.0,
                                        .message = error.what()});
    }
    if (!measurement.failures.empty()) {
        measurement.status = ParityMeasurementStatus::failed;
        measurement.message = measurement.failures.front().message;
    } else {
        measurement.message = "all fixtures are within declared tolerances";
    }
    return measurement;
}

}  // namespace

bool ParityGateReport::passed() const noexcept {
    return std::ranges::none_of(executors, [](const ParityExecutorMeasurement& measurement) {
        return measurement.status == ParityMeasurementStatus::failed;
    });
}

std::string_view parity_measurement_status_name(ParityMeasurementStatus status) noexcept {
    switch (status) {
        case ParityMeasurementStatus::reference:
            return "reference";
        case ParityMeasurementStatus::passed:
            return "passed";
        case ParityMeasurementStatus::failed:
            return "failed";
        case ParityMeasurementStatus::unmeasured:
            return "unmeasured";
    }
    return "unknown";
}

ParityGateReport run_parity_gate(std::span<const ParityFixture> fixtures,
                                 std::span<const ParityExecutorBinding> executors) {
    if (fixtures.empty() || executors.empty()) {
        throw ParityGateError("parity gate requires fixtures and executors");
    }
    std::set<std::string_view> fixture_ids;
    for (const ParityFixture& fixture : fixtures) {
        validate_fixture(fixture);
        if (!fixture_ids.insert(fixture.identifier).second) {
            throw ParityGateError("parity corpus repeats fixture '" + fixture.identifier + "'");
        }
    }
    std::vector<ParityExecutorBinding> ordered(executors.begin(), executors.end());
    if (std::ranges::any_of(ordered,
                            [](const auto& binding) { return binding.executor == nullptr; })) {
        throw ParityGateError("parity gate contains a null executor binding");
    }
    std::ranges::sort(
        ordered, {}, [](const auto& binding) { return binding.executor->descriptor().identifier; });
    for (std::size_t index = 1; index < ordered.size(); ++index) {
        if (ordered[index - 1].executor->descriptor().identifier ==
            ordered[index].executor->descriptor().identifier) {
            throw ParityGateError("parity gate repeats executor '" +
                                  ordered[index].executor->descriptor().identifier + "'");
        }
    }
    const auto is_reference = [](const auto& binding) {
        const auto& descriptor = binding.executor->descriptor();
        return descriptor.route == ExecutorRoute::cpu_reference &&
               descriptor.availability == ExecutorAvailability::available;
    };
    if (std::ranges::count_if(ordered, is_reference) != 1) {
        throw ParityGateError("parity gate requires exactly one available CPU reference");
    }
    const auto reference = std::ranges::find_if(ordered, is_reference);
    if (reference == ordered.end() || !reference->render) {
        throw ParityGateError("parity gate requires an available CPU reference renderer");
    }
    const auto reference_results = render_reference(fixtures, *reference);
    ParityGateReport report;
    report.executors.reserve(ordered.size());
    for (const ParityExecutorBinding& binding : ordered) {
        if (&binding == &*reference) {
            report.executors.push_back({.executor = binding.executor->descriptor().identifier,
                                        .device = binding.executor->descriptor().device_name,
                                        .status = ParityMeasurementStatus::reference,
                                        .measured_fixtures = fixtures.size(),
                                        .failures = {},
                                        .message = "CPU correctness reference"});
        } else {
            report.executors.push_back(measure_executor(fixtures, reference_results, binding));
        }
    }
    return report;
}

}  // namespace ctex::exec
