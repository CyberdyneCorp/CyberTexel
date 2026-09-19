#include <ctex/paint/parameters.hpp>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string_view>

namespace {

using namespace ctex::paint;

bool expect(bool condition, std::string_view message) {
    if (!condition) {
        std::cerr << message << '\n';
    }
    return condition;
}

bool shared_validation_preserves_or_reports_values() {
    constexpr ToolParameterDescriptor descriptor{"test.amount", 0.5, 0.0, 1.0};
    ToolParameterReport report;
    const double unchanged = validate_tool_parameter(descriptor, 0.25, report);
    const double lower = validate_tool_parameter(descriptor, -2.0, report);
    const double upper = validate_tool_parameter(descriptor, 3.0, report);
    return expect(unchanged == 0.25 && lower == 0.0 && upper == 1.0,
                  "shared parameter validation resolved the wrong values") &&
           expect(report.clamps.size() == 2 &&
                      report.clamp_for("test.amount") == ToolParameterClamp{.name = "test.amount",
                                                                            .supplied = -2.0,
                                                                            .resolved = 0.0} &&
                      !report.clamp_for("missing").has_value(),
                  "shared parameter validation did not report clamps by stable name");
}

bool invalid_values_and_descriptors_are_transactionally_refused() {
    constexpr ToolParameterDescriptor descriptor{"test.amount", 0.5, 0.0, 1.0};
    ToolParameterReport report;
    bool value_refused = false;
    try {
        static_cast<void>(
            validate_tool_parameter(descriptor, std::numeric_limits<double>::quiet_NaN(), report));
    } catch (const std::invalid_argument&) {
        value_refused = true;
    }
    bool descriptor_refused = false;
    try {
        constexpr ToolParameterDescriptor invalid{"test.invalid", 2.0, 0.0, 1.0};
        static_cast<void>(validate_tool_parameter(invalid, 0.5, report));
    } catch (const std::invalid_argument&) {
        descriptor_refused = true;
    }
    return expect(value_refused && descriptor_refused && report.clamps.empty(),
                  "invalid parameter input changed the report or was not refused");
}

}  // namespace

int main() {
    return shared_validation_preserves_or_reports_values() &&
                   invalid_values_and_descriptors_are_transactionally_refused()
               ? 0
               : 1;
}
