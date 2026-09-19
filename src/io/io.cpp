#include <lodepng.h>
#include <tinyexr.h>

#define STB_IMAGE_IMPLEMENTATION
#define STBI_NO_STDIO
#include <stb_image.h>

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
using StbiFloatBuffer = std::unique_ptr<float, decltype(&stbi_image_free)>;
using ExrFloatBuffer = std::unique_ptr<float, decltype(&std::free)>;

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

class TinyExrHeader {
public:
    TinyExrHeader() { InitEXRHeader(&value_); }
    ~TinyExrHeader() { FreeEXRHeader(&value_); }
    TinyExrHeader(const TinyExrHeader&) = delete;
    TinyExrHeader& operator=(const TinyExrHeader&) = delete;
    [[nodiscard]] EXRHeader* get() noexcept { return &value_; }
    [[nodiscard]] const EXRHeader* get() const noexcept { return &value_; }

private:
    EXRHeader value_{};
};

class TinyExrError {
public:
    TinyExrError() = default;
    ~TinyExrError() {
        if (value_ != nullptr) {
            FreeEXRErrorMessage(value_);
        }
    }
    TinyExrError(const TinyExrError&) = delete;
    TinyExrError& operator=(const TinyExrError&) = delete;
    [[nodiscard]] const char** output() noexcept { return &value_; }
    [[nodiscard]] std::string message() const {
        return value_ == nullptr ? "unknown TinyEXR error" : value_;
    }

private:
    const char* value_{};
};

struct RawLayout {
    LodePNGColorType color_type;
    image::PixelFormat pixel_format;
};

