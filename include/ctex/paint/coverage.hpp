#ifndef CTEX_PAINT_COVERAGE_HPP
#define CTEX_PAINT_COVERAGE_HPP

#include <array>
#include <cstddef>
#include <cstdint>
#include <ctex/paint/stroke.hpp>
#include <limits>
#include <span>
#include <vector>

namespace ctex::paint {

inline constexpr std::uint32_t no_surface_triangle = std::numeric_limits<std::uint32_t>::max();

struct TextureSpaceMeshView {
    std::span<const Vec3d> positions;
    std::span<const Vec3d> normals;
    std::span<const Vec2d> uv;
    std::span<const std::uint32_t> triangle_indices;
};

struct TextureSpaceRasterRequest {
    std::uint32_t width{};
    std::uint32_t height{};
    Vec2d tile_origin{};
};

struct SurfaceTexel {
    Vec3d position;
    Vec3d normal;
    Vec2d uv;
    std::uint32_t triangle{no_surface_triangle};
};

struct TextureSpaceRaster {
    std::uint32_t width{};
    std::uint32_t height{};
    Vec2d tile_origin{};
    std::vector<SurfaceTexel> texels;

    [[nodiscard]] bool covered(std::size_t index) const;
};

[[nodiscard]] TextureSpaceRaster rasterize_texture_space(TextureSpaceMeshView mesh,
                                                         TextureSpaceRasterRequest request);

struct CoverageRaster {
    std::uint32_t width{};
    std::uint32_t height{};
    std::vector<double> values;
};

[[nodiscard]] double brush_falloff(double normalized_distance, double hardness);
[[nodiscard]] CoverageRaster evaluate_stroke_coverage(const TextureSpaceRaster& surface,
                                                      const ResolvedStroke& stroke);

enum class MaterialCoordinateMode : std::uint8_t { uv, triplanar, planar };

struct PlanarProjectionFrame {
    Vec3d origin;
    Vec3d u_axis{1.0, 0.0, 0.0};
    Vec3d v_axis{0.0, 1.0, 0.0};
};

struct MaterialCoordinateRequest {
    MaterialCoordinateMode mode{MaterialCoordinateMode::uv};
    PlanarProjectionFrame planar;
};

struct MaterialProjection {
    Vec2d coordinate;
    double weight{};
};

struct MaterialCoordinates {
    std::array<MaterialProjection, 3> projections{};
    std::size_t count{};
};

[[nodiscard]] MaterialCoordinates material_coordinates(const SurfaceTexel& texel,
                                                       const MaterialCoordinateRequest& request);

}  // namespace ctex::paint

#endif
