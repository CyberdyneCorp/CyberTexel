#include <algorithm>
#include <cmath>
#include <ctex/paint/parameters.hpp>
#include <stdexcept>

namespace ctex::paint {

std::optional<ToolParameterClamp> ToolParameterReport::clamp_for(std::string_view name) const {
    const auto found =
        std::find_if(clamps.begin(), clamps.end(),
                     [&](const ToolParameterClamp& item) { return item.name == name; });
    if (found == clamps.end()) {
        return std::nullopt;
    }
    return *found;
}

double validate_tool_parameter(const ToolParameterDescriptor& descriptor, double supplied,
                               ToolParameterReport& report) {
    if (descriptor.name.empty() || !std::isfinite(descriptor.default_value) ||
        !std::isfinite(descriptor.minimum) || !std::isfinite(descriptor.maximum) ||
        descriptor.minimum > descriptor.default_value ||
        descriptor.default_value > descriptor.maximum) {
        throw std::invalid_argument("tool parameter descriptor is invalid");
    }
    if (!std::isfinite(supplied)) {
        throw std::invalid_argument("tool parameter '" + std::string(descriptor.name) +
                                    "' must be finite");
    }
    const double resolved = std::clamp(supplied, descriptor.minimum, descriptor.maximum);
    if (resolved != supplied) {
        report.clamps.push_back(
            {.name = std::string(descriptor.name), .supplied = supplied, .resolved = resolved});
    }
    return resolved;
}

}  // namespace ctex::paint
