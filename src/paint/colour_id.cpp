#include <algorithm>
#include <cmath>
#include <ctex/paint/colour_id.hpp>
#include <limits>
#include <stdexcept>
#include <utility>

namespace ctex::paint {
namespace {

bool finite(graph::ColourValue colour) {
    return std::isfinite(colour.r) && std::isfinite(colour.g) && std::isfinite(colour.b) &&
           std::isfinite(colour.a);
}

bool valid_id_colour(graph::ColourValue colour) {
    return finite(colour) && colour.r >= 0.0F && colour.r <= 1.0F && colour.g >= 0.0F &&
           colour.g <= 1.0F && colour.b >= 0.0F && colour.b <= 1.0F;
}

std::size_t checked_texel_count(ColourIdMapView map) {
    if (map.width == 0 || map.height == 0 ||
        static_cast<std::size_t>(map.width) >
            std::numeric_limits<std::size_t>::max() / map.height) {
        throw std::invalid_argument("colour-ID map dimensions are invalid");
    }
    const std::size_t count = static_cast<std::size_t>(map.width) * map.height;
    if (map.pixels.size() != count ||
        !std::all_of(map.pixels.begin(), map.pixels.end(), valid_id_colour)) {
        throw std::invalid_argument("colour-ID map pixels are invalid");
    }
    return count;
}

bool matches(graph::ColourValue candidate, graph::ColourValue picked, double tolerance) {
    return std::hypot(static_cast<double>(candidate.r) - picked.r,
                      static_cast<double>(candidate.g) - picked.g,
                      static_cast<double>(candidate.b) - picked.b) <= tolerance;
}

}  // namespace

ColourIdSelection select_colour_id(ColourIdMapView map, graph::ColourValue picked_colour,
                                   double tolerance) {
    const std::size_t texel_count = checked_texel_count(map);
    if (!valid_id_colour(picked_colour)) {
        throw std::invalid_argument("colour-ID picked colour is invalid");
    }
    ToolParameterReport parameter_report;
    const double resolved_tolerance =
        validate_tool_parameter(colour_id_tolerance_parameter, tolerance, parameter_report);
    ColourIdSelection result{.width = map.width,
                             .height = map.height,
                             .picked_colour = picked_colour,
                             .tolerance = resolved_tolerance,
                             .parameter_report = std::move(parameter_report),
                             .values = std::vector<double>(texel_count, 0.0),
                             .selected_texel_count = 0,
                             .status = ColourIdSelectionStatus::empty};
    for (std::size_t texel = 0; texel < texel_count; ++texel) {
        if (matches(map.pixels[texel], picked_colour, resolved_tolerance)) {
            result.values[texel] = 1.0;
            ++result.selected_texel_count;
        }
    }
    if (result.selected_texel_count != 0) {
        result.status = ColourIdSelectionStatus::matched;
    }
    return result;
}

}  // namespace ctex::paint
