#ifndef CTEX_PAINT_SEAM_FILTER_HPP
#define CTEX_PAINT_SEAM_FILTER_HPP

#include <cstddef>
#include <cstdint>
#include <ctex/paint/seam_dilation.hpp>
#include <ctex/paint/stroke.hpp>
#include <span>
#include <vector>

namespace ctex::paint {

enum class SurfaceFilterOperation : std::uint8_t {
    blur,
    smear,
    derivative,
    mip_generation,
};

struct SamplingFootprint {
    std::uint32_t radius_x{};
    std::uint32_t radius_y{};
    friend constexpr bool operator==(SamplingFootprint, SamplingFootprint) noexcept = default;
};

struct SurfaceAdjacentSample {
    std::size_t texel_index{};
    StrokeFrame tangent_frame;
    std::int32_t offset_x{};
    std::int32_t offset_y{};
    double weight{};
};

struct SurfaceAdjacencyLink {
    std::size_t first_texel{};
    StrokeFrame first_frame;
    std::size_t second_texel{};
    StrokeFrame second_frame;
};

[[nodiscard]] SurfaceAdjacentSample sample_across_surface_adjacency(
    const SurfaceAdjacencyLink& link, std::size_t source_texel, std::int32_t offset_x,
    std::int32_t offset_y, double weight);

struct SurfaceFilterRequest {
    SurfaceFilterOperation operation{};
    SamplingFootprint footprint;
    StrokeFrame output_frame;
    std::span<const SurfaceAdjacentSample> samples;
};

[[nodiscard]] double filter_surface_scalar(std::span<const double> values,
                                           const SurfaceFilterRequest& request);
[[nodiscard]] Vec3d filter_surface_tangent_vector(std::span<const Vec3d> values,
                                                  const SurfaceFilterRequest& request);

struct UnsupportedMipLevel {
    std::uint32_t mip_level{};
    std::uint32_t required_gutter_radius{};
    std::vector<std::uint32_t> affected_islands;
    friend bool operator==(const UnsupportedMipLevel&, const UnsupportedMipLevel&) = default;
};

struct IslandPaddingPlan {
    std::uint32_t width{};
    std::uint32_t height{};
    std::uint32_t requested_mip_levels{};
    std::uint32_t padding_radius{};
    std::vector<std::uint32_t> ownership;
    std::vector<UnsupportedMipLevel> unsupported_mip_levels;
};

[[nodiscard]] IslandPaddingPlan plan_island_padding(std::uint32_t width, std::uint32_t height,
                                                    std::span<const std::uint32_t> island_identity,
                                                    SamplingFootprint footprint,
                                                    std::uint32_t requested_mip_levels);

struct IslandPaddingResult {
    SeamDilationRaster raster;
    std::size_t padded_texel_count{};
};

[[nodiscard]] IslandPaddingResult apply_island_padding(
    const SeamDilationRaster& source, std::span<const std::uint32_t> island_identity,
    const IslandPaddingPlan& plan);

}  // namespace ctex::paint

#endif
