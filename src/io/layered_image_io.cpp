#include <tinyexr.h>

#include <algorithm>
#include <array>
#include <bit>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <ctex/io/image_io.hpp>
#include <limits>
#include <memory>
#include <optional>
#include <sstream>
#include <utility>

#include "color_profile.hpp"

namespace ctex::io {
namespace {

constexpr image::PixelFormat rgba8{image::ChannelType::uint8_unorm, 4};
constexpr image::PixelFormat rgba16{image::ChannelType::uint16_unorm, 4};
constexpr image::PixelFormat rgba32f{image::ChannelType::float32, 4};

[[noreturn]] void fail(ImageIoErrorCode code, ImageFileFormat format, std::string message) {
    throw ImageIoError(code, format, std::move(message));
}

std::size_t checked_add(std::size_t left, std::size_t right, ImageFileFormat format) {
    if (left > std::numeric_limits<std::size_t>::max() - right) {
        fail(ImageIoErrorCode::over_limit, format, "layered decode byte count overflows size_t");
    }
    return left + right;
}

std::size_t checked_multiply(std::size_t left, std::size_t right, ImageFileFormat format) {
    if (right != 0 && left > std::numeric_limits<std::size_t>::max() / right) {
        fail(ImageIoErrorCode::over_limit, format, "layered decode byte count overflows size_t");
    }
    return left * right;
}

struct ImageLayout {
    std::uint32_t width{};
    std::uint32_t height{};
    image::PixelFormat format{};
};

class LayerDecodeMonitor {
public:
    LayerDecodeMonitor(const LayeredDecodeRequest& request, ImageFileFormat format)
        : request_(request), format_(format) {
        checkpoint(DecodePhase::inspection, 0);
    }

    void preflight(std::span<const ImageLayout> layouts, std::size_t additional_codec_bytes = 0) {
        if (layouts.empty() || layouts.size() > request_.maximum_image_count) {
            fail(ImageIoErrorCode::over_limit, format_,
                 "layered image count is zero or exceeds maximum_image_count");
        }
        std::size_t decoded_bytes = 0;
        std::size_t resident_bytes = 0;
        std::size_t tile_metadata_bytes = 0;
        total_rows_ = 0;
        for (const ImageLayout& layout : layouts) {
            if (layout.width == 0 || layout.height == 0 ||
                layout.width > request_.image.limits.maximum_width ||
                layout.height > request_.image.limits.maximum_height) {
                fail(ImageIoErrorCode::over_limit, format_,
                     "layer dimensions are empty or exceed the configured limit");
            }
            const std::size_t pixels = checked_multiply(layout.width, layout.height, format_);
            decoded_bytes = checked_add(
                decoded_bytes, checked_multiply(pixels, layout.format.bytes_per_pixel(), format_),
                format_);
            const std::size_t columns =
                1 + (static_cast<std::size_t>(layout.width) - 1) / image::default_tile_size;
            const std::size_t rows =
                1 + (static_cast<std::size_t>(layout.height) - 1) / image::default_tile_size;
            const std::size_t tiles = checked_multiply(columns, rows, format_);
            const std::size_t tile_bytes = checked_multiply(
                checked_multiply(image::default_tile_size, image::default_tile_size, format_),
                layout.format.bytes_per_pixel(), format_);
            resident_bytes =
                checked_add(resident_bytes, checked_multiply(tiles, tile_bytes, format_), format_);
            tile_metadata_bytes =
                checked_add(tile_metadata_bytes, checked_multiply(tiles, 256, format_), format_);
            total_rows_ = checked_add(total_rows_, layout.height, format_);
        }
        if (decoded_bytes > request_.image.limits.maximum_decoded_bytes) {
            fail(ImageIoErrorCode::over_limit, format_,
                 "aggregate layered pixels exceed maximum_decoded_bytes");
        }
        constexpr std::size_t codec_scratch = 1ULL << 20;
        estimated_peak_ =
            checked_add(checked_multiply(decoded_bytes, 3, format_), resident_bytes, format_);
        estimated_peak_ = checked_add(estimated_peak_, additional_codec_bytes, format_);
        estimated_peak_ = checked_add(estimated_peak_, tile_metadata_bytes, format_);
        estimated_peak_ = checked_add(estimated_peak_, codec_scratch, format_);
        checkpoint(DecodePhase::inspection, 0);
        if (request_.image.control.maximum_working_bytes == 0 ||
            estimated_peak_ > request_.image.control.maximum_working_bytes) {
            std::ostringstream message;
            message << image_file_format_name(format_) << " layered decode requires at most "
                    << estimated_peak_ << " working bytes; ceiling is "
                    << request_.image.control.maximum_working_bytes << " bytes";
            fail(ImageIoErrorCode::over_limit, format_, message.str());
        }
    }

    void codec_started() { checkpoint(DecodePhase::codec, 0); }
    void codec_finished() { checkpoint(DecodePhase::codec, total_rows_); }

    void unpacked_row() {
        ++completed_rows_;
        const std::size_t interval =
            std::max<std::size_t>(request_.image.control.progress_interval_rows, 1);
        if (completed_rows_ == total_rows_ || completed_rows_ % interval == 0) {
            checkpoint(DecodePhase::unpack, completed_rows_);
        } else {
            cancel_if_requested(DecodePhase::unpack);
        }
    }

