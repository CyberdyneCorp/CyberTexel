#include <algorithm>
#include <cmath>
#include <ctex/paint/masking.hpp>
#include <limits>
#include <stdexcept>

namespace ctex::paint {
namespace {

std::size_t checked_texel_count(std::uint32_t width, std::uint32_t height) {
    if (width == 0 || height == 0 ||
        static_cast<std::size_t>(width) >
            std::numeric_limits<std::size_t>::max() / static_cast<std::size_t>(height)) {
        throw std::invalid_argument("paint mask dimensions are invalid");
    }
    return static_cast<std::size_t>(width) * height;
}

void intersect(PaintMaskView mask, CombinedPaintMask& result) {
    if (mask.values.size() != result.values.size() ||
        !std::all_of(mask.values.begin(), mask.values.end(), [](double value) {
            return std::isfinite(value) && value >= 0.0 && value <= 1.0;
        })) {
        throw std::invalid_argument("paint mask values are invalid");
    }
    for (std::size_t texel = 0; texel < mask.values.size(); ++texel) {
        result.values[texel] *= mask.values[texel];
    }
    ++result.active_input_count;
}

void intersect_optional(const std::optional<PaintMaskView>& mask, CombinedPaintMask& result) {
    if (mask.has_value()) {
        intersect(*mask, result);
    }
}

void validate_rejected(const RejectedCoverageRaster& rejected, std::size_t texel_count) {
    if (rejected.coverage.values.size() != texel_count ||
        !std::all_of(
            rejected.coverage.values.begin(), rejected.coverage.values.end(),
            [](double value) { return std::isfinite(value) && value >= 0.0 && value <= 1.0; })) {
        throw std::invalid_argument("rejected coverage raster is invalid");
    }
    std::vector<double> aggregate(texel_count, 0.0);
    for (const RejectedStampCoverage& event : rejected.stamp_events) {
        if (event.values.size() != texel_count ||
            !std::all_of(event.values.begin(), event.values.end(), [](double value) {
                return std::isfinite(value) && value >= 0.0 && value <= 1.0;
            })) {
            throw std::invalid_argument("rejected stamp coverage is invalid");
        }
        for (std::size_t texel = 0; texel < texel_count; ++texel) {
            aggregate[texel] = std::max(aggregate[texel], event.values[texel]);
        }
    }
    if (aggregate != rejected.coverage.values) {
        throw std::invalid_argument("rejected aggregate and stamp coverage differ");
    }
}

}  // namespace

CombinedPaintMask combine_paint_masks(std::uint32_t width, std::uint32_t height,
                                      const PaintMaskInputs& inputs) {
    CombinedPaintMask result{.width = width,
                             .height = height,
                             .values = std::vector<double>(checked_texel_count(width, height), 1.0),
                             .active_input_count = 0};
    for (const PaintMaskView mask : inputs.active_layer_masks) {
        intersect(mask, result);
    }
    intersect_optional(inputs.colour_id_selection, result);
    intersect_optional(inputs.geometry_selection, result);
    intersect_optional(inputs.screen_selection, result);
    intersect_optional(inputs.uv_island_selection, result);
    return result;
}

RejectedCoverageRaster apply_paint_masks(const RejectedCoverageRaster& rejected,
                                         const PaintMaskInputs& inputs) {
    const std::size_t texel_count =
        checked_texel_count(rejected.coverage.width, rejected.coverage.height);
    validate_rejected(rejected, texel_count);
    const CombinedPaintMask mask =
        combine_paint_masks(rejected.coverage.width, rejected.coverage.height, inputs);
    RejectedCoverageRaster result = rejected;
    std::fill(result.coverage.values.begin(), result.coverage.values.end(), 0.0);
    for (RejectedStampCoverage& event : result.stamp_events) {
        for (std::size_t texel = 0; texel < texel_count; ++texel) {
            event.values[texel] *= mask.values[texel];
            result.coverage.values[texel] =
                std::max(result.coverage.values[texel], event.values[texel]);
        }
    }
    return result;
}

}  // namespace ctex::paint
