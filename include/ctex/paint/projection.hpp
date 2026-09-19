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

struct CameraProjection {
    pick::Mat4f view_projection;
    PaintMaskView visible_surface;
};

struct PlanarProjection {
    PlanarProjectionFrame frame;
    Vec2d extent{1.0, 1.0};
};

struct TriplanarProjection {
    double scale{1.0};
    Vec2d offset;
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
};

[[nodiscard]] ProjectionResult apply_projection(
    const CachedSurfaceMaps& surface,
    std::span<const PaintToolChannelRaster> enabled_layer_snapshot, const DecalMaterial& material,
    const ProjectionSettings& settings);

}  // namespace ctex::paint

#endif
