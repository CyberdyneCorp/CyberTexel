#ifndef CTEX_PAINT_PARAMETERS_HPP
#define CTEX_PAINT_PARAMETERS_HPP

#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace ctex::paint {

struct ToolParameterDescriptor {
    std::string_view name;
    double default_value{};
    double minimum{};
    double maximum{};
};

struct ToolParameterClamp {
    std::string name;
    double supplied{};
    double resolved{};
    friend bool operator==(const ToolParameterClamp&, const ToolParameterClamp&) = default;
};

struct ToolParameterReport {
    std::vector<ToolParameterClamp> clamps;

    friend bool operator==(const ToolParameterReport&, const ToolParameterReport&) = default;

    [[nodiscard]] std::optional<ToolParameterClamp> clamp_for(std::string_view name) const;
};

[[nodiscard]] double validate_tool_parameter(const ToolParameterDescriptor& descriptor,
                                             double supplied, ToolParameterReport& report);

}  // namespace ctex::paint

#endif