std::size_t checked_multiply(std::size_t left, std::size_t right,
                             ImageFileFormat format = ImageFileFormat::png) {
    if (right != 0 && left > std::numeric_limits<std::size_t>::max() / right) {
        throw ImageIoError(ImageIoErrorCode::over_limit, format,
                           std::string(image_file_format_name(format)) +
                               " decoded byte count overflows the platform size type");
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

void enforce_limits(std::uint32_t width, std::uint32_t height,
                    const image::PixelFormat& pixel_format, const DecodeLimits& limits,
                    ImageFileFormat file_format = ImageFileFormat::png) {
    const std::size_t pixels = checked_multiply(width, height, file_format);
    const std::size_t decoded_bytes =
        checked_multiply(pixels, pixel_format.bytes_per_pixel(), file_format);
    if (width > limits.maximum_width || height > limits.maximum_height ||
        decoded_bytes > limits.maximum_decoded_bytes) {
        std::ostringstream message;
        message << image_file_format_name(file_format) << " dimensions " << width << 'x' << height
                << " require " << decoded_bytes << " decoded bytes; limits are "
                << limits.maximum_width << 'x' << limits.maximum_height << " and "
                << limits.maximum_decoded_bytes << " bytes";
        throw ImageIoError(ImageIoErrorCode::over_limit, file_format, message.str());
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
    if (extension.empty()) {
        return false;
    }
    switch (detected) {
        case ImageFileFormat::png:
            return extension != ".png";
        case ImageFileFormat::jpeg:
            return extension != ".jpg" && extension != ".jpeg";
        case ImageFileFormat::bmp:
            return extension != ".bmp";
        case ImageFileFormat::tiff:
            return extension != ".tif" && extension != ".tiff";
        case ImageFileFormat::openexr:
            return extension != ".exr";
        case ImageFileFormat::radiance_hdr:
            return extension != ".hdr" && extension != ".rgbe";
        case ImageFileFormat::psd:
            return extension != ".psd";
        case ImageFileFormat::unknown:
            return true;
    }
    return true;
}

std::pair<image::ColorSpace, ColorSpaceSource> resolve_float_color_space(
    const DecodeRequest& request) {
    if (request.color_space == image::InputColorSpace::automatic) {
        return {image::ColorSpace::linear_rec709, ColorSpaceSource::automatic_rule};
    }
    const auto resolved = image::resolve_input_space(request.color_space, request.intended_channel);
    return {resolved.color_space, ColorSpaceSource::caller};
}

DecodeReport float_decode_report(const DecodeRequest& request, ImageFileFormat format,
                                 ColorSpaceSource source) {
    const bool mismatch = extension_mismatch(request.source_name, format);
    std::vector<std::string> diagnostics;
    if (mismatch) {
        diagnostics.emplace_back("source extension disagrees with detected " +
                                 std::string(image_file_format_name(format)) + " content");
    }
    return {format, mismatch, source, std::move(diagnostics)};
}

DecodedImage decode_radiance_hdr(const DecodeRequest& request) {
    if (request.bytes.size() > static_cast<std::size_t>(std::numeric_limits<int>::max())) {
        throw ImageIoError(ImageIoErrorCode::over_limit, ImageFileFormat::radiance_hdr,
                           "Radiance HDR input exceeds the codec byte-count limit");
    }
    const auto* encoded = reinterpret_cast<const stbi_uc*>(request.bytes.data());
    const int encoded_size = static_cast<int>(request.bytes.size());
    int width = 0;
    int height = 0;
    int channels = 0;
    if (stbi_info_from_memory(encoded, encoded_size, &width, &height, &channels) == 0 ||
        width <= 0 || height <= 0 || channels < 1 || channels > 4) {
        throw ImageIoError(ImageIoErrorCode::malformed_input, ImageFileFormat::radiance_hdr,
                           "Radiance HDR header inspection failed");
    }
    const image::PixelFormat format{image::ChannelType::float32,
                                    static_cast<std::uint8_t>(channels)};
    enforce_limits(static_cast<std::uint32_t>(width), static_cast<std::uint32_t>(height), format,
                   request.limits, ImageFileFormat::radiance_hdr);
    StbiFloatBuffer decoded(
        stbi_loadf_from_memory(encoded, encoded_size, &width, &height, &channels, 0),
        &stbi_image_free);
    if (decoded == nullptr) {
        const char* reason = stbi_failure_reason();
        throw ImageIoError(ImageIoErrorCode::decode_failed, ImageFileFormat::radiance_hdr,
                           std::string("Radiance HDR decode failed: ") +
                               (reason == nullptr ? "unknown codec error" : reason));
    }
    const auto [color_space, color_source] = resolve_float_color_space(request);
    return {
        unpack_image(reinterpret_cast<const unsigned char*>(decoded.get()),
                     static_cast<std::uint32_t>(width), static_cast<std::uint32_t>(height), format),
        color_space,
        float_decode_report(request, ImageFileFormat::radiance_hdr, color_source),
    };
}

std::pair<std::uint32_t, std::uint32_t> exr_dimensions(const EXRHeader& header) {
    const std::int64_t width =
        static_cast<std::int64_t>(header.data_window.max_x) - header.data_window.min_x + 1;
    const std::int64_t height =
        static_cast<std::int64_t>(header.data_window.max_y) - header.data_window.min_y + 1;
    if (width <= 0 || height <= 0 || width > std::numeric_limits<std::uint32_t>::max() ||
        height > std::numeric_limits<std::uint32_t>::max()) {
        throw ImageIoError(ImageIoErrorCode::malformed_input, ImageFileFormat::openexr,
                           "OpenEXR data window is invalid");
    }
    return {static_cast<std::uint32_t>(width), static_cast<std::uint32_t>(height)};
}

DecodedImage decode_openexr(const DecodeRequest& request) {
    const auto* encoded = reinterpret_cast<const unsigned char*>(request.bytes.data());
    EXRVersion version{};
    if (ParseEXRVersionFromMemory(&version, encoded, request.bytes.size()) != TINYEXR_SUCCESS) {
        throw ImageIoError(ImageIoErrorCode::malformed_input, ImageFileFormat::openexr,
                           "OpenEXR version inspection failed");
    }
    if (version.multipart != 0 || version.non_image != 0) {
        throw ImageIoError(ImageIoErrorCode::unsupported_format, ImageFileFormat::openexr,
                           "OpenEXR multipart and deep images require the layered-source API");
    }
    TinyExrHeader header;
    TinyExrError header_error;
    if (ParseEXRHeaderFromMemory(header.get(), &version, encoded, request.bytes.size(),
                                 header_error.output()) != TINYEXR_SUCCESS) {
        throw ImageIoError(ImageIoErrorCode::malformed_input, ImageFileFormat::openexr,
                           "OpenEXR header inspection failed: " + header_error.message());
    }
    const auto [declared_width, declared_height] = exr_dimensions(*header.get());
    const image::PixelFormat format{image::ChannelType::float32, 4};
    enforce_limits(declared_width, declared_height, format, request.limits,
                   ImageFileFormat::openexr);
    if (declared_width > static_cast<std::uint32_t>(std::numeric_limits<int>::max()) ||
        declared_height > static_cast<std::uint32_t>(std::numeric_limits<int>::max())) {
        throw ImageIoError(ImageIoErrorCode::over_limit, ImageFileFormat::openexr,
                           "OpenEXR dimensions exceed the codec integer limit");
    }

    float* allocation = nullptr;
    int width = 0;
    int height = 0;
    TinyExrError decode_error;
    const int decode_result = LoadEXRFromMemory(&allocation, &width, &height, encoded,
                                                request.bytes.size(), decode_error.output());
    ExrFloatBuffer decoded(allocation, &std::free);
    if (decode_result != TINYEXR_SUCCESS) {
        throw ImageIoError(ImageIoErrorCode::decode_failed, ImageFileFormat::openexr,
                           "OpenEXR decode failed: " + decode_error.message());
    }
    if (width != static_cast<int>(declared_width) || height != static_cast<int>(declared_height)) {
        throw ImageIoError(ImageIoErrorCode::decode_failed, ImageFileFormat::openexr,
                           "OpenEXR decoded dimensions disagree with its header");
    }
    const auto [color_space, color_source] = resolve_float_color_space(request);
    return {
        unpack_image(reinterpret_cast<const unsigned char*>(decoded.get()), declared_width,
                     declared_height, format),
        color_space,
        float_decode_report(request, ImageFileFormat::openexr, color_source),
    };
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
    constexpr std::array<std::uint8_t, 6> rgbe{'#', '?', 'R', 'G', 'B', 'E'};
    if (starts_with(bytes, png)) return ImageFileFormat::png;
    if (starts_with(bytes, jpeg)) return ImageFileFormat::jpeg;
    if (starts_with(bytes, bmp)) return ImageFileFormat::bmp;
    if (starts_with(bytes, tiff_le) || starts_with(bytes, tiff_be)) return ImageFileFormat::tiff;
    if (starts_with(bytes, exr)) return ImageFileFormat::openexr;
    if (starts_with(bytes, psd)) return ImageFileFormat::psd;
    if (starts_with(bytes, hdr) || starts_with(bytes, rgbe)) return ImageFileFormat::radiance_hdr;
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
    if (detected == ImageFileFormat::radiance_hdr) {
        return decode_radiance_hdr(request);
    }
    if (detected == ImageFileFormat::openexr) {
        return decode_openexr(request);
    }
    if (detected != ImageFileFormat::png) {
        std::string message = "unsupported image format ";
        message += image_file_format_name(detected);
        message += "; this slice supports PNG, OpenEXR and Radiance HDR";
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
