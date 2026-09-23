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
#include <map>
#include <memory>
#include <sstream>
#include <utility>

namespace ctex::io {
namespace {

using LodePngBuffer = std::unique_ptr<unsigned char, decltype(&std::free)>;
using StbiByteBuffer = std::unique_ptr<stbi_uc, decltype(&stbi_image_free)>;
using StbiWordBuffer = std::unique_ptr<stbi_us, decltype(&stbi_image_free)>;
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

std::size_t checked_add(std::size_t left, std::size_t right, ImageFileFormat format) {
    if (left > std::numeric_limits<std::size_t>::max() - right) {
        throw ImageIoError(ImageIoErrorCode::over_limit, format,
                           std::string(image_file_format_name(format)) +
                               " working byte count overflows the platform size type");
    }
    return left + right;
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

std::size_t enforce_limits(std::uint32_t width, std::uint32_t height,
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
    return decoded_bytes;
}

class DecodeSession {
public:
    DecodeSession(const DecodeRequest& request, ImageFileFormat format)
        : request_(request), format_(format) {
        checkpoint(DecodePhase::inspection, 0, 0);
    }

    void preflight(std::uint32_t width, std::uint32_t height,
                   const image::PixelFormat& pixel_format) {
        const std::size_t decoded_bytes =
            enforce_limits(width, height, pixel_format, request_.limits, format_);
        const std::size_t tile_columns =
            1 + (static_cast<std::size_t>(width) - 1) / image::default_tile_size;
        const std::size_t tile_rows =
            1 + (static_cast<std::size_t>(height) - 1) / image::default_tile_size;
        const std::size_t tile_count = checked_multiply(tile_columns, tile_rows, format_);
        const std::size_t tile_bytes = checked_multiply(
            checked_multiply(image::default_tile_size, image::default_tile_size, format_),
            pixel_format.bytes_per_pixel(), format_);
        const std::size_t resident_tile_bytes = checked_multiply(tile_count, tile_bytes, format_);
        constexpr std::size_t estimated_metadata_bytes_per_tile = 256;
        const std::size_t tile_metadata_bytes =
            checked_multiply(tile_count, estimated_metadata_bytes_per_tile, format_);
        estimated_peak_working_bytes_ =
            checked_add(checked_multiply(decoded_bytes, 2, format_), resident_tile_bytes, format_);
        estimated_peak_working_bytes_ =
            checked_add(estimated_peak_working_bytes_, tile_metadata_bytes, format_);
        constexpr std::size_t codec_overhead = 1ULL << 20;
        estimated_peak_working_bytes_ =
            checked_add(estimated_peak_working_bytes_, codec_overhead, format_);
        checkpoint(DecodePhase::inspection, 0, height);
        if (request_.control.maximum_working_bytes == 0 ||
            estimated_peak_working_bytes_ > request_.control.maximum_working_bytes) {
            std::ostringstream message;
            message << image_file_format_name(format_) << " decode requires at most "
                    << estimated_peak_working_bytes_ << " working bytes; ceiling is "
                    << request_.control.maximum_working_bytes << " bytes";
            throw ImageIoError(ImageIoErrorCode::over_limit, format_, message.str());
        }
    }

    void codec_started(std::uint32_t rows) { checkpoint(DecodePhase::codec, 0, rows); }
    void codec_finished(std::uint32_t rows) { checkpoint(DecodePhase::codec, rows, rows); }

    void codec_progress(std::uint32_t completed_rows, std::uint32_t total_rows) {
        const std::uint32_t interval = std::max(request_.control.progress_interval_rows, 1U);
        if (completed_rows == total_rows || completed_rows % interval == 0) {
            checkpoint(DecodePhase::codec, completed_rows, total_rows);
        } else {
            cancel_if_requested(DecodePhase::codec);
        }
    }

    void unpacked_row(std::uint32_t completed_rows, std::uint32_t total_rows) {
        const std::uint32_t interval = std::max(request_.control.progress_interval_rows, 1U);
        if (completed_rows == total_rows || completed_rows % interval == 0) {
            checkpoint(DecodePhase::unpack, completed_rows, total_rows);
        } else {
            cancel_if_requested(DecodePhase::unpack);
        }
    }

    void finish(DecodeReport& report) {
        checkpoint(DecodePhase::complete, 0, 0);
        report.estimated_peak_working_bytes = estimated_peak_working_bytes_;
        report.progress_event_count = progress_event_count_;
    }

private:
    void cancel_if_requested(DecodePhase phase) const {
        if (request_.control.is_cancelled && request_.control.is_cancelled()) {
            throw ImageIoError(ImageIoErrorCode::cancelled, format_,
                               std::string(image_file_format_name(format_)) +
                                   " decode cancelled during phase " +
                                   std::to_string(static_cast<unsigned>(phase)));
        }
    }

    void checkpoint(DecodePhase phase, std::uint32_t completed_rows, std::uint32_t total_rows) {
        cancel_if_requested(phase);
        ++progress_event_count_;
        if (request_.control.report_progress) {
            request_.control.report_progress(
                {.phase = phase,
                 .completed_rows = completed_rows,
                 .total_rows = total_rows,
                 .estimated_peak_working_bytes = estimated_peak_working_bytes_});
        }
        cancel_if_requested(phase);
    }

    const DecodeRequest& request_;
    ImageFileFormat format_;
    std::size_t estimated_peak_working_bytes_{};
    std::size_t progress_event_count_{};
};

std::vector<std::byte> native_pixel(const unsigned char* source, const image::PixelFormat& format) {
    std::vector<std::byte> pixel(format.bytes_per_pixel());
    std::memcpy(pixel.data(), source, pixel.size());
    return pixel;
}

image::TiledImage unpack_image(const unsigned char* decoded, std::uint32_t width,
                               std::uint32_t height, image::PixelFormat format,
                               DecodeSession& session, bool source_uint16_is_big_endian = false) {
    image::TiledImage result(width, height, format);
    const std::size_t stride = format.bytes_per_pixel();
    for (std::uint32_t y = 0; y < height; ++y) {
        for (std::uint32_t x = 0; x < width; ++x) {
            const std::size_t offset = (static_cast<std::size_t>(y) * width + x) * stride;
            std::vector<std::byte> pixel = native_pixel(decoded + offset, format);
            if (format.channel_type == image::ChannelType::uint16_unorm &&
                source_uint16_is_big_endian == (std::endian::native == std::endian::little)) {
                for (std::size_t component = 0; component < pixel.size(); component += 2) {
                    std::swap(pixel[component], pixel[component + 1]);
                }
            }
            result.write_pixel(x, y, pixel);
        }
        session.unpacked_row(y + 1, height);
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
        case ImageFileFormat::tga:
            return extension != ".tga";
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

struct StbiIntegerLayout {
    int width{};
    int height{};
    int source_channels{};
    int output_channels{};
    bool sixteen_bit{};
    image::PixelFormat pixel_format;
};

std::uint16_t big_endian_u16(std::span<const std::byte> bytes, std::size_t offset) {
    return static_cast<std::uint16_t>(std::to_integer<std::uint8_t>(bytes[offset]) << 8U) |
           std::to_integer<std::uint8_t>(bytes[offset + 1]);
}

std::uint16_t little_endian_u16(std::span<const std::byte> bytes, std::size_t offset) {
    return std::to_integer<std::uint8_t>(bytes[offset]) |
           static_cast<std::uint16_t>(std::to_integer<std::uint8_t>(bytes[offset + 1]) << 8U);
}

void require_tga_bytes(std::span<const std::byte> bytes, std::size_t offset, std::size_t count) {
    if (offset > bytes.size() || count > bytes.size() - offset) {
        throw ImageIoError(ImageIoErrorCode::malformed_input, ImageFileFormat::tga,
                           "TGA pixel payload is truncated");
    }
}

std::size_t tga_pixel_offset(std::span<const std::byte> bytes) {
    std::size_t offset = 18 + std::to_integer<std::uint8_t>(bytes[0]);
    if (std::to_integer<std::uint8_t>(bytes[1]) == 1) {
        const std::size_t palette_entries = little_endian_u16(bytes, 5);
        const std::size_t palette_bytes =
            checked_multiply(palette_entries, (std::to_integer<std::uint8_t>(bytes[7]) + 7U) / 8U,
                             ImageFileFormat::tga);
        require_tga_bytes(bytes, offset, palette_bytes);
        offset += palette_bytes;
    }
    return offset;
}

void validate_tga_payload(std::span<const std::byte> bytes) {
    const std::uint8_t image_type = std::to_integer<std::uint8_t>(bytes[2]);
    const std::size_t width = little_endian_u16(bytes, 12);
    const std::size_t height = little_endian_u16(bytes, 14);
    const std::size_t pixel_count = checked_multiply(width, height, ImageFileFormat::tga);
    const std::size_t pixel_bytes = (std::to_integer<std::uint8_t>(bytes[16]) + 7U) / 8U;
    std::size_t cursor = tga_pixel_offset(bytes);
    if (image_type < 9) {
        require_tga_bytes(bytes, cursor,
                          checked_multiply(pixel_count, pixel_bytes, ImageFileFormat::tga));
        return;
    }
    std::size_t decoded_pixels = 0;
    while (decoded_pixels < pixel_count) {
        require_tga_bytes(bytes, cursor, 1);
        const std::uint8_t packet = std::to_integer<std::uint8_t>(bytes[cursor++]);
        const std::size_t packet_pixels = (packet & 0x7fU) + 1U;
        if (packet_pixels > pixel_count - decoded_pixels) {
            throw ImageIoError(ImageIoErrorCode::malformed_input, ImageFileFormat::tga,
                               "TGA RLE packet exceeds the declared image dimensions");
        }
        const std::size_t stored_pixels = (packet & 0x80U) != 0 ? 1 : packet_pixels;
        const std::size_t packet_bytes =
            checked_multiply(stored_pixels, pixel_bytes, ImageFileFormat::tga);
        require_tga_bytes(bytes, cursor, packet_bytes);
        cursor += packet_bytes;
        decoded_pixels += packet_pixels;
    }
}

StbiIntegerLayout inspect_stbi_integer(const DecodeRequest& request, ImageFileFormat format) {
    if (request.bytes.size() > static_cast<std::size_t>(std::numeric_limits<int>::max())) {
        throw ImageIoError(ImageIoErrorCode::over_limit, format,
                           std::string(image_file_format_name(format)) +
                               " input exceeds the codec byte-count limit");
    }
    const auto* encoded = reinterpret_cast<const stbi_uc*>(request.bytes.data());
    const int encoded_size = static_cast<int>(request.bytes.size());
    StbiIntegerLayout result;
    if (stbi_info_from_memory(encoded, encoded_size, &result.width, &result.height,
                              &result.source_channels) == 0 ||
        result.width <= 0 || result.height <= 0 || result.source_channels < 1 ||
        result.source_channels > 4) {
        throw ImageIoError(
            ImageIoErrorCode::malformed_input, format,
            std::string(image_file_format_name(format)) + " header inspection failed");
    }
    result.output_channels = result.source_channels;
    result.sixteen_bit = stbi_is_16_bit_from_memory(encoded, encoded_size) != 0;
    if (format == ImageFileFormat::psd) {
        if (request.bytes.size() < 26) {
            throw ImageIoError(ImageIoErrorCode::malformed_input, format,
                               "PSD header is truncated");
        }
        const int declared_channels = big_endian_u16(request.bytes, 12);
        const int declared_depth = big_endian_u16(request.bytes, 22);
        if (declared_channels < 1 || declared_channels > 4 ||
            (declared_depth != 8 && declared_depth != 16)) {
            throw ImageIoError(ImageIoErrorCode::unsupported_pixel_format, format,
                               "PSD requires one to four 8-bit or 16-bit composite channels");
        }
        result.output_channels = declared_channels;
        result.sixteen_bit = declared_depth == 16;
    }
    result.pixel_format = {
        result.sixteen_bit ? image::ChannelType::uint16_unorm : image::ChannelType::uint8_unorm,
        static_cast<std::uint8_t>(result.output_channels)};
    if (format == ImageFileFormat::tga) validate_tga_payload(request.bytes);
    return result;
}

[[noreturn]] void throw_stbi_decode_error(ImageFileFormat format) {
    const char* reason = stbi_failure_reason();
    throw ImageIoError(ImageIoErrorCode::decode_failed, format,
                       std::string(image_file_format_name(format)) + " decode failed: " +
                           (reason == nullptr ? "unknown codec error" : reason));
}

image::TiledImage load_stbi_integer(std::span<const std::byte> bytes,
                                    const StbiIntegerLayout& layout, ImageFileFormat format,
                                    DecodeSession& session) {
    const auto* encoded = reinterpret_cast<const stbi_uc*>(bytes.data());
    const int encoded_size = static_cast<int>(bytes.size());
    int width = layout.width;
    int height = layout.height;
    int channels = layout.source_channels;
    session.codec_started(static_cast<std::uint32_t>(height));
    if (layout.sixteen_bit) {
        StbiWordBuffer decoded(stbi_load_16_from_memory(encoded, encoded_size, &width, &height,
                                                        &channels, layout.output_channels),
                               &stbi_image_free);
        if (decoded == nullptr) throw_stbi_decode_error(format);
        session.codec_finished(static_cast<std::uint32_t>(height));
        return unpack_image(reinterpret_cast<const unsigned char*>(decoded.get()),
                            static_cast<std::uint32_t>(width), static_cast<std::uint32_t>(height),
                            layout.pixel_format, session);
    }
    StbiByteBuffer decoded(stbi_load_from_memory(encoded, encoded_size, &width, &height, &channels,
                                                 layout.output_channels),
                           &stbi_image_free);
    if (decoded == nullptr) throw_stbi_decode_error(format);
    session.codec_finished(static_cast<std::uint32_t>(height));
    return unpack_image(decoded.get(), static_cast<std::uint32_t>(width),
                        static_cast<std::uint32_t>(height), layout.pixel_format, session);
}

DecodedImage decode_stbi_integer(const DecodeRequest& request, ImageFileFormat format,
                                 DecodeSession& session) {
    const StbiIntegerLayout layout = inspect_stbi_integer(request, format);
    session.preflight(static_cast<std::uint32_t>(layout.width),
                      static_cast<std::uint32_t>(layout.height), layout.pixel_format);
    image::TiledImage pixels = load_stbi_integer(request.bytes, layout, format, session);
    const auto [color_space, color_source] = resolve_float_color_space(request);
    DecodeReport report = float_decode_report(request, format, color_source);
    session.finish(report);
    return {std::move(pixels), color_space, std::move(report)};
}

bool looks_like_tga(std::span<const std::byte> bytes) noexcept {
    if (bytes.size() < 18) {
        return false;
    }
    const auto value = [&](std::size_t offset) {
        return std::to_integer<std::uint8_t>(bytes[offset]);
    };
    const std::uint8_t color_map = value(1);
    const std::uint8_t image_type = value(2);
    const std::uint16_t width = static_cast<std::uint16_t>(value(12) | (value(13) << 8U));
    const std::uint16_t height = static_cast<std::uint16_t>(value(14) | (value(15) << 8U));
    const std::uint8_t bits = value(16);
    if (color_map > 1 || width == 0 || height == 0) return false;
    if (color_map == 1) {
        const std::uint8_t palette_bits = value(7);
        const bool known_palette_depth = palette_bits == 8 || palette_bits == 15 ||
                                         palette_bits == 16 || palette_bits == 24 ||
                                         palette_bits == 32;
        return (image_type == 1 || image_type == 9) && (bits == 8 || bits == 16) &&
               known_palette_depth;
    }
    const bool grayscale = image_type == 3 || image_type == 11;
    const bool true_color = image_type == 2 || image_type == 10;
    const bool known_depth = bits == 8 || bits == 15 || bits == 16 || bits == 24 || bits == 32;
    return (grayscale || true_color) && known_depth;
}

class TiffReader {
public:
    explicit TiffReader(std::span<const std::byte> bytes) : bytes_(bytes) {
        if (bytes.size() < 8) {
            fail("header is truncated");
        }
        const std::uint8_t first = byte(0);
        const std::uint8_t second = byte(1);
        if (first == 'I' && second == 'I') {
            little_endian_ = true;
        } else if (first != 'M' || second != 'M') {
            fail("byte order marker is invalid");
        }
        if (u16(2) != 42) {
            fail("header magic is invalid");
        }
    }

    [[nodiscard]] std::uint8_t byte(std::size_t offset) const {
        require(offset, 1);
        return std::to_integer<std::uint8_t>(bytes_[offset]);
    }

    [[nodiscard]] std::uint16_t u16(std::size_t offset) const {
        require(offset, 2);
        const std::uint16_t first = byte(offset);
        const std::uint16_t second = byte(offset + 1);
        return little_endian_ ? static_cast<std::uint16_t>(first | (second << 8U))
                              : static_cast<std::uint16_t>((first << 8U) | second);
    }

    [[nodiscard]] std::uint32_t u32(std::size_t offset) const {
        require(offset, 4);
        std::uint32_t result = 0;
        for (std::size_t index = 0; index < 4; ++index) {
            const std::size_t source = little_endian_ ? 3 - index : index;
            result = (result << 8U) | byte(offset + source);
        }
        return result;
    }

    [[nodiscard]] bool little_endian() const noexcept { return little_endian_; }
    [[nodiscard]] std::span<const std::byte> bytes(std::size_t offset, std::size_t count) const {
        require(offset, count);
        return bytes_.subspan(offset, count);
    }

    void require(std::size_t offset, std::size_t count) const {
        if (offset > bytes_.size() || count > bytes_.size() - offset) {
            fail("offset or payload is truncated");
        }
    }

    [[noreturn]] static void fail(std::string_view detail) {
        throw ImageIoError(ImageIoErrorCode::malformed_input, ImageFileFormat::tiff,
                           "TIFF " + std::string(detail));
    }

private:
    std::span<const std::byte> bytes_;
    bool little_endian_{};
};

struct TiffEntry {
    std::uint16_t type{};
    std::uint32_t count{};
    std::uint32_t value_offset{};
    std::size_t inline_offset{};
};

using TiffEntries = std::map<std::uint16_t, TiffEntry>;

std::size_t tiff_type_bytes(std::uint16_t type) {
    if (type == 3) return 2;
    if (type == 4) return 4;
    TiffReader::fail("IFD uses an unsupported field type");
}

TiffEntries tiff_entries(const TiffReader& reader) {
    const std::uint32_t ifd_offset = reader.u32(4);
    const std::uint16_t count = reader.u16(ifd_offset);
    const std::size_t entries_offset = static_cast<std::size_t>(ifd_offset) + 2;
    reader.require(entries_offset, static_cast<std::size_t>(count) * 12 + 4);
    TiffEntries result;
    for (std::uint16_t index = 0; index < count; ++index) {
        const std::size_t offset = entries_offset + static_cast<std::size_t>(index) * 12;
        const std::uint16_t tag = reader.u16(offset);
        const TiffEntry entry{.type = reader.u16(offset + 2),
                              .count = reader.u32(offset + 4),
                              .value_offset = reader.u32(offset + 8),
                              .inline_offset = offset + 8};
        if (entry.count == 0 || !result.emplace(tag, entry).second) {
            TiffReader::fail("IFD has an empty or duplicate field");
        }
    }
    return result;
}

std::vector<std::uint32_t> tiff_values(const TiffReader& reader, const TiffEntry& entry) {
    const std::size_t item_bytes = tiff_type_bytes(entry.type);
    const std::size_t total_bytes =
        checked_multiply(entry.count, item_bytes, ImageFileFormat::tiff);
    const std::size_t offset =
        total_bytes <= 4 ? entry.inline_offset : static_cast<std::size_t>(entry.value_offset);
    reader.require(offset, total_bytes);
    std::vector<std::uint32_t> result;
    result.reserve(entry.count);
    for (std::uint32_t index = 0; index < entry.count; ++index) {
        const std::size_t value_offset = offset + static_cast<std::size_t>(index) * item_bytes;
        result.push_back(entry.type == 3 ? reader.u16(value_offset) : reader.u32(value_offset));
    }
    return result;
}

const TiffEntry& tiff_required(const TiffEntries& entries, std::uint16_t tag,
                               std::string_view name) {
    const auto found = entries.find(tag);
    if (found == entries.end()) {
        TiffReader::fail("IFD is missing " + std::string(name));
    }
    return found->second;
}

std::uint32_t tiff_scalar(const TiffReader& reader, const TiffEntries& entries, std::uint16_t tag,
                          std::string_view name, std::uint32_t default_value = 0) {
    const auto found = entries.find(tag);
    if (found == entries.end()) return default_value;
    const std::vector<std::uint32_t> values = tiff_values(reader, found->second);
    if (values.size() != 1) {
        TiffReader::fail("IFD field is not scalar: " + std::string(name));
    }
    return values.front();
}

std::uint32_t tiff_uniform_sample_value(const TiffReader& reader, const TiffEntry& entry,
                                        std::uint32_t channels, std::string_view name) {
    const std::vector<std::uint32_t> values = tiff_values(reader, entry);
    if (values.empty() || (values.size() != 1 && values.size() != channels) ||
        !std::all_of(values.begin(), values.end(),
                     [&](std::uint32_t value) { return value == values.front(); })) {
        TiffReader::fail(std::string(name) + " values are inconsistent");
    }
    return values.front();
}

void normalize_tiff_byte_order(std::vector<std::byte>& pixels, std::size_t sample_bytes,
                               bool source_little_endian) {
    if (sample_bytes == 1 || source_little_endian == (std::endian::native == std::endian::little)) {
        return;
    }
    for (std::size_t offset = 0; offset < pixels.size(); offset += sample_bytes) {
        std::reverse(pixels.begin() + static_cast<std::ptrdiff_t>(offset),
                     pixels.begin() + static_cast<std::ptrdiff_t>(offset + sample_bytes));
    }
}

image::ChannelType tiff_channel_type(std::uint32_t sample_format, std::uint32_t bits) {
    if (sample_format == 1 && bits == 8) return image::ChannelType::uint8_unorm;
    if (sample_format == 1 && bits == 16) return image::ChannelType::uint16_unorm;
    if (sample_format == 3 && bits == 32) return image::ChannelType::float32;
    throw ImageIoError(ImageIoErrorCode::unsupported_pixel_format, ImageFileFormat::tiff,
                       "TIFF supports 8/16-bit unsigned or 32-bit floating-point samples");
}

void validate_tiff_storage(const TiffReader& reader, const TiffEntries& entries,
                           std::uint32_t channels) {
    if (tiff_scalar(reader, entries, 259, "Compression", 1) != 1 ||
        tiff_scalar(reader, entries, 284, "PlanarConfiguration", 1) != 1 ||
        tiff_scalar(reader, entries, 274, "Orientation", 1) != 1) {
        throw ImageIoError(ImageIoErrorCode::unsupported_pixel_format, ImageFileFormat::tiff,
                           "TIFF requires uncompressed, contiguous, top-left scanlines");
    }
    const std::uint32_t photometric =
        tiff_scalar(reader, entries, 262, "PhotometricInterpretation");
    if ((channels < 3 && photometric != 1) || (channels >= 3 && photometric != 2)) {
        throw ImageIoError(ImageIoErrorCode::unsupported_pixel_format, ImageFileFormat::tiff,
                           "TIFF photometric interpretation does not match its channels");
    }
}

struct TiffLayout {
    std::uint32_t width{};
    std::uint32_t height{};
    std::uint32_t rows_per_strip{};
    image::PixelFormat pixel_format;
};

TiffLayout inspect_tiff_layout(const TiffReader& reader, const TiffEntries& entries,
                               const DecodeLimits& limits) {
    const std::uint32_t width = tiff_scalar(reader, entries, 256, "ImageWidth");
    const std::uint32_t height = tiff_scalar(reader, entries, 257, "ImageLength");
    const std::uint32_t channels = tiff_scalar(reader, entries, 277, "SamplesPerPixel", 1);
    if (width == 0 || height == 0 || channels == 0 || channels > 4) {
        TiffReader::fail("dimensions or channel count are invalid");
    }
    const std::uint32_t bits = tiff_uniform_sample_value(
        reader, tiff_required(entries, 258, "BitsPerSample"), channels, "BitsPerSample");
    const auto sample_format_entry = entries.find(339);
    const std::uint32_t sample_format =
        sample_format_entry == entries.end()
            ? 1
            : tiff_uniform_sample_value(reader, sample_format_entry->second, channels,
                                        "SampleFormat");
    const image::PixelFormat pixel_format{tiff_channel_type(sample_format, bits),
                                          static_cast<std::uint8_t>(channels)};
    validate_tiff_storage(reader, entries, channels);
    enforce_limits(width, height, pixel_format, limits, ImageFileFormat::tiff);
    const std::uint32_t rows_per_strip = tiff_scalar(reader, entries, 278, "RowsPerStrip", height);
    if (rows_per_strip == 0) TiffReader::fail("RowsPerStrip is zero");
    return {width, height, rows_per_strip, pixel_format};
}

std::vector<std::byte> read_tiff_pixels(const TiffReader& reader, const TiffEntries& entries,
                                        const TiffLayout& layout, DecodeSession& session) {
    const std::vector<std::uint32_t> offsets =
        tiff_values(reader, tiff_required(entries, 273, "StripOffsets"));
    const std::vector<std::uint32_t> byte_counts =
        tiff_values(reader, tiff_required(entries, 279, "StripByteCounts"));
    if (offsets.size() != byte_counts.size()) {
        TiffReader::fail("strip offset and byte-count arrays disagree");
    }
    const std::size_t row_bytes = checked_multiply(
        layout.width, layout.pixel_format.bytes_per_pixel(), ImageFileFormat::tiff);
    const std::size_t expected_strip_count = (layout.height - 1U) / layout.rows_per_strip + 1U;
    if (offsets.size() != expected_strip_count) {
        TiffReader::fail("strip count is inconsistent with RowsPerStrip");
    }
    std::vector<std::byte> pixels(
        checked_multiply(row_bytes, layout.height, ImageFileFormat::tiff));
    std::size_t destination = 0;
    session.codec_started(layout.height);
    for (std::size_t index = 0; index < offsets.size(); ++index) {
        const std::size_t remaining = pixels.size() - destination;
        const std::size_t expected = std::min(
            remaining, checked_multiply(layout.rows_per_strip, row_bytes, ImageFileFormat::tiff));
        if (byte_counts[index] != expected) TiffReader::fail("strip byte count is inconsistent");
        const auto strip = reader.bytes(offsets[index], byte_counts[index]);
        std::copy(strip.begin(), strip.end(),
                  pixels.begin() + static_cast<std::ptrdiff_t>(destination));
        destination += strip.size();
        const std::uint32_t completed_rows = static_cast<std::uint32_t>(std::min<std::size_t>(
            layout.height, (index + 1) * static_cast<std::size_t>(layout.rows_per_strip)));
        session.codec_progress(completed_rows, layout.height);
    }
    if (destination != pixels.size()) TiffReader::fail("strip data is incomplete");
    normalize_tiff_byte_order(pixels, layout.pixel_format.bytes_per_channel(),
                              reader.little_endian());
    return pixels;
}

DecodedImage decode_tiff(const DecodeRequest& request, DecodeSession& session) {
    const TiffReader reader(request.bytes);
    const TiffEntries entries = tiff_entries(reader);
    const TiffLayout layout = inspect_tiff_layout(reader, entries, request.limits);
    session.preflight(layout.width, layout.height, layout.pixel_format);
    const std::vector<std::byte> pixels = read_tiff_pixels(reader, entries, layout, session);
    const auto [color_space, color_source] = resolve_float_color_space(request);
    image::TiledImage unpacked =
        unpack_image(reinterpret_cast<const unsigned char*>(pixels.data()), layout.width,
                     layout.height, layout.pixel_format, session);
    DecodeReport report = float_decode_report(request, ImageFileFormat::tiff, color_source);
    session.finish(report);
    return {std::move(unpacked), color_space, std::move(report)};
}

DecodedImage decode_radiance_hdr(const DecodeRequest& request, DecodeSession& session) {
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
    session.preflight(static_cast<std::uint32_t>(width), static_cast<std::uint32_t>(height),
                      format);
    session.codec_started(static_cast<std::uint32_t>(height));
    StbiFloatBuffer decoded(
        stbi_loadf_from_memory(encoded, encoded_size, &width, &height, &channels, 0),
        &stbi_image_free);
    if (decoded == nullptr) {
        const char* reason = stbi_failure_reason();
        throw ImageIoError(ImageIoErrorCode::decode_failed, ImageFileFormat::radiance_hdr,
                           std::string("Radiance HDR decode failed: ") +
                               (reason == nullptr ? "unknown codec error" : reason));
    }
    session.codec_finished(static_cast<std::uint32_t>(height));
    const auto [color_space, color_source] = resolve_float_color_space(request);
    image::TiledImage unpacked = unpack_image(reinterpret_cast<const unsigned char*>(decoded.get()),
                                              static_cast<std::uint32_t>(width),
                                              static_cast<std::uint32_t>(height), format, session);
    DecodeReport report = float_decode_report(request, ImageFileFormat::radiance_hdr, color_source);
    session.finish(report);
    return {std::move(unpacked), color_space, std::move(report)};
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

DecodedImage decode_openexr(const DecodeRequest& request, DecodeSession& session) {
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
    session.preflight(declared_width, declared_height, format);
    if (declared_width > static_cast<std::uint32_t>(std::numeric_limits<int>::max()) ||
        declared_height > static_cast<std::uint32_t>(std::numeric_limits<int>::max())) {
        throw ImageIoError(ImageIoErrorCode::over_limit, ImageFileFormat::openexr,
                           "OpenEXR dimensions exceed the codec integer limit");
    }

    float* allocation = nullptr;
    int width = 0;
    int height = 0;
    TinyExrError decode_error;
    session.codec_started(declared_height);
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
    session.codec_finished(declared_height);
    const auto [color_space, color_source] = resolve_float_color_space(request);
    image::TiledImage unpacked = unpack_image(reinterpret_cast<const unsigned char*>(decoded.get()),
                                              declared_width, declared_height, format, session);
    DecodeReport report = float_decode_report(request, ImageFileFormat::openexr, color_source);
    session.finish(report);
    return {std::move(unpacked), color_space, std::move(report)};
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
    if (looks_like_tga(bytes)) return ImageFileFormat::tga;
    return ImageFileFormat::unknown;
}

std::string_view image_file_format_name(ImageFileFormat format) noexcept {
    switch (format) {
        case ImageFileFormat::png:
            return "PNG";
        case ImageFileFormat::jpeg:
            return "JPEG";
        case ImageFileFormat::tga:
            return "TGA";
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
    if (detected == ImageFileFormat::unknown) {
        std::string message = "unsupported image format ";
        message += image_file_format_name(detected);
        message +=
            "; supported formats are PNG, JPEG, TGA, BMP, TIFF, OpenEXR, Radiance HDR and PSD";
        throw ImageIoError(ImageIoErrorCode::unsupported_format, detected, std::move(message));
    }
    DecodeSession session(request, detected);
    if (detected == ImageFileFormat::radiance_hdr) {
        return decode_radiance_hdr(request, session);
    }
    if (detected == ImageFileFormat::openexr) {
        return decode_openexr(request, session);
    }
    if (detected == ImageFileFormat::tiff) {
        return decode_tiff(request, session);
    }
    if (detected == ImageFileFormat::jpeg || detected == ImageFileFormat::tga ||
        detected == ImageFileFormat::bmp || detected == ImageFileFormat::psd) {
        return decode_stbi_integer(request, detected, session);
    }
    if (detected != ImageFileFormat::png) {
        std::string message = "unsupported image format ";
        message += image_file_format_name(detected);
        message +=
            "; supported formats are PNG, JPEG, TGA, BMP, TIFF, OpenEXR, Radiance HDR and PSD";
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
    session.preflight(width, height, layout.pixel_format);

    LodePngState decoding;
    decoding.get()->info_raw.colortype = layout.color_type;
    decoding.get()->info_raw.bitdepth =
        static_cast<unsigned>(layout.pixel_format.bytes_per_channel() * 8);
    unsigned char* allocation = nullptr;
    session.codec_started(height);
    error =
        lodepng_decode(&allocation, &width, &height, decoding.get(), encoded, request.bytes.size());
    LodePngBuffer decoded(allocation, &std::free);
    if (error != 0) {
        throw ImageIoError(ImageIoErrorCode::decode_failed, detected,
                           std::string("PNG decode failed: ") + lodepng_error_text(error));
    }
    session.codec_finished(height);

    std::vector<std::string> diagnostics;
    const auto [color_space, color_source] =
        resolve_color_space(request, decoding.get()->info_png, diagnostics);
    const bool mismatch = extension_mismatch(request.source_name, detected);
    if (mismatch) {
        diagnostics.emplace_back("source extension disagrees with detected PNG content");
    }
    image::TiledImage unpacked =
        unpack_image(decoded.get(), width, height, layout.pixel_format, session, true);
    DecodeReport report{detected, mismatch, color_source, std::move(diagnostics)};
    session.finish(report);
    return {std::move(unpacked), color_space, std::move(report)};
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
