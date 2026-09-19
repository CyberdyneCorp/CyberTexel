#ifndef CTEX_IMAGE_SEAM_DILATION_HPP
#define CTEX_IMAGE_SEAM_DILATION_HPP

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace ctex::image {

inline constexpr std::uint32_t default_uv_dilation_radius = 2;
inline constexpr std::uint32_t maximum_uv_dilation_radius = 4'096;

struct SeamDilationRaster {
    std::uint32_t width{};
    std::uint32_t height{};
    std::uint8_t component_count{};
    std::vector<double> pixels;
};

struct SeamDilationPixels {
    SeamDilationRaster raster;
    std::size_t dilated_texel_count{};
    std::size_t zero_gradient_texel_count{};
};

[[nodiscard]] SeamDilationPixels extrapolate_uv_seams(
    const SeamDilationRaster& source, std::span<const std::uint8_t> coverage,
    std::uint32_t radius = default_uv_dilation_radius);

}  // namespace ctex::image

#endif
