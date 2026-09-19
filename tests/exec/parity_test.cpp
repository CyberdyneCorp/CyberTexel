#include <array>
#include <cmath>
#include <ctex/exec/parity.hpp>
#include <iostream>
#include <limits>
#include <string_view>

namespace {

using ctex::exec::ParityValueClass;

bool expect(bool condition, std::string_view message) {
    if (!condition) {
        std::cerr << message << '\n';
    }
    return condition;
}

bool tolerances_are_numeric_and_precision_specific() {
    const auto u8 = ctex::exec::parity_tolerance(ParityValueClass::unorm8, false);
    const auto u8_filtered = ctex::exec::parity_tolerance(ParityValueClass::unorm8, true);
    const auto u16 = ctex::exec::parity_tolerance(ParityValueClass::unorm16, false);
    const auto u16_filtered = ctex::exec::parity_tolerance(ParityValueClass::unorm16, true);
    const auto floating = ctex::exec::parity_tolerance(ParityValueClass::floating_point, false);
    const auto floating_filtered =
        ctex::exec::parity_tolerance(ParityValueClass::floating_point, true);
    return expect(u8.absolute == 1.0 / 255.0 && u8.relative == 0.0 &&
                      u8_filtered.absolute == 2.0 / 255.0,
                  "8-bit parity tolerance is not one/two normalized code values") &&
           expect(u16.absolute == 1.0 / 65535.0 && u16.relative == 0.0 &&
                      u16_filtered.absolute == 2.0 / 65535.0,
                  "16-bit parity tolerance is not one/two normalized code values") &&
           expect(floating.absolute == 1.0e-6 && floating.relative == 1.0e-5 &&
                      floating_filtered.absolute == 5.0e-6 && floating_filtered.relative == 5.0e-5,
                  "floating-point filtered and unfiltered tolerances are not explicit");
}

bool boundary_values_pass_and_excess_values_fail() {
    const double step = 1.0 / 255.0;
    const std::array<double, 2> reference{0.25, 0.75};
    const std::array<double, 2> at_boundary{0.25 + step, 0.75 - step};
    const std::array<double, 2> outside{0.25, 0.75 + step * 1.01};
    const auto matching =
        ctex::exec::compare_parity(reference, at_boundary, ParityValueClass::unorm8, false);
    const auto failing =
        ctex::exec::compare_parity(reference, outside, ParityValueClass::unorm8, false);
    return expect(matching.matches && matching.compared_values == 2 && !matching.first_failure,
                  "values on the declared parity boundary did not match") &&
           expect(!failing.matches && failing.first_failure->value_index == 1 &&
                      failing.first_failure->absolute_deviation >
                          failing.first_failure->allowed_deviation &&
                      failing.first_failure->message.find("value 1") != std::string::npos,
                  "out-of-tolerance value did not report index and measured bound");
}

bool floating_point_comparison_uses_absolute_and_relative_terms() {
    const std::array<double, 2> reference{0.0, 1000.0};
    const std::array<double, 2> measured{0.9e-6, 1000.009};
    const auto comparison =
        ctex::exec::compare_parity(reference, measured, ParityValueClass::floating_point, false);
    return expect(comparison.matches && comparison.maximum_absolute_deviation > 0.008,
                  "floating-point comparison did not combine absolute and relative tolerance");
}

bool filtering_uses_the_declared_wider_bound() {
    const std::array<double, 1> reference{0.5};
    const std::array<double, 1> measured{0.5 + 1.5 / 255.0};
    const auto unfiltered =
        ctex::exec::compare_parity(reference, measured, ParityValueClass::unorm8, false);
    const auto filtered =
        ctex::exec::compare_parity(reference, measured, ParityValueClass::unorm8, true);
    return expect(!unfiltered.matches && filtered.matches,
                  "filtered values did not use their separately declared tolerance");
}

bool invalid_comparisons_do_not_hide_failures() {
    const std::array<double, 1> one{0.0};
    const std::array<double, 2> two{0.0, 0.0};
    bool size_refused = false;
    try {
        static_cast<void>(ctex::exec::compare_parity(one, two, ParityValueClass::unorm16, false));
    } catch (const std::invalid_argument& error) {
        size_refused = std::string_view(error.what()).find("different") != std::string_view::npos;
    }
    const std::array<double, 1> non_finite{std::numeric_limits<double>::quiet_NaN()};
    const auto nan =
        ctex::exec::compare_parity(non_finite, non_finite, ParityValueClass::floating_point, false);
    bool empty_refused = false;
    try {
        static_cast<void>(ctex::exec::compare_parity({}, {}, ParityValueClass::unorm8, false));
    } catch (const std::invalid_argument& error) {
        empty_refused =
            std::string_view(error.what()).find("at least one") != std::string_view::npos;
    }
    return expect(size_refused && empty_refused && !nan.matches && nan.first_failure.has_value() &&
                      std::isinf(nan.maximum_absolute_deviation),
                  "mismatched sizes or non-finite values were accepted as parity");
}

}  // namespace

int main() {
    return tolerances_are_numeric_and_precision_specific() &&
                   boundary_values_pass_and_excess_values_fail() &&
                   floating_point_comparison_uses_absolute_and_relative_terms() &&
                   filtering_uses_the_declared_wider_bound() &&
                   invalid_comparisons_do_not_hide_failures()
               ? 0
               : 1;
}
