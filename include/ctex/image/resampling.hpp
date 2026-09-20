#ifndef CTEX_IMAGE_RESAMPLING_HPP
#define CTEX_IMAGE_RESAMPLING_HPP

#include <cstddef>
#include <cstdint>
#include <ctex/image/pixel_format.hpp>
#include <span>
#include <vector>

namespace ctex::image {

inline constexpr std::size_t default_resample_byte_limit = 1ULL << 30;

enum class ImageResampleFilter : std::uint8_t { nearest, bilinear };
inline constexpr ImageResampleFilter default_image_resample_filter = ImageResampleFilter::bilinear;

struct ImageResampleRequest {
    std::span<const std::byte> pixels;
    std::uint32_t source_width{};
    std::uint32_t source_height{};
    PixelFormat format{};
    std::size_t source_row_stride_bytes{};
    std::uint32_t output_width{};
    std::uint32_t output_height{};
    ImageResampleFilter filter{default_image_resample_filter};
    std::size_t maximum_output_bytes{default_resample_byte_limit};
};

struct ImageResampleResult {
    std::uint32_t width{};
    std::uint32_t height{};
    PixelFormat format{};
    ImageResampleFilter filter{};
    std::vector<std::byte> pixels;
};

[[nodiscard]] ImageResampleResult resample_image(const ImageResampleRequest& request);

}  // namespace ctex::image

#endif
