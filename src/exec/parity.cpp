#include <algorithm>
#include <cmath>
#include <ctex/exec/parity.hpp>
#include <limits>
#include <stdexcept>

namespace ctex::exec {
namespace {

double allowed_deviation(ParityTolerance tolerance, double reference, double measured) {
    return tolerance.absolute +
           tolerance.relative * std::max(std::abs(reference), std::abs(measured));
}

std::string failure_message(std::size_t index, double deviation, double allowed) {
    return "parity mismatch at value " + std::to_string(index) + ": deviation " +
           std::to_string(deviation) + " exceeds tolerance " + std::to_string(allowed);
}

}  // namespace

ParityTolerance parity_tolerance(ParityValueClass value_class, bool filtered) noexcept {
    switch (value_class) {
        case ParityValueClass::unorm8:
            return {.absolute = (filtered ? 2.0 : 1.0) / 255.0, .relative = 0.0};
        case ParityValueClass::unorm16:
            return {.absolute = (filtered ? 2.0 : 1.0) / 65535.0, .relative = 0.0};
        case ParityValueClass::floating_point:
            return filtered ? ParityTolerance{.absolute = 5.0e-6, .relative = 5.0e-5}
                            : ParityTolerance{.absolute = 1.0e-6, .relative = 1.0e-5};
    }
    return {};
}

ParityComparison compare_parity(std::span<const double> reference, std::span<const double> measured,
                                ParityValueClass value_class, bool filtered) {
    if (reference.size() != measured.size()) {
        throw std::invalid_argument(
            "parity inputs have different value counts: " + std::to_string(reference.size()) +
            " and " + std::to_string(measured.size()));
    }
    if (reference.empty()) {
        throw std::invalid_argument("parity comparison requires at least one value");
    }
    const ParityTolerance tolerance = parity_tolerance(value_class, filtered);
    ParityComparison result{.matches = true,
                            .compared_values = reference.size(),
                            .maximum_absolute_deviation = 0.0,
                            .first_failure = std::nullopt};
    for (std::size_t index = 0; index < reference.size(); ++index) {
        const bool values_are_finite =
            std::isfinite(reference[index]) && std::isfinite(measured[index]);
        const double deviation = values_are_finite ? std::abs(reference[index] - measured[index])
                                                   : std::numeric_limits<double>::infinity();
        const double allowed = values_are_finite
                                   ? allowed_deviation(tolerance, reference[index], measured[index])
                                   : tolerance.absolute;
        result.maximum_absolute_deviation = std::max(result.maximum_absolute_deviation, deviation);
        if (!values_are_finite || deviation > allowed) {
            result.matches = false;
            if (!result.first_failure) {
                result.first_failure =
                    ParityFailure{.value_index = index,
                                  .reference = reference[index],
                                  .measured = measured[index],
                                  .absolute_deviation = deviation,
                                  .allowed_deviation = allowed,
                                  .message = failure_message(index, deviation, allowed)};
            }
        }
    }
    return result;
}

}  // namespace ctex::exec
