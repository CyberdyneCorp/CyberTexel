#include <algorithm>
#include <cmath>
#include <ctex/paint/decal_stencil.hpp>
#include <limits>
#include <set>
#include <stdexcept>
#include <string>
#include <utility>

namespace ctex::paint {
namespace {

constexpr double vector_epsilon = 1.0e-12;

bool finite(Vec2d value) { return std::isfinite(value.x) && std::isfinite(value.y); }

bool finite(Vec3d value) {
    return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z);
}

bool finite(graph::ColourValue value) {
    return std::isfinite(value.r) && std::isfinite(value.g) && std::isfinite(value.b) &&
           std::isfinite(value.a);
}

double dot(Vec3d left, Vec3d right) {
    return left.x * right.x + left.y * right.y + left.z * right.z;
}

Vec3d cross(Vec3d left, Vec3d right) {
    return {left.y * right.z - left.z * right.y, left.z * right.x - left.x * right.z,
            left.x * right.y - left.y * right.x};
}

Vec3d subtract(Vec3d left, Vec3d right) {
    return {left.x - right.x, left.y - right.y, left.z - right.z};
}

Vec3d normalized(Vec3d value, std::string_view role) {
    const double length = std::sqrt(dot(value, value));
    if (!finite(value) || !std::isfinite(length) || length <= vector_epsilon) {
        throw std::invalid_argument(std::string(role) + " must be a finite non-zero vector");
    }
    return {value.x / length, value.y / length, value.z / length};
}

std::size_t checked_area(std::uint32_t width, std::uint32_t height, std::string_view role) {
    if (width == 0 || height == 0 ||
        static_cast<std::size_t>(width) > std::numeric_limits<std::size_t>::max() / height) {
        throw std::invalid_argument(std::string(role) + " dimensions are invalid");
    }
    return static_cast<std::size_t>(width) * height;
}

void validate_opacity(std::span<const double> opacity, std::size_t expected,
                      std::string_view role) {
    if (opacity.size() != expected ||
        !std::all_of(opacity.begin(), opacity.end(), [](double value) {
            return std::isfinite(value) && value >= 0.0 && value <= 1.0;
        })) {
        throw std::invalid_argument(std::string(role) + " opacity is invalid");
    }
}

void validate_material(const DecalMaterial& material) {
    const std::size_t pixel_count = checked_area(material.width, material.height, "decal material");
    validate_opacity(material.opacity, pixel_count, "decal material");
    if (material.channels.empty()) {
        throw std::invalid_argument("decal material requires at least one channel");
    }
    std::set<std::string_view> identifiers;
    for (const PaintToolChannelRaster& channel : material.channels) {
        if (channel.semantic_id.empty() || channel.component_count == 0 ||
            channel.component_count > 4 || channel.pixels.size() != pixel_count ||
            !identifiers.insert(channel.semantic_id).second ||
            !std::all_of(channel.pixels.begin(), channel.pixels.end(),
                         [](graph::ColourValue value) { return finite(value); })) {
            throw std::invalid_argument("decal material channels are invalid or duplicated");
        }
    }
}

void validate_transform(const DecalTransform& transform) {
    if (!std::isfinite(transform.rotation_radians) || !std::isfinite(transform.uniform_scale) ||
        transform.uniform_scale <= 0.0 || !finite(transform.axis_scale) ||
        transform.axis_scale.x <= 0.0 || transform.axis_scale.y <= 0.0) {
        throw std::invalid_argument("decal transform is invalid");
    }
}

std::size_t checked_surface(const CachedSurfaceMaps& maps) {
    const std::size_t count =
        checked_area(maps.surface.width, maps.surface.height, "decal surface");
    if (maps.surface.texels.size() != count || maps.coverage.size() != count) {
        throw std::invalid_argument("decal surface maps have inconsistent dimensions");
    }
    for (std::size_t texel = 0; texel < count; ++texel) {
        if (maps.coverage[texel] > 1 ||
            (maps.coverage[texel] != 0) != maps.surface.covered(texel)) {
            throw std::invalid_argument("decal surface coverage is invalid");
        }
    }
    return count;
}

std::size_t image_index(std::uint32_t width, std::uint32_t height, Vec2d uv) {
    if (!finite(uv) || uv.x < 0.0 || uv.x >= 1.0 || uv.y < 0.0 || uv.y >= 1.0) {
        return no_decal_sample;
    }
    const auto x = static_cast<std::uint32_t>(std::floor(uv.x * width));
    const auto bottom_up_y = static_cast<std::uint32_t>(std::floor(uv.y * height));
    const std::uint32_t y = height - 1 - bottom_up_y;
    return static_cast<std::size_t>(y) * width + x;
}

Vec2d decal_coordinate(Vec3d position, const DecalFrame& frame) {
    const Vec3d delta = subtract(position, frame.origin);
    return {.x = dot(delta, frame.tangent) / frame.scale.x + 0.5,
            .y = dot(delta, frame.bitangent) / frame.scale.y + 0.5};
}

std::vector<std::size_t> decal_samples(const CachedSurfaceMaps& surface, const DecalFrame& frame,
                                       const DecalMaterial& material) {
    std::vector<std::size_t> result(surface.surface.texels.size(), no_decal_sample);
    for (std::size_t texel = 0; texel < result.size(); ++texel) {
        if (surface.coverage[texel] != 0) {
            result[texel] =
                image_index(material.width, material.height,
                            decal_coordinate(surface.surface.texels[texel].position, frame));
        }
    }
    return result;
}

std::vector<PaintToolChannelRaster> sample_material(const DecalMaterial& material,
                                                    std::span<const std::size_t> sample_indices) {
    std::vector<PaintToolChannelRaster> sampled;
    sampled.reserve(material.channels.size());
    for (const PaintToolChannelRaster& source : material.channels) {
        PaintToolChannelRaster destination{
            .semantic_id = source.semantic_id,
            .component_count = source.component_count,
            .pixels = std::vector<graph::ColourValue>(sample_indices.size())};
        for (std::size_t texel = 0; texel < sample_indices.size(); ++texel) {
            if (sample_indices[texel] != no_decal_sample) {
                destination.pixels[texel] = source.pixels[sample_indices[texel]];
            }
        }
        sampled.push_back(std::move(destination));
    }
    return sampled;
}

std::vector<double> decal_strength(const DecalMaterial& material,
                                   std::span<const std::size_t> sample_indices) {
    std::vector<double> result(sample_indices.size(), 0.0);
    for (std::size_t texel = 0; texel < sample_indices.size(); ++texel) {
        if (sample_indices[texel] != no_decal_sample) {
            result[texel] = material.opacity[sample_indices[texel]];
        }
    }
    return result;
}

void multiply_mask(std::span<double> values, PaintMaskView mask, std::string_view role) {
    if (mask.values.size() != values.size()) {
        throw std::invalid_argument(std::string(role) + " dimensions are inconsistent");
    }
    for (std::size_t texel = 0; texel < values.size(); ++texel) {
        const double mask_value = mask.values[texel];
        if (!std::isfinite(mask_value) || mask_value < 0.0 || mask_value > 1.0) {
            throw std::invalid_argument(std::string(role) + " values are invalid");
        }
        values[texel] *= mask_value;
    }
}

Vec2d screen_to_image(Vec2d screen, const StencilTransform& transform) {
    const double cosine = std::cos(transform.rotation_radians);
    const double sine = std::sin(transform.rotation_radians);
    const double x = screen.x - transform.position.x;
    const double y = screen.y - transform.position.y;
    return {.x = (cosine * x + sine * y) / transform.scale.x + 0.5,
            .y = (-sine * x + cosine * y) / transform.scale.y + 0.5};
}

void validate_stencil_transform(const StencilTransform& transform) {
    if (!finite(transform.position) || !std::isfinite(transform.rotation_radians) ||
        !finite(transform.scale) || transform.scale.x <= 0.0 || transform.scale.y <= 0.0) {
        throw std::invalid_argument("stencil transform is invalid");
    }
}

RejectedCoverageRaster apply_stencil_mask(const RejectedCoverageRaster& rejected,
                                          std::span<const double> mask) {
    RejectedCoverageRaster result = rejected;
    std::fill(result.coverage.values.begin(), result.coverage.values.end(), 0.0);
    for (RejectedStampCoverage& event : result.stamp_events) {
        for (std::size_t texel = 0; texel < mask.size(); ++texel) {
            event.values[texel] *= mask[texel];
            result.coverage.values[texel] =
                std::max(result.coverage.values[texel], event.values[texel]);
        }
    }
    return result;
}

}  // namespace

