#ifndef CTEX_PAINT_DECAL_STENCIL_HPP
#define CTEX_PAINT_DECAL_STENCIL_HPP

#include <cstddef>
#include <cstdint>
#include <ctex/paint/brush.hpp>
#include <ctex/paint/parameters.hpp>
#include <ctex/paint/surface_cache.hpp>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace ctex::paint {

inline constexpr double maximum_tool_transform_extent = 1'000'000.0;
inline constexpr ToolParameterDescriptor decal_rotation_parameter{"decal.rotation_radians", 0.0,
                                                                  -maximum_stroke_rotation_radians,
                                                                  maximum_stroke_rotation_radians};
inline constexpr ToolParameterDescriptor decal_uniform_scale_parameter{
    "decal.uniform_scale", 1.0, stroke_position_tolerance, maximum_tool_transform_extent};
inline constexpr ToolParameterDescriptor decal_axis_scale_x_parameter{
    "decal.axis_scale.x", 1.0, stroke_position_tolerance, maximum_tool_transform_extent};
inline constexpr ToolParameterDescriptor decal_axis_scale_y_parameter{
    "decal.axis_scale.y", 1.0, stroke_position_tolerance, maximum_tool_transform_extent};
inline constexpr ToolParameterDescriptor stencil_position_x_parameter{
    "stencil.position.x", 0.0, -maximum_tool_transform_extent, maximum_tool_transform_extent};
inline constexpr ToolParameterDescriptor stencil_position_y_parameter{
    "stencil.position.y", 0.0, -maximum_tool_transform_extent, maximum_tool_transform_extent};
inline constexpr ToolParameterDescriptor stencil_rotation_parameter{
    "stencil.rotation_radians", 0.0, -maximum_stroke_rotation_radians,
    maximum_stroke_rotation_radians};
inline constexpr ToolParameterDescriptor stencil_scale_x_parameter{
    "stencil.scale.x", 1.0, stroke_position_tolerance, maximum_tool_transform_extent};
inline constexpr ToolParameterDescriptor stencil_scale_y_parameter{
    "stencil.scale.y", 1.0, stroke_position_tolerance, maximum_tool_transform_extent};

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
    double rotation_radians{decal_rotation_parameter.default_value};
    double uniform_scale{decal_uniform_scale_parameter.default_value};
    Vec2d axis_scale{decal_axis_scale_x_parameter.default_value,
                     decal_axis_scale_y_parameter.default_value};
    friend constexpr bool operator==(DecalTransform, DecalTransform) noexcept = default;
};

struct DecalPlacement {
    Vec3d position;
    Vec3d surface_normal{0.0, 0.0, 1.0};
    DecalTransform transform;
    ToolParameterReport parameter_report;
};

struct DecalFrame {
    Vec3d origin;
    Vec3d tangent;
    Vec3d bitangent;
    Vec3d normal;
    Vec2d scale;
    DecalTransform resolved_transform;
    ToolParameterReport parameter_report;
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
    ToolParameterReport parameter_report;
};

inline constexpr std::size_t no_decal_sample = static_cast<std::size_t>(-1);

[[nodiscard]] DecalRasterResult rasterize_decal(
    const CachedSurfaceMaps& surface,
    std::span<const PaintToolChannelRaster> enabled_layer_snapshot, const DecalPlacement& placement,
    const DecalMaterial& material, const DecalRasterSettings& settings = {});

[[nodiscard]] DecalRasterResult rasterize_editable_decal(
    const CachedSurfaceMaps& surface,
    std::span<const PaintToolChannelRaster> enabled_layer_snapshot, const EditableDecalEntry& entry,
    const DecalRasterSettings& settings = {});

struct StencilTransform {
    Vec2d position{stencil_position_x_parameter.default_value,
                   stencil_position_y_parameter.default_value};
    double rotation_radians{stencil_rotation_parameter.default_value};
    Vec2d scale{stencil_scale_x_parameter.default_value, stencil_scale_y_parameter.default_value};
    friend constexpr bool operator==(StencilTransform, StencilTransform) noexcept = default;
};

struct StencilMaskResult {
    std::uint32_t width{};
    std::uint32_t height{};
    bool inverted{};
    std::vector<double> values;
    StencilTransform resolved_transform;
    ToolParameterReport parameter_report;
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
