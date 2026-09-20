#ifndef CTEX_IO_IMAGE_IO_HPP
#define CTEX_IO_IMAGE_IO_HPP

#include <cstddef>
#include <cstdint>
#include <ctex/image/color_policy.hpp>
#include <ctex/image/tiled_image.hpp>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace ctex::io {

enum class ImageFileFormat { unknown, png, jpeg, bmp, tiff, openexr, radiance_hdr, psd, tga };

enum class ImageIoErrorCode {
    unsupported_format,
    malformed_input,
    over_limit,
    decode_failed,
    encode_failed,
    unsupported_pixel_format,
};

enum class ColorSpaceSource { caller, embedded_srgb, automatic_rule };

struct DecodeLimits {
    std::uint32_t maximum_width = 16'384;
    std::uint32_t maximum_height = 16'384;
    std::size_t maximum_decoded_bytes = 1ULL << 30;
};

struct DecodeRequest {
    std::span<const std::byte> bytes;
    std::string_view source_name;
    image::ChannelSemantic intended_channel = image::ChannelSemantic::base_color;
    image::InputColorSpace color_space = image::InputColorSpace::automatic;
    DecodeLimits limits{};
};

struct DecodeReport {
    ImageFileFormat detected_format;
    bool extension_mismatch;
    ColorSpaceSource color_space_source;
    std::vector<std::string> diagnostics;
};

struct DecodedImage {
    image::TiledImage pixels;
    image::ColorSpace source_color_space;
    DecodeReport report;
};

struct PngEncodeOptions {
    image::ColorSpace color_space = image::ColorSpace::linear_rec709;
};

class ImageIoError : public std::runtime_error {
public:
    ImageIoError(ImageIoErrorCode code, ImageFileFormat format, std::string message);

    [[nodiscard]] ImageIoErrorCode code() const noexcept { return code_; }
    [[nodiscard]] ImageFileFormat format() const noexcept { return format_; }

private:
    ImageIoErrorCode code_;
    ImageFileFormat format_;
};

[[nodiscard]] ImageFileFormat detect_image_format(std::span<const std::byte> bytes) noexcept;
[[nodiscard]] std::string_view image_file_format_name(ImageFileFormat format) noexcept;
[[nodiscard]] DecodedImage decode_image_memory(const DecodeRequest& request);
[[nodiscard]] std::vector<std::byte> encode_png_memory(const image::TiledImage& image,
                                                       PngEncodeOptions options = {});

}  // namespace ctex::io

#endif
