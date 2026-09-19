#include <algorithm>
#include <cmath>
#include <ctex/paint/brush.hpp>
#include <limits>
#include <set>
#include <stdexcept>
#include <utility>

namespace ctex::paint {
namespace {

std::size_t checked_texel_count(const RejectedCoverageRaster& rejected) {
    if (rejected.coverage.width == 0 || rejected.coverage.height == 0 ||
        static_cast<std::size_t>(rejected.coverage.width) >
            std::numeric_limits<std::size_t>::max() / rejected.coverage.height) {
        throw std::invalid_argument("paint tool coverage dimensions are invalid");
    }
    return static_cast<std::size_t>(rejected.coverage.width) * rejected.coverage.height;
}

void validate_channel_ids(std::span<const PaintToolChannelRaster> channels, std::string_view role) {
    std::set<std::string_view> identifiers;
    for (const PaintToolChannelRaster& channel : channels) {
        if (channel.semantic_id.empty() || channel.component_count == 0 ||
            channel.component_count > 4 || !identifiers.insert(channel.semantic_id).second) {
            throw std::invalid_argument(std::string(role) +
                                        " channel descriptors are invalid or duplicated");
        }
    }
}

const PaintToolChannelRaster& material_channel(std::span<const PaintToolChannelRaster> material,
                                               std::string_view semantic_id) {
    const auto found = std::find_if(material.begin(), material.end(), [&](const auto& channel) {
        return channel.semantic_id == semantic_id;
    });
    if (found == material.end()) {
        throw std::invalid_argument("active material does not provide enabled channel '" +
                                    std::string(semantic_id) + "'");
    }
    return *found;
}

DepositionRaster tool_deposition(const ResolvedStroke& stroke,
                                 const RejectedCoverageRaster& rejected,
                                 const PaintMaskInputs& masks, DepositionMode mode) {
    return evaluate_deposition(stroke, apply_paint_masks(rejected, masks), mode);
}

void validate_eraser_values(std::span<const double> values, std::size_t expected) {
    if (values.size() != expected || !std::all_of(values.begin(), values.end(), [](double value) {
            return std::isfinite(value) && value >= 0.0 && value <= 1.0;
        })) {
        throw std::invalid_argument("eraser stroke-start values are invalid");
    }
}

void validate_eraser_target(EraserTarget target) {
    if (target != EraserTarget::layer_opacity && target != EraserTarget::mask) {
        throw std::invalid_argument("eraser target is invalid");
    }
}

}  // namespace

PaintToolShadeResult shade_paint_tool_channels(
    std::uint32_t width, std::uint32_t height,
    std::span<const PaintToolChannelRaster> enabled_layer_snapshot,
    std::span<const PaintToolChannelRaster> material, std::span<const double> strength,
    std::string_view blend_mode) {
    if (enabled_layer_snapshot.empty()) {
        throw std::invalid_argument("paint tool requires at least one enabled layer channel");
    }
    if (material.empty()) {
        throw std::invalid_argument("paint tool requires an active material");
    }
    validate_channel_ids(enabled_layer_snapshot, "enabled layer");
    validate_channel_ids(material, "material");
    for (const PaintToolChannelRaster& layer : enabled_layer_snapshot) {
        const PaintToolChannelRaster& paint = material_channel(material, layer.semantic_id);
        if (paint.component_count != layer.component_count) {
            throw std::invalid_argument("material and layer component counts differ for channel '" +
                                        layer.semantic_id + "'");
        }
    }

    PaintToolShadeResult result;
    result.channels.reserve(enabled_layer_snapshot.size());
    result.applied_channel_ids.reserve(enabled_layer_snapshot.size());
    const DepositionRaster shading_strength{.width = width,
                                            .height = height,
                                            .mode = DepositionMode::non_building,
                                            .non_building_coverage = {},
                                            .build_up_deposition = {},
                                            .strength = {strength.begin(), strength.end()},
                                            .applied_stamp_count = 0};
    for (const PaintToolChannelRaster& layer : enabled_layer_snapshot) {
        const PaintToolChannelRaster& paint = material_channel(material, layer.semantic_id);
        StrokeBlendRaster blended = blend_stroke_snapshot(width, height, layer.pixels, paint.pixels,
                                                          shading_strength, blend_mode);
        result.channels.push_back({.semantic_id = layer.semantic_id,
                                   .component_count = layer.component_count,
                                   .pixels = std::move(blended.pixels)});
        result.applied_channel_ids.push_back(layer.semantic_id);
    }
    return result;
}

BrushResult apply_brush(const ResolvedStroke& stroke, const RejectedCoverageRaster& rejected,
                        std::span<const PaintToolChannelRaster> enabled_layer_snapshot,
                        std::span<const PaintToolChannelRaster> material,
                        const BrushSettings& settings) {
    static_cast<void>(checked_texel_count(rejected));
    BrushResult result{
        .width = rejected.coverage.width,
        .height = rejected.coverage.height,
        .deposition = tool_deposition(stroke, rejected, settings.masks, settings.deposition_mode),
        .channels = {},
        .applied_channel_ids = {}};
    PaintToolShadeResult shaded =
        shade_paint_tool_channels(result.width, result.height, enabled_layer_snapshot, material,
                                  result.deposition.strength, settings.blend_mode);
    result.channels = std::move(shaded.channels);
    result.applied_channel_ids = std::move(shaded.applied_channel_ids);
    return result;
}

EraserResult apply_eraser(const ResolvedStroke& stroke, const RejectedCoverageRaster& rejected,
                          std::span<const double> stroke_start_values, EraserTarget target,
                          const EraserSettings& settings) {
    const std::size_t texel_count = checked_texel_count(rejected);
    validate_eraser_target(target);
    validate_eraser_values(stroke_start_values, texel_count);
    EraserResult result{
        .width = rejected.coverage.width,
        .height = rejected.coverage.height,
        .target = target,
        .deposition = tool_deposition(stroke, rejected, settings.masks, settings.deposition_mode),
        .values = {stroke_start_values.begin(), stroke_start_values.end()}};
    for (std::size_t texel = 0; texel < texel_count; ++texel) {
        result.values[texel] *= 1.0 - result.deposition.strength[texel];
    }
    return result;
}

}  // namespace ctex::paint
