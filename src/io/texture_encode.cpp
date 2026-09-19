#include <lodepng.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STBI_WRITE_NO_STDIO
#include <stb_image_write.h>

#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <ctex/io/texture_encode.hpp>
#include <limits>
#include <memory>
#include <string_view>
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

private:
    LodePNGState state_{};
};

class ByteWriter {
public:
    void u8(std::uint8_t value) { bytes_.push_back(static_cast<std::byte>(value)); }
    void u16(std::uint16_t value) {
        u8(static_cast<std::uint8_t>(value));
        u8(static_cast<std::uint8_t>(value >> 8U));
    }
    void u32(std::uint32_t value) {
        for (unsigned shift = 0; shift < 32; shift += 8) {
            u8(static_cast<std::uint8_t>(value >> shift));
        }
    }
    void u64(std::uint64_t value) {
        for (unsigned shift = 0; shift < 64; shift += 8) {
            u8(static_cast<std::uint8_t>(value >> shift));
        }
    }
    void f32(float value) { u32(std::bit_cast<std::uint32_t>(value)); }
    void stringz(std::string_view value) {
        bytes_.insert(bytes_.end(), reinterpret_cast<const std::byte*>(value.data()),
                      reinterpret_cast<const std::byte*>(value.data() + value.size()));
        u8(0);
    }
    void bytes(std::span<const std::byte> value) {
        bytes_.insert(bytes_.end(), value.begin(), value.end());
    }
    [[nodiscard]] std::size_t size() const noexcept { return bytes_.size(); }
    [[nodiscard]] const std::vector<std::byte>& view() const noexcept { return bytes_; }
    [[nodiscard]] std::vector<std::byte> finish() && { return std::move(bytes_); }

private:
    std::vector<std::byte> bytes_;
};

std::size_t checked_multiply(std::size_t left, std::size_t right, ExportImageFormat format,
                             ExportBitDepth depth) {
    if (right != 0 && left > std::numeric_limits<std::size_t>::max() / right) {
        throw TextureEncodeError(TextureEncodeErrorCode::over_limit, format, depth,
                                 "encoded texture size overflows the platform size type");
    }
    return left * right;
}

std::size_t checked_add(std::size_t left, std::size_t right, ExportImageFormat format,
                        ExportBitDepth depth) {
    if (left > std::numeric_limits<std::size_t>::max() - right) {
        throw TextureEncodeError(TextureEncodeErrorCode::over_limit, format, depth,
                                 "encoded texture size overflows the platform size type");
    }
    return left + right;
}

double read_component(std::span<const std::byte> pixel, image::ChannelType type,
                      std::size_t component) {
    switch (type) {
        case image::ChannelType::uint8_unorm:
            return std::to_integer<std::uint8_t>(pixel[component]) / 255.0;
        case image::ChannelType::uint16_unorm: {
            std::uint16_t value = 0;
            std::memcpy(&value, pixel.data() + component * sizeof(value), sizeof(value));
            return static_cast<double>(value) / 65535.0;
        }
        case image::ChannelType::float32: {
            float value = 0.0F;
            std::memcpy(&value, pixel.data() + component * sizeof(value), sizeof(value));
            return value;
        }
    }
    return 0.0;
}

std::uint8_t unorm8(double value) {
    return static_cast<std::uint8_t>(std::round(std::clamp(value, 0.0, 1.0) * 255.0));
}

std::uint16_t unorm16(double value) {
    return static_cast<std::uint16_t>(std::round(std::clamp(value, 0.0, 1.0) * 65535.0));
}

enum class SampleByteOrder : std::uint8_t { little_endian, big_endian };

void write_sample(std::vector<std::byte>& result, std::size_t& offset, double value,
                  ExportBitDepth depth, SampleByteOrder order) {
    if (depth == ExportBitDepth::bits_8) {
        result[offset++] = static_cast<std::byte>(unorm8(value));
        return;
    }
    if (depth == ExportBitDepth::bits_16) {
        const std::uint16_t encoded = unorm16(value);
        const std::byte low = static_cast<std::byte>(encoded);
        const std::byte high = static_cast<std::byte>(encoded >> 8U);
        result[offset++] = order == SampleByteOrder::little_endian ? low : high;
        result[offset++] = order == SampleByteOrder::little_endian ? high : low;
        return;
    }
    const std::uint32_t encoded = std::bit_cast<std::uint32_t>(static_cast<float>(value));
    for (unsigned shift = 0; shift < 32; shift += 8) {
        result[offset++] = static_cast<std::byte>(encoded >> shift);
    }
}

