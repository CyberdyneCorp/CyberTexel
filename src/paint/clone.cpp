#include <algorithm>
#include <cmath>
#include <ctex/paint/clone.hpp>
#include <limits>
#include <stdexcept>
#include <string>
#include <utility>

namespace ctex::paint {
namespace {

bool finite(Vec2d value) { return std::isfinite(value.x) && std::isfinite(value.y); }

std::size_t checked_texel_count(const CachedSurfaceMaps& maps,
                                const RejectedCoverageRaster& rejected) {
    const std::uint32_t width = maps.surface.width;
    const std::uint32_t height = maps.surface.height;
    if (width == 0 || height == 0 ||
        static_cast<std::size_t>(width) > std::numeric_limits<std::size_t>::max() / height) {
        throw std::invalid_argument("clone surface dimensions are invalid");
    }
    const std::size_t count = static_cast<std::size_t>(width) * height;
    if (maps.texture_set_id.empty() || maps.surface.texels.size() != count ||
        maps.coverage.size() != count || rejected.coverage.width != width ||
        rejected.coverage.height != height) {
        throw std::invalid_argument("clone surface and coverage dimensions are inconsistent");
    }
    for (std::size_t texel = 0; texel < count; ++texel) {
        if (maps.coverage[texel] > 1 ||
            (maps.coverage[texel] != 0) != maps.surface.covered(texel)) {
            throw std::invalid_argument("clone surface coverage is invalid");
        }
    }
    return count;
}

void validate_mode(CloneMode mode) {
    if (mode != CloneMode::aligned && mode != CloneMode::fixed) {
        throw std::invalid_argument("clone mode is invalid");
    }
}

void validate_source_snapshot(std::span<const PaintToolChannelRaster> source_snapshot,
                              std::size_t texel_count) {
    if (source_snapshot.empty() ||
        std::any_of(source_snapshot.begin(), source_snapshot.end(),
                    [&](const auto& channel) { return channel.pixels.size() != texel_count; })) {
        throw std::invalid_argument("clone source snapshot dimensions are inconsistent");
    }
}

std::size_t source_texel_index(const TextureSpaceRaster& surface, Vec2d uv) {
    const double local_u = uv.x - surface.tile_origin.x;
    const double local_v = uv.y - surface.tile_origin.y;
    if (!finite(uv) || local_u < 0.0 || local_u >= 1.0 || local_v < 0.0 || local_v >= 1.0) {
        return no_clone_sample;
    }
    const auto x =
        static_cast<std::uint32_t>(std::floor(local_u * static_cast<double>(surface.width)));
    const auto bottom_up_y =
        static_cast<std::uint32_t>(std::floor(local_v * static_cast<double>(surface.height)));
    const std::uint32_t y = surface.height - 1 - bottom_up_y;
    return static_cast<std::size_t>(y) * surface.width + x;
}

Vec2d source_coordinate(CloneMode mode, Vec2d source_anchor, Vec2d destination_anchor,
                        Vec2d destination) {
    if (mode == CloneMode::fixed) {
        return source_anchor;
    }
    return {.x = destination.x + source_anchor.x - destination_anchor.x,
            .y = destination.y + source_anchor.y - destination_anchor.y};
}

std::vector<std::size_t> resolve_source_samples(const CachedSurfaceMaps& maps,
                                                const CloneSource& source,
                                                const CloneSettings& settings) {
    std::vector<std::size_t> result(maps.surface.texels.size(), no_clone_sample);
    for (std::size_t texel = 0; texel < result.size(); ++texel) {
        if (maps.coverage[texel] == 0) {
            continue;
        }
        const Vec2d destination = maps.surface.texels[texel].uv;
        if (!finite(destination)) {
            throw std::invalid_argument("clone destination surface contains a non-finite UV");
        }
        result[texel] = source_texel_index(
            maps.surface, source_coordinate(settings.mode, source.uv,
                                            settings.destination_anchor_uv, destination));
    }
    return result;
}

RejectedCoverageRaster restrict_to_valid_samples(const RejectedCoverageRaster& rejected,
                                                 std::span<const std::size_t> sample_indices) {
    RejectedCoverageRaster result = rejected;
    for (std::size_t texel = 0; texel < sample_indices.size(); ++texel) {
        if (sample_indices[texel] == no_clone_sample) {
            result.coverage.values[texel] = 0.0;
            for (RejectedStampCoverage& event : result.stamp_events) {
                event.values[texel] = 0.0;
            }
        }
    }
    return result;
}

std::vector<PaintToolChannelRaster> sample_channels(
    std::span<const PaintToolChannelRaster> source_snapshot,
    std::span<const std::size_t> sample_indices) {
    std::vector<PaintToolChannelRaster> sampled;
    sampled.reserve(source_snapshot.size());
    for (const PaintToolChannelRaster& source : source_snapshot) {
        PaintToolChannelRaster channel{
            .semantic_id = source.semantic_id,
            .component_count = source.component_count,
            .pixels = std::vector<graph::ColourValue>(sample_indices.size())};
        for (std::size_t texel = 0; texel < sample_indices.size(); ++texel) {
            if (sample_indices[texel] != no_clone_sample) {
                channel.pixels[texel] = source.pixels[sample_indices[texel]];
            }
        }
        sampled.push_back(std::move(channel));
    }
    return sampled;
}

const CloneSource& checked_source(const CloneSourceState& state,
                                  std::string_view destination_texture_set) {
    if (!state.source()) {
        throw std::invalid_argument("clone source has not been set");
    }
    const CloneSource& source = *state.source();
    if (source.texture_set_id != destination_texture_set) {
        throw std::invalid_argument("clone source texture set '" + source.texture_set_id +
                                    "' differs from destination texture set '" +
                                    std::string(destination_texture_set) + "'");
    }
    return source;
}

}  // namespace

void CloneSourceState::set_source(std::string texture_set_id, Vec2d uv) {
    if (texture_set_id.empty() || !finite(uv)) {
        throw std::invalid_argument("clone source requires a texture set and finite UV");
    }
    source_ = CloneSource{.texture_set_id = std::move(texture_set_id), .uv = uv};
}

void CloneSourceState::clear_source() noexcept { source_.reset(); }

CloneResult apply_clone(const CachedSurfaceMaps& destination_surface, const ResolvedStroke& stroke,
                        const RejectedCoverageRaster& rejected,
                        const CloneSourceState& source_state,
                        std::span<const PaintToolChannelRaster> enabled_layer_snapshot,
                        std::span<const PaintToolChannelRaster> source_snapshot,
                        const CloneSettings& settings) {
    const std::size_t texel_count = checked_texel_count(destination_surface, rejected);
    validate_mode(settings.mode);
    if (!finite(settings.destination_anchor_uv)) {
        throw std::invalid_argument("clone destination anchor UV must be finite");
    }
    const CloneSource& source = checked_source(source_state, destination_surface.texture_set_id);
    validate_source_snapshot(source_snapshot, texel_count);
    std::vector<std::size_t> sample_indices =
        resolve_source_samples(destination_surface, source, settings);
    const RejectedCoverageRaster masked = apply_paint_masks(rejected, settings.masks);
    const RejectedCoverageRaster sampleable = restrict_to_valid_samples(masked, sample_indices);
    DepositionRaster deposition = evaluate_deposition(stroke, sampleable, settings.deposition_mode);
    const std::vector<PaintToolChannelRaster> sampled =
        sample_channels(source_snapshot, sample_indices);
    PaintToolShadeResult shaded = shade_paint_tool_channels(
        destination_surface.surface.width, destination_surface.surface.height,
        enabled_layer_snapshot, sampled, deposition.strength, settings.blend_mode);
    return {.width = destination_surface.surface.width,
            .height = destination_surface.surface.height,
            .mode = settings.mode,
            .source_anchor_uv = source.uv,
            .destination_anchor_uv = settings.destination_anchor_uv,
            .deposition = std::move(deposition),
            .source_sample_indices = std::move(sample_indices),
            .channels = std::move(shaded.channels),
            .applied_channel_ids = std::move(shaded.applied_channel_ids)};
}

}  // namespace ctex::paint
