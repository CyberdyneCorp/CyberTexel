#ifndef CTEX_IO_TEXTURE_ENCODE_HPP
#define CTEX_IO_TEXTURE_ENCODE_HPP

#include <cstdint>
#include <ctex/image/tiled_image.hpp>
#include <ctex/io/export_preset.hpp>
#include <span>
#include <stdexcept>
#include <string>
#include <vector>

namespace ctex::io {

enum class TextureEncodeErrorCode : std::uint8_t {
    unsupported_combination,
    invalid_option,
    invalid_pixels,
    over_limit,
    encode_failed,
};

class TextureEncodeError : public std::runtime_error {
public:
    TextureEncodeError(TextureEncodeErrorCode code, ExportImageFormat format,
                       ExportBitDepth bit_depth, std::string message);

    [[nodiscard]] TextureEncodeErrorCode code() const noexcept { return code_; }
    [[nodiscard]] ExportImageFormat format() const noexcept { return format_; }
    [[nodiscard]] ExportBitDepth bit_depth() const noexcept { return bit_depth_; }

private:
    TextureEncodeErrorCode code_;
    ExportImageFormat format_;
    ExportBitDepth bit_depth_;
};

struct TextureEncodeOptions {
    ExportImageFormat format{ExportImageFormat::png};
    ExportBitDepth bit_depth{ExportBitDepth::bits_8};
    image::ColorSpace color_space{image::ColorSpace::linear_rec709};
    int jpeg_quality{90};
};

[[nodiscard]] std::vector<std::byte> encode_texture_memory(const image::TiledImage& pixels,
                                                           const TextureEncodeOptions& options);

}  // namespace ctex::io

#endif
