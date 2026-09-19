#ifndef CTEX_PAINT_PROJECTION_HPP
#define CTEX_PAINT_PROJECTION_HPP

#include <array>
#include <cstddef>
#include <cstdint>
#include <ctex/paint/decal_stencil.hpp>
#include <ctex/pick/ray.hpp>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace ctex::paint {

inline constexpr ToolParameterDescriptor projection_planar_extent_x_parameter{
    "projection.planar.extent.x", 1.0, stroke_position_tolerance, maximum_tool_transform_extent};
inline constexpr ToolParameterDescriptor projection_planar_extent_y_parameter{
    "projection.planar.extent.y", 1.0, stroke_position_tolerance, maximum_tool_transform_extent};
inline constexpr ToolParameterDescriptor projection_triplanar_scale_parameter{
    "projection.triplanar.scale", 1.0, stroke_position_tolerance, maximum_tool_transform_extent};
inline constexpr ToolParameterDescriptor projection_triplanar_offset_x_parameter{
    "projection.triplanar.offset.x", 0.0, -1.0, 1.0};
inline constexpr ToolParameterDescriptor projection_triplanar_offset_y_parameter{
    "projection.triplanar.offset.y", 0.0, -1.0, 1.0};

struct CameraProjection {
    pick::Mat4f view_projection;
    PaintMaskView visible_surface;
};

struct PlanarProjection {
    PlanarProjectionFrame frame;
    Vec2d extent{projection_planar_extent_x_parameter.default_value,
                 projection_planar_extent_y_parameter.default_value};
};

struct TriplanarProjection {
    double scale{projection_triplanar_scale_parameter.default_value};
    Vec2d offset{projection_triplanar_offset_x_parameter.default_value,
                 projection_triplanar_offset_y_parameter.default_value};
};

using ProjectionMapping = std::variant<CameraProjection, PlanarProjection, TriplanarProjection>;

struct ProjectionSettings {
    ProjectionMapping mapping{PlanarProjection{}};
    std::string_view blend_mode{"normal"};
    PaintMaskInputs masks{};
    std::optional<PaintMaskView> rejection_acceptance{};
};

inline constexpr std::size_t no_projection_sample = static_cast<std::size_t>(-1);

struct ProjectionSample {
    std::array<std::size_t, 3> source_indices{no_projection_sample, no_projection_sample,
                                              no_projection_sample};
    std::array<double, 3> weights{};
    std::size_t count{};
};

struct ProjectionResult {
    std::uint32_t width{};
    std::uint32_t height{};
    std::vector<ProjectionSample> samples;
    std::vector<double> strength;
    std::vector<PaintToolChannelRaster> sampled_material;
    std::vector<PaintToolChannelRaster> channels;
    std::vector<std::string> applied_channel_ids;
    ToolParameterReport parameter_report;
};

[[nodiscard]] ProjectionResult apply_projection(
    const CachedSurfaceMaps& surface,
    std::span<const PaintToolChannelRaster> enabled_layer_snapshot, const DecalMaterial& material,
    const ProjectionSettings& settings);

}  // namespace ctex::paint

#endif
