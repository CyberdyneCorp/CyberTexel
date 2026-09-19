#include <lodepng.h>

#include <algorithm>
#include <array>
#include <bit>
#include <cctype>
#include <cstdlib>
#include <cstring>
#include <ctex/io/container_version.hpp>
#include <ctex/io/image_io.hpp>
#include <limits>
#include <memory>
#include <sstream>
#include <utility>

namespace ctex::io {
namespace {

using LodePngBuffer = std::unique_ptr<unsigned char, decltype(&std::free)>;

class LodePngState {
public:
    LodePngState() { lodepng_state_init(&state_); }
    ~LodePngState() { lodepng_state_cleanup(&state_); }

    LodePngState(const LodePngState&) = delete;
    LodePngState& operator=(const LodePngState&) = delete;

    [[nodiscard]] LodePNGState* get() noexcept { return &state_; }
    [[nodiscard]] const LodePNGState* get() const noexcept { return &state_; }

private:
    LodePNGState state_{};
};

struct RawLayout {
    LodePNGColorType color_type;
    image::PixelFormat pixel_format;
};

std::size_t checked_multiply(std::size_t left, std::size_t right) {
    if (right != 0 && left > std::numeric_limits<std::size_t>::max() / right) {
        throw ImageIoError(ImageIoErrorCode::over_limit, ImageFileFormat::png,
                           "PNG decoded byte count overflows the platform size type");
    }
    return left * right;
}

bool starts_with(std::span<const std::byte> bytes, std::span<const std::uint8_t> signature) {
    if (bytes.size() < signature.size()) {
        return false;
    }
    return std::equal(signature.begin(), signature.end(), bytes.begin(),
                      [](std::uint8_t expected, std::byte actual) {
                          return expected == std::to_integer<unsigned>(actual);
                      });
}

std::string lowercase_extension(std::string_view source_name) {
    const std::size_t dot = source_name.find_last_of('.');
    if (dot == std::string_view::npos) {
        return {};
    }
    std::string extension(source_name.substr(dot));
    std::transform(
        extension.begin(), extension.end(), extension.begin(),
        [](unsigned char character) { return static_cast<char>(std::tolower(character)); });
    return extension;
}

RawLayout choose_raw_layout(const LodePNGColorMode& source) {
    const unsigned bit_depth = source.bitdepth < 8 ? 8 : source.bitdepth;
    const auto channel_type =
        bit_depth == 16 ? image::ChannelType::uint16_unorm : image::ChannelType::uint8_unorm;
    const bool has_alpha = lodepng_can_have_alpha(&source) != 0;
    switch (source.colortype) {
        case LCT_GREY:
            return {
                has_alpha ? LCT_GREY_ALPHA : LCT_GREY,
                {channel_type, static_cast<std::uint8_t>(has_alpha ? 2 : 1)},
            };
        case LCT_RGB:
            return {
                has_alpha ? LCT_RGBA : LCT_RGB,
                {channel_type, static_cast<std::uint8_t>(has_alpha ? 4 : 3)},
            };
        case LCT_GREY_ALPHA:
            return {LCT_GREY_ALPHA, {channel_type, 2}};
        case LCT_RGBA:
            return {LCT_RGBA, {channel_type, 4}};
        case LCT_PALETTE:
            return {LCT_RGBA, {image::ChannelType::uint8_unorm, 4}};
        default:
            throw ImageIoError(ImageIoErrorCode::unsupported_pixel_format, ImageFileFormat::png,
                               "PNG declares an unsupported color type");
    }
}

void enforce_limits(std::uint32_t width, std::uint32_t height, const image::PixelFormat& format,
                    const DecodeLimits& limits) {
    const std::size_t pixels = checked_multiply(width, height);
    const std::size_t decoded_bytes = checked_multiply(pixels, format.bytes_per_pixel());
    if (width > limits.maximum_width || height > limits.maximum_height ||
        decoded_bytes > limits.maximum_decoded_bytes) {
        std::ostringstream message;
        message << "PNG dimensions " << width << 'x' << height << " require " << decoded_bytes
                << " decoded bytes; limits are " << limits.maximum_width << 'x'
                << limits.maximum_height << " and " << limits.maximum_decoded_bytes << " bytes";
        throw ImageIoError(ImageIoErrorCode::over_limit, ImageFileFormat::png, message.str());
    }
}

std::vector<std::byte> native_pixel(const unsigned char* source, const image::PixelFormat& format) {
    std::vector<std::byte> pixel(format.bytes_per_pixel());
    std::memcpy(pixel.data(), source, pixel.size());
    if (format.channel_type == image::ChannelType::uint16_unorm &&
        std::endian::native == std::endian::little) {
        for (std::size_t offset = 0; offset < pixel.size(); offset += 2) {
            std::swap(pixel[offset], pixel[offset + 1]);
        }
    }
    return pixel;
}

image::TiledImage unpack_image(const unsigned char* decoded, std::uint32_t width,
                               std::uint32_t height, image::PixelFormat format) {
    image::TiledImage result(width, height, format);
    const std::size_t stride = format.bytes_per_pixel();
    for (std::uint32_t y = 0; y < height; ++y) {
        for (std::uint32_t x = 0; x < width; ++x) {
            const std::size_t offset = (static_cast<std::size_t>(y) * width + x) * stride;
            const std::vector<std::byte> pixel = native_pixel(decoded + offset, format);
            result.write_pixel(x, y, pixel);
        }
    }
    result.clear_dirty();
    return result;
}

std::pair<image::ColorSpace, ColorSpaceSource> resolve_color_space(
    const DecodeRequest& request, const LodePNGInfo& png_info,
    std::vector<std::string>& diagnostics) {
    if (request.color_space != image::InputColorSpace::automatic) {
        const auto resolved =
            image::resolve_input_space(request.color_space, request.intended_channel);
        return {resolved.color_space, ColorSpaceSource::caller};
    }
    if (png_info.srgb_defined != 0) {
        return {image::ColorSpace::srgb_rec709, ColorSpaceSource::embedded_srgb};
    }
    if (png_info.iccp_defined != 0) {
        diagnostics.emplace_back(
            "PNG ICC profile is not interpreted; automatic channel rule applied");
    }
    const auto resolved =
        image::resolve_input_space(image::InputColorSpace::automatic, request.intended_channel);
    return {resolved.color_space, ColorSpaceSource::automatic_rule};
}

bool extension_mismatch(std::string_view source_name, ImageFileFormat detected) {
    const std::string extension = lowercase_extension(source_name);
    return !extension.empty() && detected == ImageFileFormat::png && extension != ".png";
}

LodePNGColorType png_color_type(image::PixelFormat format) {
    switch (format.channel_count) {
        case 1:
            return LCT_GREY;
        case 2:
            return LCT_GREY_ALPHA;
        case 3:
            return LCT_RGB;
        case 4:
            return LCT_RGBA;
        default:
            throw ImageIoError(ImageIoErrorCode::unsupported_pixel_format, ImageFileFormat::png,
                               "PNG encoding requires one to four channels");
    }
}

std::vector<unsigned char> pack_image(const image::TiledImage& source) {
    const std::size_t pixels = checked_multiply(source.width(), source.height());
    std::vector<unsigned char> raw(checked_multiply(pixels, source.pixel_bytes()));
    for (std::uint32_t y = 0; y < source.height(); ++y) {
        for (std::uint32_t x = 0; x < source.width(); ++x) {
            const auto pixel = source.read_pixel(x, y);
            const std::size_t offset =
                (static_cast<std::size_t>(y) * source.width() + x) * source.pixel_bytes();
            std::memcpy(raw.data() + offset, pixel.data(), pixel.size());
            if (source.format().channel_type == image::ChannelType::uint16_unorm &&
                std::endian::native == std::endian::little) {
                for (std::size_t channel = 0; channel < pixel.size(); channel += 2) {
                    std::swap(raw[offset + channel], raw[offset + channel + 1]);
                }
            }
        }
    }
    return raw;
}

}  // namespace

static_assert(!container_writer_version.string.empty());

ImageIoError::ImageIoError(ImageIoErrorCode code, ImageFileFormat format, std::string message)
    : std::runtime_error(std::move(message)), code_(code), format_(format) {}

ImageFileFormat detect_image_format(std::span<const std::byte> bytes) noexcept {
    constexpr std::array<std::uint8_t, 8> png{0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a};
    constexpr std::array<std::uint8_t, 3> jpeg{0xff, 0xd8, 0xff};
    constexpr std::array<std::uint8_t, 2> bmp{'B', 'M'};
    constexpr std::array<std::uint8_t, 4> tiff_le{'I', 'I', 0x2a, 0x00};
    constexpr std::array<std::uint8_t, 4> tiff_be{'M', 'M', 0x00, 0x2a};
    constexpr std::array<std::uint8_t, 4> exr{0x76, 0x2f, 0x31, 0x01};
    constexpr std::array<std::uint8_t, 4> psd{'8', 'B', 'P', 'S'};
    constexpr std::array<std::uint8_t, 10> hdr{'#', '?', 'R', 'A', 'D', 'I', 'A', 'N', 'C', 'E'};
    if (starts_with(bytes, png)) return ImageFileFormat::png;
    if (starts_with(bytes, jpeg)) return ImageFileFormat::jpeg;
    if (starts_with(bytes, bmp)) return ImageFileFormat::bmp;
    if (starts_with(bytes, tiff_le) || starts_with(bytes, tiff_be)) return ImageFileFormat::tiff;
    if (starts_with(bytes, exr)) return ImageFileFormat::openexr;
    if (starts_with(bytes, psd)) return ImageFileFormat::psd;
    if (starts_with(bytes, hdr)) return ImageFileFormat::radiance_hdr;
    return ImageFileFormat::unknown;
}

std::string_view image_file_format_name(ImageFileFormat format) noexcept {
    switch (format) {
        case ImageFileFormat::png:
            return "PNG";
        case ImageFileFormat::jpeg:
            return "JPEG";
        case ImageFileFormat::bmp:
            return "BMP";
        case ImageFileFormat::tiff:
            return "TIFF";
        case ImageFileFormat::openexr:
            return "OpenEXR";
        case ImageFileFormat::radiance_hdr:
            return "Radiance HDR";
        case ImageFileFormat::psd:
            return "PSD";
        case ImageFileFormat::unknown:
            return "unknown";
    }
    return "unknown";
}

DecodedImage decode_image_memory(const DecodeRequest& request) {
    const ImageFileFormat detected = detect_image_format(request.bytes);
    if (detected != ImageFileFormat::png) {
        std::string message = "unsupported image format ";
        message += image_file_format_name(detected);
        message += "; this slice supports PNG";
        throw ImageIoError(ImageIoErrorCode::unsupported_format, detected, std::move(message));
    }

    const auto* encoded = reinterpret_cast<const unsigned char*>(request.bytes.data());
    LodePngState inspection;
    unsigned width = 0;
    unsigned height = 0;
    unsigned error =
        lodepng_inspect(&width, &height, inspection.get(), encoded, request.bytes.size());
    if (error != 0) {
        throw ImageIoError(
            ImageIoErrorCode::malformed_input, detected,
            std::string("PNG header inspection failed: ") + lodepng_error_text(error));
    }
    const RawLayout layout = choose_raw_layout(inspection.get()->info_png.color);
    enforce_limits(width, height, layout.pixel_format, request.limits);

    LodePngState decoding;
    decoding.get()->info_raw.colortype = layout.color_type;
    decoding.get()->info_raw.bitdepth =
        static_cast<unsigned>(layout.pixel_format.bytes_per_channel() * 8);
    unsigned char* allocation = nullptr;
    error =
        lodepng_decode(&allocation, &width, &height, decoding.get(), encoded, request.bytes.size());
    LodePngBuffer decoded(allocation, &std::free);
    if (error != 0) {
        throw ImageIoError(ImageIoErrorCode::decode_failed, detected,
                           std::string("PNG decode failed: ") + lodepng_error_text(error));
    }

    std::vector<std::string> diagnostics;
    const auto [color_space, color_source] =
        resolve_color_space(request, decoding.get()->info_png, diagnostics);
    const bool mismatch = extension_mismatch(request.source_name, detected);
    if (mismatch) {
        diagnostics.emplace_back("source extension disagrees with detected PNG content");
    }
    return {
        unpack_image(decoded.get(), width, height, layout.pixel_format),
        color_space,
        {detected, mismatch, color_source, std::move(diagnostics)},
    };
}

std::vector<std::byte> encode_png_memory(const image::TiledImage& source,
                                         PngEncodeOptions options) {
    const image::PixelFormat format = source.format();
    if (format.channel_type == image::ChannelType::float32) {
        throw ImageIoError(ImageIoErrorCode::unsupported_pixel_format, ImageFileFormat::png,
                           "PNG cannot encode 32-bit floating-point channels");
    }
    const unsigned bit_depth = static_cast<unsigned>(format.bytes_per_channel() * 8);
    const LodePNGColorType color_type = png_color_type(format);
    const std::vector<unsigned char> raw = pack_image(source);

    LodePngState encoding;
    encoding.get()->info_raw.colortype = color_type;
    encoding.get()->info_raw.bitdepth = bit_depth;
    encoding.get()->info_png.color.colortype = color_type;
    encoding.get()->info_png.color.bitdepth = bit_depth;
    encoding.get()->encoder.auto_convert = 0;
    if (options.color_space == image::ColorSpace::srgb_rec709) {
        encoding.get()->info_png.srgb_defined = 1;
        encoding.get()->info_png.srgb_intent = 0;
    }

    unsigned char* allocation = nullptr;
    std::size_t encoded_size = 0;
    const unsigned error = lodepng_encode(&allocation, &encoded_size, raw.data(), source.width(),
                                          source.height(), encoding.get());
    LodePngBuffer encoded(allocation, &std::free);
    if (error != 0) {
        throw ImageIoError(ImageIoErrorCode::encode_failed, ImageFileFormat::png,
                           std::string("PNG encode failed: ") + lodepng_error_text(error));
    }
    const auto* begin = reinterpret_cast<const std::byte*>(encoded.get());
    return {begin, begin + encoded_size};
}

}  // namespace ctex::io
