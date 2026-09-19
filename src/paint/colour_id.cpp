#include <algorithm>
#include <cmath>
#include <ctex/paint/colour_id.hpp>
#include <limits>
#include <stdexcept>

namespace ctex::paint {
namespace {

bool finite(graph::ColourValue colour) {
    return std::isfinite(colour.r) && std::isfinite(colour.g) && std::isfinite(colour.b) &&
           std::isfinite(colour.a);
}

std::size_t checked_texel_count(ColourIdMapView map) {
    if (map.width == 0 || map.height == 0 ||
        static_cast<std::size_t>(map.width) >
            std::numeric_limits<std::size_t>::max() / map.height) {
        throw std::invalid_argument("colour-ID map dimensions are invalid");
    }
    const std::size_t count = static_cast<std::size_t>(map.width) * map.height;
    if (map.pixels.size() != count || !std::all_of(map.pixels.begin(), map.pixels.end(), finite)) {
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
    if (!finite(picked_colour) || !std::isfinite(tolerance) || tolerance < 0.0) {
        throw std::invalid_argument("colour-ID picked colour or tolerance is invalid");
    }
    ColourIdSelection result{.width = map.width,
                             .height = map.height,
                             .picked_colour = picked_colour,
                             .tolerance = tolerance,
                             .values = std::vector<double>(texel_count, 0.0),
                             .selected_texel_count = 0,
                             .status = ColourIdSelectionStatus::empty};
    for (std::size_t texel = 0; texel < texel_count; ++texel) {
        if (matches(map.pixels[texel], picked_colour, tolerance)) {
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