    void finish(std::span<DecodedImageLayer> layers) {
        checkpoint(DecodePhase::complete, total_rows_);
        for (DecodedImageLayer& layer : layers) {
            layer.image.report.estimated_peak_working_bytes = estimated_peak_;
            layer.image.report.progress_event_count = progress_events_;
        }
    }

private:
    void cancel_if_requested(DecodePhase phase) const {
        if (request_.image.control.is_cancelled && request_.image.control.is_cancelled()) {
            fail(ImageIoErrorCode::cancelled, format_,
                 std::string(image_file_format_name(format_)) +
                     " layered decode cancelled during phase " +
                     std::to_string(static_cast<unsigned>(phase)));
        }
    }

    void checkpoint(DecodePhase phase, std::size_t completed_rows) {
        cancel_if_requested(phase);
        ++progress_events_;
        if (request_.image.control.report_progress) {
            request_.image.control.report_progress(
                {.phase = phase,
                 .completed_rows = static_cast<std::uint32_t>(std::min<std::size_t>(
                     completed_rows, std::numeric_limits<std::uint32_t>::max())),
                 .total_rows = static_cast<std::uint32_t>(
                     std::min<std::size_t>(total_rows_, std::numeric_limits<std::uint32_t>::max())),
                 .estimated_peak_working_bytes = estimated_peak_});
        }
        cancel_if_requested(phase);
    }

    const LayeredDecodeRequest& request_;
    ImageFileFormat format_;
    std::size_t total_rows_{};
    std::size_t completed_rows_{};
    std::size_t estimated_peak_{};
    std::size_t progress_events_{};
};

std::pair<image::ColorSpace, ColorSpaceSource> layered_color_space(const DecodeRequest& request,
                                                                   bool high_dynamic_range) {
    if (request.color_space != image::InputColorSpace::automatic) {
        return {
            image::resolve_input_space(request.color_space, request.intended_channel).color_space,
            ColorSpaceSource::caller};
    }
    if (high_dynamic_range) {
        return {image::ColorSpace::linear_rec709, ColorSpaceSource::automatic_rule};
    }
    return {image::resolve_input_space(image::InputColorSpace::automatic, request.intended_channel)
                .color_space,
            ColorSpaceSource::automatic_rule};
}

bool layered_extension_mismatch(std::string_view source_name, ImageFileFormat format) {
    const std::size_t dot = source_name.find_last_of('.');
    if (dot == std::string_view::npos) return false;
    std::string extension(source_name.substr(dot));
    std::ranges::transform(extension, extension.begin(), [](unsigned char value) {
        return static_cast<char>(std::tolower(value));
    });
    return (format == ImageFileFormat::psd && extension != ".psd") ||
           (format == ImageFileFormat::openexr && extension != ".exr");
}

DecodeReport make_report(const DecodeRequest& request, ImageFileFormat format,
                         ColorSpaceSource source, std::vector<std::string> diagnostics = {}) {
    const bool mismatch = layered_extension_mismatch(request.source_name, format);
    if (mismatch) {
        diagnostics.emplace_back("source extension disagrees with detected " +
                                 std::string(image_file_format_name(format)) + " content");
    }
    return {.detected_format = format,
            .extension_mismatch = mismatch,
            .color_space_source = source,
            .diagnostics = std::move(diagnostics)};
}

image::TiledImage tiled_from_interleaved(std::span<const std::byte> pixels, ImageLayout layout,
                                         LayerDecodeMonitor& monitor,
                                         bool source_uint16_big_endian = false) {
    image::TiledImage result(layout.width, layout.height, layout.format);
    const std::size_t pixel_bytes = layout.format.bytes_per_pixel();
    for (std::uint32_t y = 0; y < layout.height; ++y) {
        for (std::uint32_t x = 0; x < layout.width; ++x) {
            const std::size_t offset =
                (static_cast<std::size_t>(y) * layout.width + x) * pixel_bytes;
            std::array<std::byte, 16> native{};
            std::copy_n(pixels.data() + offset, pixel_bytes, native.data());
            if (source_uint16_big_endian && std::endian::native == std::endian::little) {
                for (std::size_t component = 0; component < pixel_bytes; component += 2) {
                    std::swap(native[component], native[component + 1]);
                }
            }
            result.write_pixel(x, y, std::span(native.data(), pixel_bytes));
        }
        monitor.unpacked_row();
    }
    result.clear_dirty();
    return result;
}

class BigEndianReader {
public:
    explicit BigEndianReader(std::span<const std::byte> bytes) : bytes_(bytes) {}

    [[nodiscard]] std::size_t remaining() const noexcept { return bytes_.size() - offset_; }
    [[nodiscard]] std::size_t position() const noexcept { return offset_; }

    std::uint8_t u8() {
        require(1);
        return std::to_integer<std::uint8_t>(bytes_[offset_++]);
    }

    std::uint16_t u16() {
        require(2);
        const std::uint16_t value =
            static_cast<std::uint16_t>(std::to_integer<std::uint8_t>(bytes_[offset_]) << 8U) |
            std::to_integer<std::uint8_t>(bytes_[offset_ + 1]);
        offset_ += 2;
        return value;
    }

    std::int16_t i16() { return static_cast<std::int16_t>(u16()); }