std::vector<std::byte> pack_pixels(const image::TiledImage& source, ExportBitDepth depth,
                                   SampleByteOrder order, ExportImageFormat format) {
    const std::size_t channel_bytes = static_cast<unsigned>(depth) / 8;
    const std::size_t sample_count =
        checked_multiply(checked_multiply(source.width(), source.height(), format, depth),
                         source.format().channel_count, format, depth);
    std::vector<std::byte> result(checked_multiply(sample_count, channel_bytes, format, depth));
    std::size_t offset = 0;
    for (std::uint32_t y = 0; y < source.height(); ++y) {
        for (std::uint32_t x = 0; x < source.width(); ++x) {
            const auto pixel = source.read_pixel(x, y);
            for (std::size_t channel = 0; channel < source.format().channel_count; ++channel) {
                const double value = read_component(pixel, source.format().channel_type, channel);
                write_sample(result, offset, value, depth, order);
            }
        }
    }
    return result;
}

LodePNGColorType png_color_type(std::uint8_t channels) {
    switch (channels) {
        case 1:
            return LCT_GREY;
        case 2:
            return LCT_GREY_ALPHA;
        case 3:
            return LCT_RGB;
        case 4:
            return LCT_RGBA;
        default:
            return LCT_MAX_OCTET_VALUE;
    }
}

std::vector<std::byte> encode_png(const image::TiledImage& source,
                                  const TextureEncodeOptions& options) {
    const unsigned bit_depth = static_cast<unsigned>(options.bit_depth);
    const LodePNGColorType color_type = png_color_type(source.format().channel_count);
    const std::vector<std::byte> raw =
        pack_pixels(source, options.bit_depth, SampleByteOrder::big_endian, options.format);
    LodePngState state;
    state.get()->info_raw.colortype = color_type;
    state.get()->info_raw.bitdepth = bit_depth;
    state.get()->info_png.color.colortype = color_type;
    state.get()->info_png.color.bitdepth = bit_depth;
    state.get()->encoder.auto_convert = 0;
    if (options.color_space == image::ColorSpace::srgb_rec709) {
        state.get()->info_png.srgb_defined = 1;
        state.get()->info_png.srgb_intent = 0;
    }
    unsigned char* allocation = nullptr;
    std::size_t encoded_size = 0;
    const unsigned error = lodepng_encode(&allocation, &encoded_size,
                                          reinterpret_cast<const unsigned char*>(raw.data()),
                                          source.width(), source.height(), state.get());
    LodePngBuffer encoded(allocation, &std::free);
    if (error != 0) {
        throw TextureEncodeError(TextureEncodeErrorCode::encode_failed, options.format,
                                 options.bit_depth,
                                 std::string("PNG encode failed: ") + lodepng_error_text(error));
    }
    const auto* begin = reinterpret_cast<const std::byte*>(encoded.get());
    return {begin, begin + encoded_size};
}

void stb_write_callback(void* context, void* data, int size) {
    auto& output = *static_cast<std::vector<std::byte>*>(context);
    const auto* begin = static_cast<const std::byte*>(data);
    output.insert(output.end(), begin, begin + size);
}

std::vector<std::byte> encode_stb(const image::TiledImage& source,
                                  const TextureEncodeOptions& options) {
    if (source.width() > static_cast<std::uint32_t>(std::numeric_limits<int>::max()) ||
        source.height() > static_cast<std::uint32_t>(std::numeric_limits<int>::max())) {
        throw TextureEncodeError(TextureEncodeErrorCode::over_limit, options.format,
                                 options.bit_depth,
                                 "JPEG or TGA dimensions exceed the codec integer limit");
    }
    const std::vector<std::byte> raw =
        pack_pixels(source, options.bit_depth, SampleByteOrder::little_endian, options.format);
    std::vector<std::byte> encoded;
    const int width = static_cast<int>(source.width());
    const int height = static_cast<int>(source.height());
    const int channels = source.format().channel_count;
    int result = 0;
    if (options.format == ExportImageFormat::jpeg) {
        result = stbi_write_jpg_to_func(stb_write_callback, &encoded, width, height, channels,
                                        raw.data(), options.jpeg_quality);
    } else {
        result = stbi_write_tga_to_func(stb_write_callback, &encoded, width, height, channels,
                                        raw.data());
    }
    if (result == 0) {
        throw TextureEncodeError(
            TextureEncodeErrorCode::encode_failed, options.format, options.bit_depth,
            std::string(export_image_format_name(options.format)) + " encoder refused the image");
    }
    return encoded;
}

