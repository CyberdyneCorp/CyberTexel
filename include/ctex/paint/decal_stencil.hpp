#ifndef CTEX_PAINT_DECAL_STENCIL_HPP
#define CTEX_PAINT_DECAL_STENCIL_HPP

#include <cstddef>
#include <cstdint>
#include <ctex/paint/brush.hpp>
#include <ctex/paint/surface_cache.hpp>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace ctex::paint {

struct ToolOpacityImage {
    std::uint32_t width{};
    std::uint32_t height{};
    std::vector<double> opacity;
};

struct DecalMaterial {
    std::uint32_t width{};
    std::uint32_t height{};
    std::vector<PaintToolChannelRaster> channels;
    std::vector<double> opacity;
};

struct DecalTransform {
    double rotation_radians{};
    double uniform_scale{1.0};
    Vec2d axis_scale{1.0, 1.0};
};

struct DecalPlacement {
    Vec3d position;
    Vec3d surface_normal{0.0, 0.0, 1.0};
    DecalTransform transform;
};

struct DecalFrame {
    Vec3d origin;
    Vec3d tangent;
    Vec3d bitangent;
    Vec3d normal;
    Vec2d scale;
};

[[nodiscard]] DecalPlacement place_decal_on_surface(const CachedSurfaceMaps& surface,
                                                    std::size_t picked_texel,
                                                    DecalTransform transform = {});
[[nodiscard]] DecalFrame resolve_decal_frame(const DecalPlacement& placement);

struct EditableDecalEntry {
    std::string entry_id;
    std::string material_content_id;
    std::uint64_t revision{1};
    DecalPlacement placement;
    DecalMaterial pinned_material;
};

[[nodiscard]] EditableDecalEntry retain_editable_decal(std::string entry_id,
                                                       std::string material_content_id,
                                                       DecalPlacement placement,
                                                       DecalMaterial material);
[[nodiscard]] EditableDecalEntry edit_decal_transform(const EditableDecalEntry& entry,
                                                      DecalTransform transform);

struct DecalRasterSettings {
    std::string_view blend_mode{"normal"};
    PaintMaskInputs masks;
    std::optional<PaintMaskView> rejection_acceptance;
};

struct DecalRasterResult {
    std::uint32_t width{};
    std::uint32_t height{};
    std::uint64_t editable_revision{};
    DecalFrame frame;
    std::vector<std::size_t> source_sample_indices;
    std::vector<double> strength;
    std::vector<PaintToolChannelRaster> sampled_material;
    std::vector<PaintToolChannelRaster> channels;
    std::vector<std::string> applied_channel_ids;
};

inline constexpr std::size_t no_decal_sample = static_cast<std::size_t>(-1);

[[nodiscard]] DecalRasterResult rasterize_editable_decal(
    const CachedSurfaceMaps& surface,
    std::span<const PaintToolChannelRaster> enabled_layer_snapshot, const EditableDecalEntry& entry,
    const DecalRasterSettings& settings = {});

struct StencilTransform {
    Vec2d position;
    double rotation_radians{};
    Vec2d scale{1.0, 1.0};
};

struct StencilMaskResult {
    std::uint32_t width{};
    std::uint32_t height{};
    bool inverted{};
    std::vector<double> values;
};

[[nodiscard]] StencilMaskResult resolve_stencil_mask(std::uint32_t width, std::uint32_t height,
                                                     std::span<const Vec2d> screen_positions,
                                                     const ToolOpacityImage& image,
                                                     const StencilTransform& transform,
                                                     bool inverted = false);

struct StencilSettings {
    StencilTransform transform;
    ToolOpacityImage image;
    bool inverted{};
    DepositionMode deposition_mode{DepositionMode::non_building};
    std::string_view blend_mode{"normal"};
    PaintMaskInputs masks;
};

struct StencilResult {
    StencilMaskResult stencil_mask;
    DepositionRaster deposition;
    std::vector<PaintToolChannelRaster> channels;
    std::vector<std::string> applied_channel_ids;
};

[[nodiscard]] StencilResult apply_stencil(
    const ResolvedStroke& stroke, const RejectedCoverageRaster& rejected,
    std::span<const Vec2d> screen_positions,
    std::span<const PaintToolChannelRaster> enabled_layer_snapshot,
    std::span<const PaintToolChannelRaster> material, const StencilSettings& settings);

}  // namespace ctex::paint

#endif