    std::uint32_t u32() {
        require(4);
        std::uint32_t value = 0;
        for (unsigned index = 0; index < 4; ++index) value = (value << 8U) | u8();
        return value;
    }

    std::int32_t i32() { return static_cast<std::int32_t>(u32()); }

    std::span<const std::byte> take(std::size_t count) {
        require(count);
        const auto result = bytes_.subspan(offset_, count);
        offset_ += count;
        return result;
    }

    void skip(std::size_t count) { static_cast<void>(take(count)); }

private:
    void require(std::size_t count) const {
        if (count > remaining()) {
            fail(ImageIoErrorCode::malformed_input, ImageFileFormat::psd,
                 "PSD structure is truncated");
        }
    }

    std::span<const std::byte> bytes_;
    std::size_t offset_{};
};

struct PsdChannelRecord {
    std::int16_t id{};
    std::uint32_t byte_count{};
};

struct PsdLayerRecord {
    std::int32_t top{};
    std::int32_t left{};
    std::uint32_t width{};
    std::uint32_t height{};
    std::string name;
    std::vector<PsdChannelRecord> channels;
};

void append_utf8(std::string& output, std::uint32_t codepoint) {
    if (codepoint <= 0x7f) {
        output.push_back(static_cast<char>(codepoint));
    } else if (codepoint <= 0x7ff) {
        output.push_back(static_cast<char>(0xc0U | (codepoint >> 6U)));
        output.push_back(static_cast<char>(0x80U | (codepoint & 0x3fU)));
    } else if (codepoint <= 0xffff) {
        output.push_back(static_cast<char>(0xe0U | (codepoint >> 12U)));
        output.push_back(static_cast<char>(0x80U | ((codepoint >> 6U) & 0x3fU)));
        output.push_back(static_cast<char>(0x80U | (codepoint & 0x3fU)));
    } else {
        output.push_back(static_cast<char>(0xf0U | (codepoint >> 18U)));
        output.push_back(static_cast<char>(0x80U | ((codepoint >> 12U) & 0x3fU)));
        output.push_back(static_cast<char>(0x80U | ((codepoint >> 6U) & 0x3fU)));
        output.push_back(static_cast<char>(0x80U | (codepoint & 0x3fU)));
    }
}

std::string psd_unicode_name(std::span<const std::byte> payload) {
    BigEndianReader reader(payload);
    const std::uint32_t count = reader.u32();
    if (count > reader.remaining() / 2) {
        fail(ImageIoErrorCode::malformed_input, ImageFileFormat::psd,
             "PSD Unicode layer name is truncated");
    }
    std::string result;
    for (std::uint32_t index = 0; index < count; ++index) {
        std::uint32_t codepoint = reader.u16();
        if (codepoint >= 0xd800 && codepoint <= 0xdbff && index + 1 < count) {
            const std::uint32_t low = reader.u16();
            ++index;
            if (low < 0xdc00 || low > 0xdfff) {
                codepoint = 0xfffd;
            } else {
                codepoint = 0x10000 + ((codepoint - 0xd800) << 10U) + (low - 0xdc00);
            }
        } else if (codepoint >= 0xd800 && codepoint <= 0xdfff) {
            codepoint = 0xfffd;
        }
        append_utf8(result, codepoint);
    }
    return result;
}

std::string parse_psd_layer_extra(BigEndianReader& record) {
    const std::size_t mask_size = record.u32();
    record.skip(mask_size);
    const std::size_t ranges_size = record.u32();
    record.skip(ranges_size);
    const std::uint8_t name_size = record.u8();
    const auto name_bytes = record.take(name_size);
    std::string name(reinterpret_cast<const char*>(name_bytes.data()), name_bytes.size());
    const std::size_t pascal_bytes = static_cast<std::size_t>(name_size) + 1;
    record.skip((4 - (pascal_bytes % 4)) % 4);
    while (record.remaining() >= 12) {
        const auto signature = record.take(4);
        const auto key = record.take(4);
        const std::uint32_t size = record.u32();
        const auto payload = record.take(size);
        if (std::memcmp(signature.data(), "8BIM", 4) == 0 &&
            std::memcmp(key.data(), "luni", 4) == 0) {
            name = psd_unicode_name(payload);
        }
        if ((size & 1U) != 0 && record.remaining() != 0) record.skip(1);
    }
    return name;
}

PsdLayerRecord parse_psd_layer_record(BigEndianReader& layer_info) {
    PsdLayerRecord layer;
    layer.top = layer_info.i32();
    layer.left = layer_info.i32();
    const std::int32_t bottom = layer_info.i32();
    const std::int32_t right = layer_info.i32();
    if (bottom <= layer.top || right <= layer.left) {
        fail(ImageIoErrorCode::unsupported_format, ImageFileFormat::psd,
             "PSD adjustment or empty layers are not raster images");
    }
    const std::int64_t width = static_cast<std::int64_t>(right) - layer.left;
    const std::int64_t height = static_cast<std::int64_t>(bottom) - layer.top;
    if (width > std::numeric_limits<std::uint32_t>::max() ||
        height > std::numeric_limits<std::uint32_t>::max()) {
        fail(ImageIoErrorCode::over_limit, ImageFileFormat::psd,
             "PSD layer dimensions exceed uint32 range");
    }
    layer.width = static_cast<std::uint32_t>(width);
    layer.height = static_cast<std::uint32_t>(height);
    const std::uint16_t channel_count = layer_info.u16();
    if (channel_count == 0 || channel_count > 56) {
        fail(ImageIoErrorCode::malformed_input, ImageFileFormat::psd,
             "PSD layer channel count is invalid");
    }
    layer.channels.reserve(channel_count);
    for (std::uint16_t channel = 0; channel < channel_count; ++channel) {
        layer.channels.push_back({layer_info.i16(), layer_info.u32()});
    }
    if (std::memcmp(layer_info.take(4).data(), "8BIM", 4) != 0) {
        fail(ImageIoErrorCode::malformed_input, ImageFileFormat::psd,
             "PSD layer blend signature is invalid");
    }
    layer_info.skip(4);  // blend mode key
    layer_info.skip(4);  // opacity, clipping, flags and filler
    BigEndianReader extra(layer_info.take(layer_info.u32()));
    layer.name = parse_psd_layer_extra(extra);
    if (layer.name.empty()) layer.name = "Layer";
    return layer;
}

std::vector<std::byte> decode_packbits_row(BigEndianReader& source, std::size_t encoded_size,
                                           std::size_t expected_size) {
    BigEndianReader encoded(source.take(encoded_size));
    std::vector<std::byte> row;
    row.reserve(expected_size);
    while (encoded.remaining() != 0 && row.size() < expected_size) {
        const std::int8_t control = static_cast<std::int8_t>(encoded.u8());
        if (control >= 0) {
            const std::size_t count = static_cast<std::size_t>(control) + 1;
            const auto literal = encoded.take(count);
            row.insert(row.end(), literal.begin(), literal.end());
        } else if (control != -128) {
            const std::size_t count = static_cast<std::size_t>(1 - control);
            const std::byte value = static_cast<std::byte>(encoded.u8());
            row.insert(row.end(), count, value);
        }
        if (row.size() > expected_size) {
            fail(ImageIoErrorCode::malformed_input, ImageFileFormat::psd,
                 "PSD PackBits row expands beyond its declared width");
        }
    }
    if (row.size() != expected_size || encoded.remaining() != 0) {
        fail(ImageIoErrorCode::malformed_input, ImageFileFormat::psd,
             "PSD PackBits row does not match its declared width");
    }
    return row;
}

std::vector<std::byte> decode_psd_channel(BigEndianReader& source, const PsdLayerRecord& layer,
                                          const PsdChannelRecord& channel,
                                          std::size_t bytes_per_sample) {
    if (channel.byte_count < 2 || channel.byte_count - 2 > source.remaining()) {
        fail(ImageIoErrorCode::malformed_input, ImageFileFormat::psd,
             "PSD layer channel payload is truncated");
    }
    BigEndianReader payload(source.take(channel.byte_count));
    const std::uint16_t compression = payload.u16();
    const std::size_t row_size =
        checked_multiply(layer.width, bytes_per_sample, ImageFileFormat::psd);
    const std::size_t expected = checked_multiply(row_size, layer.height, ImageFileFormat::psd);
    if (compression == 0) {
        const auto raw = payload.take(expected);
        if (payload.remaining() != 0) {
            fail(ImageIoErrorCode::malformed_input, ImageFileFormat::psd,
                 "PSD raw channel length is inconsistent");
        }
        return {raw.begin(), raw.end()};
    }
    if (compression != 1) {
        fail(ImageIoErrorCode::unsupported_format, ImageFileFormat::psd,
             "PSD layered import supports raw and PackBits channel compression");
    }
    std::vector<std::uint16_t> row_sizes(layer.height);
    for (std::uint16_t& size : row_sizes) size = payload.u16();
    std::vector<std::byte> result;
    result.reserve(expected);
    for (std::uint16_t size : row_sizes) {
        auto row = decode_packbits_row(payload, size, row_size);
        result.insert(result.end(), row.begin(), row.end());
    }
    if (payload.remaining() != 0) {
        fail(ImageIoErrorCode::malformed_input, ImageFileFormat::psd,
             "PSD PackBits channel length is inconsistent");
    }
    return result;
}

std::vector<std::byte> interleave_psd_layer(BigEndianReader& pixels, const PsdLayerRecord& layer,
                                            std::size_t bytes_per_sample) {
    const std::size_t sample_count =
        checked_multiply(layer.width, layer.height, ImageFileFormat::psd);
    const std::size_t pixel_bytes = checked_multiply(bytes_per_sample, 4, ImageFileFormat::psd);
    std::vector<std::byte> result(
        checked_multiply(sample_count, pixel_bytes, ImageFileFormat::psd));
    for (std::size_t sample = 0; sample < sample_count; ++sample) {
        for (std::size_t byte = 0; byte < bytes_per_sample; ++byte) {
            result[sample * pixel_bytes + 3 * bytes_per_sample + byte] = std::byte{0xff};
        }
    }
    for (const PsdChannelRecord& channel : layer.channels) {
        std::vector<std::byte> planar =
            decode_psd_channel(pixels, layer, channel, bytes_per_sample);
        const int destination = channel.id == -1 ? 3 : channel.id;
        if (destination < 0 || destination > 3) continue;
        for (std::size_t sample = 0; sample < sample_count; ++sample) {
            std::copy_n(planar.data() + sample * bytes_per_sample, bytes_per_sample,
                        result.data() + sample * pixel_bytes +
                            static_cast<std::size_t>(destination) * bytes_per_sample);
        }
    }
    return result;
}

LayeredDecodedImage decode_psd_layers(const LayeredDecodeRequest& request) {
    BigEndianReader file(request.image.bytes);
    if (std::memcmp(file.take(4).data(), "8BPS", 4) != 0 || file.u16() != 1) {
        fail(ImageIoErrorCode::malformed_input, ImageFileFormat::psd,
             "PSD layered import requires a version-1 PSD file");
    }
    file.skip(6);
    const std::uint16_t document_channels = file.u16();
    static_cast<void>(file.u32());
    static_cast<void>(file.u32());
    const std::uint16_t depth = file.u16();
    const std::uint16_t color_mode = file.u16();
    if (document_channels < 3 || (depth != 8 && depth != 16) || color_mode != 3) {
        fail(ImageIoErrorCode::unsupported_format, ImageFileFormat::psd,
             "PSD layered import requires 8-bit or 16-bit RGB content");
    }
    file.skip(file.u32());
    file.skip(file.u32());
    BigEndianReader layer_mask(file.take(file.u32()));
    if (layer_mask.remaining() < 4) {
        fail(ImageIoErrorCode::malformed_input, ImageFileFormat::psd,
             "PSD has no layer information");
    }
    BigEndianReader layer_info(layer_mask.take(layer_mask.u32()));
    const std::int16_t signed_count = layer_info.i16();
    const std::size_t layer_count = static_cast<std::size_t>(
        signed_count < 0 ? -static_cast<std::int32_t>(signed_count) : signed_count);
    if (layer_count == 0 || layer_count > request.maximum_image_count) {
        fail(ImageIoErrorCode::over_limit, ImageFileFormat::psd,
             "PSD layer count is zero or exceeds maximum_image_count");
    }
    std::vector<PsdLayerRecord> records;
    records.reserve(layer_count);
    for (std::size_t index = 0; index < layer_count; ++index) {
        records.push_back(parse_psd_layer_record(layer_info));
    }
    std::vector<ImageLayout> layouts;
    layouts.reserve(layer_count);
    const image::PixelFormat format = depth == 16 ? rgba16 : rgba8;
    for (const PsdLayerRecord& layer : records) {
        layouts.push_back({layer.width, layer.height, format});
    }
    LayerDecodeMonitor monitor(request, ImageFileFormat::psd);
    monitor.preflight(layouts);
    monitor.codec_started();
    const std::size_t bytes_per_sample = depth / 8;
    std::vector<std::vector<std::byte>> packed;
    packed.reserve(layer_count);
    for (const PsdLayerRecord& layer : records) {
        packed.push_back(interleave_psd_layer(layer_info, layer, bytes_per_sample));
    }
    monitor.codec_finished();
    const detail::ResolvedProfileColorSpace resolved =
        detail::resolve_psd_color_space(request.image);
    LayeredDecodedImage result{
        .format = ImageFileFormat::psd, .source_was_layered = true, .images = {}};
    result.images.reserve(layer_count);
    for (std::size_t index = 0; index < layer_count; ++index) {
        result.images.push_back(
            {.name = records[index].name,
             .origin_x = records[index].left,
             .origin_y = records[index].top,
             .image = {tiled_from_interleaved(packed[index], layouts[index], monitor, depth == 16),
                       resolved.color_space,
                       make_report(request.image, ImageFileFormat::psd, resolved.source,
                                   resolved.diagnostics)}});
    }
    monitor.finish(result.images);
    return result;
}

bool psd_has_layers(std::span<const std::byte> bytes) {
    BigEndianReader file(bytes);
    file.skip(26);
    file.skip(file.u32());
    file.skip(file.u32());
    const std::uint32_t layer_mask_size = file.u32();
    if (layer_mask_size == 0) return false;
    BigEndianReader layer_mask(file.take(layer_mask_size));
    if (layer_mask.remaining() < 6) return false;
    const std::uint32_t layer_info_size = layer_mask.u32();
    if (layer_info_size < 2) return false;
    BigEndianReader layer_info(layer_mask.take(layer_info_size));
    return layer_info.i16() != 0;
}

class MultipartHeaders {
public:
    ~MultipartHeaders() {
        for (int index = 0; index < count_; ++index) {
            FreeEXRHeader(headers_[index]);
            std::free(headers_[index]);
        }
        std::free(headers_);
    }
    MultipartHeaders(const MultipartHeaders&) = delete;
    MultipartHeaders& operator=(const MultipartHeaders&) = delete;
    MultipartHeaders() = default;
    EXRHeader*** output() noexcept { return &headers_; }
    int* count_output() noexcept { return &count_; }
    [[nodiscard]] int count() const noexcept { return count_; }
    [[nodiscard]] EXRHeader* operator[](std::size_t index) noexcept { return headers_[index]; }
    [[nodiscard]] const EXRHeader** data() noexcept {
        return const_cast<const EXRHeader**>(headers_);
    }

private:
    EXRHeader** headers_{};
    int count_{};
};

class ExrImages {
public:
    explicit ExrImages(std::size_t count) : values_(count) {
        for (EXRImage& value : values_) InitEXRImage(&value);
    }
    ~ExrImages() {
        for (EXRImage& value : values_) FreeEXRImage(&value);
    }
    ExrImages(const ExrImages&) = delete;
    ExrImages& operator=(const ExrImages&) = delete;
    EXRImage* data() noexcept { return values_.data(); }
    EXRImage& operator[](std::size_t index) noexcept { return values_[index]; }

private:
    std::vector<EXRImage> values_;
};

class ExrError {
public:
    ~ExrError() {
        if (message_ != nullptr) FreeEXRErrorMessage(message_);
    }
    const char** output() noexcept { return &message_; }
    [[nodiscard]] std::string message() const {
        return message_ == nullptr ? "unknown TinyEXR error" : message_;
    }

private:
    const char* message_{};
};

std::pair<std::uint32_t, std::uint32_t> exr_dimensions(const EXRHeader& header) {
    const std::int64_t width =
        static_cast<std::int64_t>(header.data_window.max_x) - header.data_window.min_x + 1;
    const std::int64_t height =
        static_cast<std::int64_t>(header.data_window.max_y) - header.data_window.min_y + 1;
    if (width <= 0 || height <= 0 || width > std::numeric_limits<std::uint32_t>::max() ||
        height > std::numeric_limits<std::uint32_t>::max()) {
        fail(ImageIoErrorCode::malformed_input, ImageFileFormat::openexr,
             "OpenEXR part has an invalid data window");
    }
    return {static_cast<std::uint32_t>(width), static_cast<std::uint32_t>(height)};
}

std::string_view exr_channel_component(std::string_view name) {
    const std::size_t separator = name.find_last_of('.');
    return separator == std::string_view::npos ? name : name.substr(separator + 1);
}

std::optional<std::size_t> exr_channel(const EXRHeader& header, std::string_view component) {
    for (int index = 0; index < header.num_channels; ++index) {
        if (exr_channel_component(header.channels[index].name) == component) {
            return static_cast<std::size_t>(index);
        }
    }
    return std::nullopt;
}

float exr_sample(const EXRImage& image, const EXRHeader& header, std::size_t channel,
                 std::uint32_t x, std::uint32_t y) {
    if (header.pixel_types[channel] != TINYEXR_PIXELTYPE_FLOAT) {
        fail(ImageIoErrorCode::unsupported_pixel_format, ImageFileFormat::openexr,
             "OpenEXR layered color channels must be floating point");
    }
    if (image.images != nullptr) {
        return reinterpret_cast<const float*>(
            image.images[channel])[static_cast<std::size_t>(y) * image.width + x];
    }
    for (int index = 0; index < image.num_tiles; ++index) {
        const EXRTile& tile = image.tiles[index];
        if (tile.level_x != 0 || tile.level_y != 0) continue;
        const std::uint32_t tile_x = static_cast<std::uint32_t>(tile.offset_x * header.tile_size_x);
        const std::uint32_t tile_y = static_cast<std::uint32_t>(tile.offset_y * header.tile_size_y);
        if (x >= tile_x && y >= tile_y && x < tile_x + static_cast<std::uint32_t>(tile.width) &&
            y < tile_y + static_cast<std::uint32_t>(tile.height)) {
            const std::size_t local = static_cast<std::size_t>(y - tile_y) * tile.width +
                                      static_cast<std::size_t>(x - tile_x);
            return reinterpret_cast<const float*>(tile.images[channel])[local];
        }
    }
    fail(ImageIoErrorCode::decode_failed, ImageFileFormat::openexr,
         "OpenEXR tiled part is missing a base-level pixel");
}

std::vector<std::byte> interleave_exr_part(const EXRImage& image, const EXRHeader& header,
                                           std::uint32_t width, std::uint32_t height) {
    const auto red = exr_channel(header, "R");
    const auto green = exr_channel(header, "G");
    const auto blue = exr_channel(header, "B");
    const auto luminance = exr_channel(header, "Y");
    const auto alpha = exr_channel(header, "A");
    if ((!red || !green || !blue) && !luminance) {
        fail(ImageIoErrorCode::unsupported_pixel_format, ImageFileFormat::openexr,
             "OpenEXR part has neither RGB nor luminance color channels");
    }
    std::vector<std::byte> result(
        checked_multiply(checked_multiply(width, height, ImageFileFormat::openexr),
                         sizeof(float) * 4, ImageFileFormat::openexr));
    auto* output = reinterpret_cast<float*>(result.data());
    for (std::uint32_t y = 0; y < height; ++y) {
        for (std::uint32_t x = 0; x < width; ++x) {
            const std::size_t pixel = static_cast<std::size_t>(y) * width + x;
            const float gray = luminance ? exr_sample(image, header, *luminance, x, y) : 0.0F;
            output[pixel * 4] = red ? exr_sample(image, header, *red, x, y) : gray;
            output[pixel * 4 + 1] = green ? exr_sample(image, header, *green, x, y) : gray;
            output[pixel * 4 + 2] = blue ? exr_sample(image, header, *blue, x, y) : gray;
            output[pixel * 4 + 3] = alpha ? exr_sample(image, header, *alpha, x, y) : 1.0F;
        }
    }
    return result;
}

std::vector<std::byte> composite_exr_parts(std::span<const DecodedImageLayer> parts,
                                           std::int32_t origin_x, std::int32_t origin_y,
                                           ImageLayout layout) {
    std::vector<std::byte> result(
        checked_multiply(checked_multiply(layout.width, layout.height, ImageFileFormat::openexr),
                         rgba32f.bytes_per_pixel(), ImageFileFormat::openexr));
    auto* output = reinterpret_cast<float*>(result.data());
    for (const DecodedImageLayer& part : parts) {
        for (std::uint32_t y = 0; y < part.image.pixels.height(); ++y) {
            for (std::uint32_t x = 0; x < part.image.pixels.width(); ++x) {
                const std::uint32_t destination_x =
                    static_cast<std::uint32_t>(part.origin_x - origin_x) + x;
                const std::uint32_t destination_y =
                    static_cast<std::uint32_t>(part.origin_y - origin_y) + y;
                const auto source_bytes = part.image.pixels.read_pixel(x, y);
                std::array<float, 4> source{};
                std::memcpy(source.data(), source_bytes.data(), sizeof(source));
                float* destination =
                    output +
                    (static_cast<std::size_t>(destination_y) * layout.width + destination_x) * 4;
                const float source_alpha = std::clamp(source[3], 0.0F, 1.0F);
                const float destination_alpha = std::clamp(destination[3], 0.0F, 1.0F);
                const float output_alpha = source_alpha + destination_alpha * (1.0F - source_alpha);
                for (std::size_t channel = 0; channel < 3; ++channel) {
                    destination[channel] =
                        output_alpha == 0.0F
                            ? 0.0F
                            : (source[channel] * source_alpha +
                               destination[channel] * destination_alpha * (1.0F - source_alpha)) /
                                  output_alpha;
                }
                destination[3] = output_alpha;
            }
        }
    }
    return result;
}

LayeredDecodedImage decode_exr_parts(const LayeredDecodeRequest& request) {
    const auto* bytes = reinterpret_cast<const unsigned char*>(request.image.bytes.data());
    EXRVersion version{};
    if (ParseEXRVersionFromMemory(&version, bytes, request.image.bytes.size()) != TINYEXR_SUCCESS) {
        fail(ImageIoErrorCode::malformed_input, ImageFileFormat::openexr,
             "OpenEXR version inspection failed");
    }
    if (version.non_image != 0) {
        fail(ImageIoErrorCode::unsupported_format, ImageFileFormat::openexr,
             "deep OpenEXR parts are not image layers");
    }
    if (version.multipart == 0) {
        DecodedImage decoded = decode_image_memory(request.image);
        return {.format = ImageFileFormat::openexr,
                .source_was_layered = false,
                .images = {
                    {.name = "Image", .origin_x = 0, .origin_y = 0, .image = std::move(decoded)}}};
    }
    MultipartHeaders headers;
    ExrError parse_error;
    if (ParseEXRMultipartHeaderFromMemory(headers.output(), headers.count_output(), &version, bytes,
                                          request.image.bytes.size(),
                                          parse_error.output()) != TINYEXR_SUCCESS) {
        fail(ImageIoErrorCode::malformed_input, ImageFileFormat::openexr,
             "OpenEXR multipart header inspection failed: " + parse_error.message());
    }
    if (headers.count() <= 0 ||
        static_cast<std::size_t>(headers.count()) > request.maximum_image_count) {
        fail(ImageIoErrorCode::over_limit, ImageFileFormat::openexr,
             "OpenEXR part count is zero or exceeds maximum_image_count");
    }
    std::vector<ImageLayout> layouts;
    layouts.reserve(static_cast<std::size_t>(headers.count()));
    std::size_t codec_planar_bytes = 0;
    std::int64_t min_x = std::numeric_limits<std::int64_t>::max();
    std::int64_t min_y = std::numeric_limits<std::int64_t>::max();
    std::int64_t max_x = std::numeric_limits<std::int64_t>::min();
    std::int64_t max_y = std::numeric_limits<std::int64_t>::min();
    for (int index = 0; index < headers.count(); ++index) {
        EXRHeader& header = *headers[static_cast<std::size_t>(index)];
        const auto [width, height] = exr_dimensions(header);
        if (header.num_channels <= 0) {
            fail(ImageIoErrorCode::malformed_input, ImageFileFormat::openexr,
                 "OpenEXR part has no channels");
        }
        layouts.push_back({width, height, rgba32f});
        std::size_t part_planar_bytes =
            checked_multiply(checked_multiply(width, height, ImageFileFormat::openexr),
                             checked_multiply(static_cast<std::size_t>(header.num_channels),
                                              sizeof(float), ImageFileFormat::openexr),
                             ImageFileFormat::openexr);
        if (header.tiled != 0 && header.tile_level_mode != TINYEXR_TILE_ONE_LEVEL) {
            // Four times the base level bounds the full 2D geometric series, including ripmaps.
            part_planar_bytes = checked_multiply(part_planar_bytes, 4, ImageFileFormat::openexr);
        }
        codec_planar_bytes =
            checked_add(codec_planar_bytes, part_planar_bytes, ImageFileFormat::openexr);
        min_x = std::min(min_x, static_cast<std::int64_t>(header.data_window.min_x));
        min_y = std::min(min_y, static_cast<std::int64_t>(header.data_window.min_y));
        max_x = std::max(max_x, static_cast<std::int64_t>(header.data_window.max_x) + 1);
        max_y = std::max(max_y, static_cast<std::int64_t>(header.data_window.max_y) + 1);
        for (int channel = 0; channel < header.num_channels; ++channel) {
            if (header.pixel_types[channel] == TINYEXR_PIXELTYPE_HALF) {
                header.requested_pixel_types[channel] = TINYEXR_PIXELTYPE_FLOAT;
            }
        }
    }
    const std::int64_t composite_width = max_x - min_x;
    const std::int64_t composite_height = max_y - min_y;
    if (composite_width <= 0 || composite_height <= 0 ||
        composite_width > std::numeric_limits<std::uint32_t>::max() ||
        composite_height > std::numeric_limits<std::uint32_t>::max()) {
        fail(ImageIoErrorCode::over_limit, ImageFileFormat::openexr,
             "OpenEXR multipart union exceeds the supported image range");
    }
    const ImageLayout composite_layout{static_cast<std::uint32_t>(composite_width),
                                       static_cast<std::uint32_t>(composite_height), rgba32f};
    std::vector<ImageLayout> admitted_layouts = layouts;
    if (request.mode == LayeredDecodeMode::composite) {
        admitted_layouts.push_back(composite_layout);
    }
    LayerDecodeMonitor monitor(request, ImageFileFormat::openexr);
    monitor.preflight(admitted_layouts, codec_planar_bytes);
    ExrImages images(static_cast<std::size_t>(headers.count()));
    ExrError decode_error;
    monitor.codec_started();
    if (LoadEXRMultipartImageFromMemory(
            images.data(), headers.data(), static_cast<unsigned>(headers.count()), bytes,
            request.image.bytes.size(), decode_error.output()) != TINYEXR_SUCCESS) {
        fail(ImageIoErrorCode::decode_failed, ImageFileFormat::openexr,
             "OpenEXR multipart decode failed: " + decode_error.message());
    }
    std::vector<std::vector<std::byte>> packed;
    packed.reserve(static_cast<std::size_t>(headers.count()));
    for (int index = 0; index < headers.count(); ++index) {
        packed.push_back(interleave_exr_part(images[static_cast<std::size_t>(index)],
                                             *headers[static_cast<std::size_t>(index)],
                                             layouts[static_cast<std::size_t>(index)].width,
                                             layouts[static_cast<std::size_t>(index)].height));
    }
    monitor.codec_finished();
    const auto [color_space, color_source] = layered_color_space(request.image, true);
    LayeredDecodedImage result{
        .format = ImageFileFormat::openexr, .source_was_layered = true, .images = {}};
    result.images.reserve(static_cast<std::size_t>(headers.count()));
    for (int index = 0; index < headers.count(); ++index) {
        const std::size_t part = static_cast<std::size_t>(index);
        const EXRHeader& header = *headers[part];
        result.images.push_back(
            {.name = header.name,
             .origin_x = header.data_window.min_x,
             .origin_y = header.data_window.min_y,
             .image = {tiled_from_interleaved(packed[part], layouts[part], monitor), color_space,
                       make_report(request.image, ImageFileFormat::openexr, color_source)}});
    }
    if (request.mode == LayeredDecodeMode::composite) {
        std::vector<std::byte> composite =
            composite_exr_parts(result.images, static_cast<std::int32_t>(min_x),
                                static_cast<std::int32_t>(min_y), composite_layout);
        DecodedImageLayer output{
            .name = "Composite",
            .origin_x = static_cast<std::int32_t>(min_x),
            .origin_y = static_cast<std::int32_t>(min_y),
            .image = {tiled_from_interleaved(composite, composite_layout, monitor), color_space,
                      make_report(request.image, ImageFileFormat::openexr, color_source)},
        };
        result.images.clear();
        result.images.push_back(std::move(output));
    }
    monitor.finish(result.images);
    return result;
}

LayeredDecodedImage composite_flat(const LayeredDecodeRequest& request, ImageFileFormat format,
                                   bool source_was_layered) {
    DecodedImage decoded = decode_image_memory(request.image);
    return {.format = format,
            .source_was_layered = source_was_layered,
            .images = {
                {.name = "Composite", .origin_x = 0, .origin_y = 0, .image = std::move(decoded)}}};
}

}  // namespace

LayeredDecodedImage decode_layered_image_memory(const LayeredDecodeRequest& request) {
    if (request.maximum_image_count == 0) {
        fail(ImageIoErrorCode::over_limit, ImageFileFormat::unknown,
             "maximum_image_count must be non-zero");
    }
    const ImageFileFormat format = detect_image_format(request.image.bytes);
    if (format != ImageFileFormat::psd && format != ImageFileFormat::openexr) {
        fail(ImageIoErrorCode::unsupported_format, format,
             "layered decode requires PSD or OpenEXR content");
    }
    if (request.mode == LayeredDecodeMode::composite) {
        if (format == ImageFileFormat::psd) {
            return composite_flat(request, format, psd_has_layers(request.image.bytes));
        }
        return decode_exr_parts(request);
    }
    return format == ImageFileFormat::psd ? decode_psd_layers(request) : decode_exr_parts(request);
}

}  // namespace ctex::io