struct TiffEntry {
    std::uint16_t tag{};
    std::uint16_t type{};
    std::uint32_t count{};
    std::uint32_t value{};
};

struct TiffLayout {
    std::uint32_t bits_value{};
    std::uint32_t sample_format_value{};
    std::uint32_t pixel_offset{};
};

std::uint32_t inline_short_values(std::uint16_t value, std::uint16_t count) {
    return value | (count == 2 ? static_cast<std::uint32_t>(value) << 16U : 0);
}

TiffLayout make_tiff_layout(std::uint16_t channels, std::uint16_t entry_count, std::uint16_t bits,
                            std::uint16_t sample_format) {
    constexpr std::uint32_t ifd_offset = 8;
    const std::uint32_t ifd_bytes = 2 + entry_count * 12 + 4;
    std::uint32_t extra_offset = ifd_offset + ifd_bytes;
    TiffLayout result{.bits_value = inline_short_values(bits, channels),
                      .sample_format_value = inline_short_values(sample_format, channels)};
    if (channels > 2) {
        result.bits_value = extra_offset;
        extra_offset += channels * 2;
        result.sample_format_value = extra_offset;
        extra_offset += channels * 2;
    }
    result.pixel_offset = extra_offset;
    return result;
}

void write_tiff_sample_arrays(ByteWriter& writer, std::uint16_t channels, std::uint16_t bits,
                              std::uint16_t sample_format) {
    if (channels <= 2) {
        return;
    }
    for (std::uint16_t index = 0; index < channels; ++index) {
        writer.u16(bits);
    }
    for (std::uint16_t index = 0; index < channels; ++index) {
        writer.u16(sample_format);
    }
}

std::vector<std::byte> encode_tiff(const image::TiledImage& source,
                                   const TextureEncodeOptions& options) {
    const std::uint16_t channels = source.format().channel_count;
    const std::uint16_t bits = static_cast<unsigned>(options.bit_depth);
    const std::uint16_t sample_format = options.bit_depth == ExportBitDepth::bits_32 ? 3 : 1;
    const std::vector<std::byte> pixels =
        pack_pixels(source, options.bit_depth, SampleByteOrder::little_endian, options.format);
    const bool has_alpha = channels % 2 == 0;
    const std::uint16_t entry_count = static_cast<std::uint16_t>(11 + (has_alpha ? 1 : 0));
    const std::uint32_t ifd_offset = 8;
    const TiffLayout layout = make_tiff_layout(channels, entry_count, bits, sample_format);
    if (pixels.size() > std::numeric_limits<std::uint32_t>::max()) {
        throw TextureEncodeError(TextureEncodeErrorCode::over_limit, options.format,
                                 options.bit_depth, "TIFF strip exceeds the 32-bit baseline limit");
    }
    std::vector<TiffEntry> entries{
        {256, 4, 1, source.width()},
        {257, 4, 1, source.height()},
        {258, 3, channels, layout.bits_value},
        {259, 3, 1, 1},
        {262, 3, 1, static_cast<std::uint32_t>(channels >= 3 ? 2 : 1)},
        {273, 4, 1, layout.pixel_offset},
        {277, 3, 1, channels},
        {278, 4, 1, source.height()},
        {279, 4, 1, static_cast<std::uint32_t>(pixels.size())},
        {284, 3, 1, 1},
    };
    if (has_alpha) {
        entries.push_back({338, 3, 1, 2});
    }
    entries.push_back({339, 3, channels, layout.sample_format_value});
    std::sort(entries.begin(), entries.end(),
              [](const TiffEntry& left, const TiffEntry& right) { return left.tag < right.tag; });

    ByteWriter writer;
    writer.u8('I');
    writer.u8('I');
    writer.u16(42);
    writer.u32(ifd_offset);
    writer.u16(entry_count);
    for (const TiffEntry& entry : entries) {
        writer.u16(entry.tag);
        writer.u16(entry.type);
        writer.u32(entry.count);
        writer.u32(entry.value);
    }
    writer.u32(0);
    write_tiff_sample_arrays(writer, channels, bits, sample_format);
    writer.bytes(pixels);
    return std::move(writer).finish();
}