DecalPlacement place_decal_on_surface(const CachedSurfaceMaps& surface, std::size_t picked_texel,
                                      DecalTransform transform) {
    const std::size_t texel_count = checked_surface(surface);
    if (picked_texel >= texel_count || surface.coverage[picked_texel] == 0) {
        throw std::invalid_argument("decal placement requires a covered picked texel");
    }
    const SurfaceTexel& picked = surface.surface.texels[picked_texel];
    DecalPlacement placement{
        .position = picked.position, .surface_normal = picked.normal, .transform = transform};
    static_cast<void>(resolve_decal_frame(placement));
    return placement;
}

DecalFrame resolve_decal_frame(const DecalPlacement& placement) {
    validate_transform(placement.transform);
    if (!finite(placement.position)) {
        throw std::invalid_argument("decal position must be finite");
    }
    const Vec3d normal = normalized(placement.surface_normal, "decal surface normal");
    const Vec3d reference = std::abs(normal.z) < 0.9 ? Vec3d{0.0, 0.0, 1.0} : Vec3d{0.0, 1.0, 0.0};
    const Vec3d unrotated_tangent = normalized(cross(reference, normal), "decal tangent");
    const Vec3d unrotated_bitangent = cross(normal, unrotated_tangent);
    const double cosine = std::cos(placement.transform.rotation_radians);
    const double sine = std::sin(placement.transform.rotation_radians);
    return {.origin = placement.position,
            .tangent = {unrotated_tangent.x * cosine + unrotated_bitangent.x * sine,
                        unrotated_tangent.y * cosine + unrotated_bitangent.y * sine,
                        unrotated_tangent.z * cosine + unrotated_bitangent.z * sine},
            .bitangent = {-unrotated_tangent.x * sine + unrotated_bitangent.x * cosine,
                          -unrotated_tangent.y * sine + unrotated_bitangent.y * cosine,
                          -unrotated_tangent.z * sine + unrotated_bitangent.z * cosine},
            .normal = normal,
            .scale = {placement.transform.uniform_scale * placement.transform.axis_scale.x,
                      placement.transform.uniform_scale * placement.transform.axis_scale.y}};
}