std::uint16_t float_to_half(float value) noexcept {
    const std::uint32_t bits = std::bit_cast<std::uint32_t>(value);
    const std::uint16_t sign = static_cast<std::uint16_t>((bits >> 16U) & 0x8000U);
    const std::uint32_t exponent = (bits >> 23U) & 0xffU;
    std::uint32_t mantissa = bits & 0x7fffffU;
    if (exponent == 0xffU) {
        return static_cast<std::uint16_t>(sign | 0x7c00U |
                                          (mantissa == 0 ? 0 : (mantissa >> 13U) | 1U));
    }
    int half_exponent = static_cast<int>(exponent) - 127 + 15;
    if (half_exponent >= 31) {
        return static_cast<std::uint16_t>(sign | 0x7c00U);
    }
    if (half_exponent <= 0) {
        if (half_exponent < -10) {
            return sign;
        }
        mantissa = (mantissa | 0x800000U) >> static_cast<unsigned>(1 - half_exponent);
        return static_cast<std::uint16_t>(sign | ((mantissa + 0x1000U) >> 13U));
    }
    mantissa += 0x1000U;
    if ((mantissa & 0x800000U) != 0) {
        mantissa = 0;
        ++half_exponent;
        if (half_exponent >= 31) {
            return static_cast<std::uint16_t>(sign | 0x7c00U);
        }
    }
    return static_cast<std::uint16_t>(sign | (static_cast<unsigned>(half_exponent) << 10U) |
                                      (mantissa >> 13U));
}

struct ExrChannel {
    std::string_view name;
    std::uint8_t source_component{};
};

std::span<const ExrChannel> exr_channels(std::uint8_t count) {
    static constexpr std::array one{ExrChannel{"Y", 0}};
    static constexpr std::array two{ExrChannel{"A", 1}, ExrChannel{"Y", 0}};
    static constexpr std::array three{ExrChannel{"B", 2}, ExrChannel{"G", 1}, ExrChannel{"R", 0}};
    static constexpr std::array four{ExrChannel{"A", 3}, ExrChannel{"B", 2}, ExrChannel{"G", 1},
                                     ExrChannel{"R", 0}};
    switch (count) {
        case 1:
            return one;
        case 2:
            return two;
        case 3:
            return three;
        case 4:
            return four;
        default:
            return {};
    }
}

void exr_attribute(ByteWriter& header, std::string_view name, std::string_view type,
                   const ByteWriter& value) {
    header.stringz(name);
    header.stringz(type);
    header.u32(static_cast<std::uint32_t>(value.size()));
    header.bytes(value.view());
}

std::vector<std::byte> encode_openexr(const image::TiledImage& source,
                                      const TextureEncodeOptions& options) {
    const std::span channels = exr_channels(source.format().channel_count);
    const std::uint32_t pixel_type = options.bit_depth == ExportBitDepth::bits_16 ? 1 : 2;
    const std::size_t component_bytes = static_cast<unsigned>(options.bit_depth) / 8;
    const std::size_t row_bytes = checked_multiply(
        checked_multiply(source.width(), channels.size(), options.format, options.bit_depth),
        component_bytes, options.format, options.bit_depth);
    if (row_bytes > std::numeric_limits<std::int32_t>::max() ||
        source.width() > static_cast<std::uint32_t>(std::numeric_limits<std::int32_t>::max()) ||
        source.height() > static_cast<std::uint32_t>(std::numeric_limits<std::int32_t>::max())) {
        throw TextureEncodeError(TextureEncodeErrorCode::over_limit, options.format,
                                 options.bit_depth, "OpenEXR dimensions exceed format limits");
    }

    ByteWriter channel_list;
    for (const ExrChannel& channel : channels) {
        channel_list.stringz(channel.name);
        channel_list.u32(pixel_type);
        channel_list.u8(0);
        channel_list.u8(0);
        channel_list.u8(0);
        channel_list.u8(0);
        channel_list.u32(1);
        channel_list.u32(1);
    }
    channel_list.u8(0);
    ByteWriter compression;
    compression.u8(0);
    ByteWriter window;
    window.u32(0);
    window.u32(0);
    window.u32(source.width() - 1);
    window.u32(source.height() - 1);
    ByteWriter line_order;
    line_order.u8(0);
    ByteWriter aspect;
    aspect.f32(1.0F);
    ByteWriter center;
    center.f32(0.0F);
    center.f32(0.0F);
    ByteWriter screen_width;
    screen_width.f32(1.0F);

    ByteWriter writer;
    writer.u32(20000630U);
    writer.u32(2);
    exr_attribute(writer, "channels", "chlist", channel_list);
    exr_attribute(writer, "compression", "compression", compression);
    exr_attribute(writer, "dataWindow", "box2i", window);
    exr_attribute(writer, "displayWindow", "box2i", window);
    exr_attribute(writer, "lineOrder", "lineOrder", line_order);
    exr_attribute(writer, "pixelAspectRatio", "float", aspect);
    exr_attribute(writer, "screenWindowCenter", "v2f", center);
    exr_attribute(writer, "screenWindowWidth", "float", screen_width);
    writer.u8(0);

    const std::size_t offset_table_bytes =
        checked_multiply(source.height(), sizeof(std::uint64_t), options.format, options.bit_depth);
    const std::size_t chunk_bytes = checked_add(8, row_bytes, options.format, options.bit_depth);
    std::size_t chunk_offset =
        checked_add(writer.size(), offset_table_bytes, options.format, options.bit_depth);
    for (std::uint32_t y = 0; y < source.height(); ++y) {
        writer.u64(chunk_offset);
        chunk_offset = checked_add(chunk_offset, chunk_bytes, options.format, options.bit_depth);
    }
    for (std::uint32_t y = 0; y < source.height(); ++y) {
        writer.u32(y);
        writer.u32(static_cast<std::uint32_t>(row_bytes));
        for (const ExrChannel& channel : channels) {
            for (std::uint32_t x = 0; x < source.width(); ++x) {
                const auto pixel = source.read_pixel(x, y);
                const float value = static_cast<float>(
                    read_component(pixel, source.format().channel_type, channel.source_component));
                if (options.bit_depth == ExportBitDepth::bits_16) {
                    writer.u16(float_to_half(value));
                } else {
                    writer.f32(value);
                }
            }
        }
    }
    return std::move(writer).finish();
}