EditableDecalEntry retain_editable_decal(std::string entry_id, std::string material_content_id,
                                         DecalPlacement placement, DecalMaterial material) {
    if (entry_id.empty() || material_content_id.empty() || !finite(placement.position)) {
        throw std::invalid_argument("editable decal identities and position are required");
    }
    static_cast<void>(resolve_decal_frame(placement));
    validate_material(material);
    return {.entry_id = std::move(entry_id),
            .material_content_id = std::move(material_content_id),
            .revision = 1,
            .placement = placement,
            .pinned_material = std::move(material)};
}

EditableDecalEntry edit_decal_transform(const EditableDecalEntry& entry, DecalTransform transform) {
    validate_transform(transform);
    if (entry.revision == std::numeric_limits<std::uint64_t>::max()) {
        throw std::overflow_error("editable decal revision exhausted");
    }
    EditableDecalEntry result = entry;
    result.placement.transform = transform;
    ++result.revision;
    return result;
}

DecalRasterResult rasterize_decal(const CachedSurfaceMaps& surface,
                                  std::span<const PaintToolChannelRaster> enabled_layer_snapshot,
                                  const DecalPlacement& placement, const DecalMaterial& material,
                                  const DecalRasterSettings& settings) {
    static_cast<void>(checked_surface(surface));
    validate_material(material);
    const DecalFrame frame = resolve_decal_frame(placement);
    std::vector<std::size_t> sample_indices = decal_samples(surface, frame, material);
    std::vector<double> strength = decal_strength(material, sample_indices);
    const CombinedPaintMask masks =
        combine_paint_masks(surface.surface.width, surface.surface.height, settings.masks);
    multiply_mask(strength, PaintMaskView{masks.values}, "decal masks");
    if (settings.rejection_acceptance) {
        multiply_mask(strength, *settings.rejection_acceptance, "decal rejection acceptance");
    }
    std::vector<PaintToolChannelRaster> sampled = sample_material(material, sample_indices);
    PaintToolShadeResult shaded =
        shade_paint_tool_channels(surface.surface.width, surface.surface.height,
                                  enabled_layer_snapshot, sampled, strength, settings.blend_mode);
    return {.width = surface.surface.width,
            .height = surface.surface.height,
            .editable_revision = 0,
            .frame = frame,
            .source_sample_indices = std::move(sample_indices),
            .strength = std::move(strength),
            .sampled_material = std::move(sampled),
            .channels = std::move(shaded.channels),
            .applied_channel_ids = std::move(shaded.applied_channel_ids)};
}