void validate_options(const image::TiledImage& pixels, const TextureEncodeOptions& options) {
    const bool known_format =
        options.format == ExportImageFormat::png || options.format == ExportImageFormat::jpeg ||
        options.format == ExportImageFormat::tga || options.format == ExportImageFormat::tiff ||
        options.format == ExportImageFormat::openexr;
    if (!known_format) {
        throw TextureEncodeError(TextureEncodeErrorCode::invalid_option, options.format,
                                 options.bit_depth, "texture output format is invalid");
    }
    if (!export_format_supports_bit_depth(options.format, options.bit_depth)) {
        std::string message(export_image_format_name(options.format));
        message += " cannot encode ";
        message += std::to_string(static_cast<unsigned>(options.bit_depth));
        message += " bits per channel";
        throw TextureEncodeError(TextureEncodeErrorCode::unsupported_combination, options.format,
                                 options.bit_depth, std::move(message));
    }
    if (options.color_space != image::ColorSpace::linear_rec709 &&
        options.color_space != image::ColorSpace::srgb_rec709) {
        throw TextureEncodeError(TextureEncodeErrorCode::invalid_option, options.format,
                                 options.bit_depth, "texture output colour space is invalid");
    }
    if (options.format == ExportImageFormat::jpeg &&
        (options.jpeg_quality < 1 || options.jpeg_quality > 100)) {
        throw TextureEncodeError(TextureEncodeErrorCode::invalid_option, options.format,
                                 options.bit_depth, "JPEG quality must be from 1 through 100");
    }
    if (pixels.format().channel_count == 0 || pixels.format().channel_count > 4 ||
        !pixels.format().is_valid()) {
        throw TextureEncodeError(TextureEncodeErrorCode::invalid_pixels, options.format,
                                 options.bit_depth,
                                 "texture encoding requires one through four valid channels");
    }
}

}  // namespace

TextureEncodeError::TextureEncodeError(TextureEncodeErrorCode code, ExportImageFormat format,
                                       ExportBitDepth bit_depth, std::string message)
    : std::runtime_error(std::move(message)), code_(code), format_(format), bit_depth_(bit_depth) {}

std::vector<std::byte> encode_texture_memory(const image::TiledImage& pixels,
                                             const TextureEncodeOptions& options) {
    validate_options(pixels, options);
    switch (options.format) {
        case ExportImageFormat::png:
            return encode_png(pixels, options);
        case ExportImageFormat::jpeg:
        case ExportImageFormat::tga:
            return encode_stb(pixels, options);
        case ExportImageFormat::tiff:
            return encode_tiff(pixels, options);
        case ExportImageFormat::openexr:
            return encode_openexr(pixels, options);
    }
    throw TextureEncodeError(TextureEncodeErrorCode::invalid_option, options.format,
                             options.bit_depth, "texture output format is invalid");
}

}  // namespace ctex::io