DecalRasterResult rasterize_editable_decal(
    const CachedSurfaceMaps& surface,
    std::span<const PaintToolChannelRaster> enabled_layer_snapshot, const EditableDecalEntry& entry,
    const DecalRasterSettings& settings) {
    if (entry.entry_id.empty() || entry.material_content_id.empty() || entry.revision == 0) {
        throw std::invalid_argument("editable decal entry is invalid");
    }
    DecalRasterResult result = rasterize_decal(surface, enabled_layer_snapshot, entry.placement,
                                               entry.pinned_material, settings);
    result.editable_revision = entry.revision;
    return result;
}

StencilMaskResult resolve_stencil_mask(std::uint32_t width, std::uint32_t height,
                                       std::span<const Vec2d> screen_positions,
                                       const ToolOpacityImage& image,
                                       const StencilTransform& transform, bool inverted) {
    const std::size_t texel_count = checked_area(width, height, "stencil destination");
    const std::size_t image_pixels = checked_area(image.width, image.height, "stencil image");
    validate_opacity(image.opacity, image_pixels, "stencil image");
    validate_stencil_transform(transform);
    if (screen_positions.size() != texel_count) {
        throw std::invalid_argument("stencil requires one screen position per texel");
    }
    StencilMaskResult result{.width = width,
                             .height = height,
                             .inverted = inverted,
                             .values = std::vector<double>(texel_count, 0.0)};
    for (std::size_t texel = 0; texel < texel_count; ++texel) {
        if (!finite(screen_positions[texel])) {
            throw std::invalid_argument("stencil screen position is not finite");
        }
        const std::size_t source = image_index(image.width, image.height,
                                               screen_to_image(screen_positions[texel], transform));
        const double opacity = source == no_decal_sample ? 0.0 : image.opacity[source];
        result.values[texel] = inverted ? 1.0 - opacity : opacity;
    }
    return result;
}

StencilResult apply_stencil(const ResolvedStroke& stroke, const RejectedCoverageRaster& rejected,
                            std::span<const Vec2d> screen_positions,
                            std::span<const PaintToolChannelRaster> enabled_layer_snapshot,
                            std::span<const PaintToolChannelRaster> material,
                            const StencilSettings& settings) {
    StencilMaskResult stencil =
        resolve_stencil_mask(rejected.coverage.width, rejected.coverage.height, screen_positions,
                             settings.image, settings.transform, settings.inverted);
    const RejectedCoverageRaster masked = apply_paint_masks(rejected, settings.masks);
    const RejectedCoverageRaster constrained = apply_stencil_mask(masked, stencil.values);
    DepositionRaster deposition =
        evaluate_deposition(stroke, constrained, settings.deposition_mode);
    PaintToolShadeResult shaded = shade_paint_tool_channels(
        rejected.coverage.width, rejected.coverage.height, enabled_layer_snapshot, material,
        deposition.strength, settings.blend_mode);
    return {.stencil_mask = std::move(stencil),
            .deposition = std::move(deposition),
            .channels = std::move(shaded.channels),
            .applied_channel_ids = std::move(shaded.applied_channel_ids)};
}

}  // namespace ctex::paint
