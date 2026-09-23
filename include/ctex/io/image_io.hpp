#ifndef CTEX_IO_IMAGE_IO_HPP
#define CTEX_IO_IMAGE_IO_HPP

#include <cstddef>
#include <cstdint>
#include <ctex/image/color_policy.hpp>
#include <ctex/image/tiled_image.hpp>
#include <functional>
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
    cancelled,
};

enum class ColorSpaceSource { caller, embedded_srgb, automatic_rule, embedded_profile };

struct DecodeLimits {
    std::uint32_t maximum_width = 16'384;
    std::uint32_t maximum_height = 16'384;
    std::size_t maximum_decoded_bytes = 1ULL << 30;
};

enum class DecodePhase : std::uint8_t { inspection, codec, unpack, complete };

struct DecodeProgress {
    DecodePhase phase{};
    std::uint32_t completed_rows{};
    std::uint32_t total_rows{};
    std::size_t estimated_peak_working_bytes{};
};

inline constexpr std::size_t default_decode_working_byte_limit =
    sizeof(std::size_t) >= 8 ? (4ULL << 30) : (2ULL << 30);

struct DecodeControl {
    std::size_t maximum_working_bytes = default_decode_working_byte_limit;
    std::uint32_t progress_interval_rows = 64;
    std::function<bool()> is_cancelled;
    std::function<void(const DecodeProgress&)> report_progress;
};

struct DecodeRequest {
    std::span<const std::byte> bytes;
    std::string_view source_name;
    image::ChannelSemantic intended_channel = image::ChannelSemantic::base_color;
    image::InputColorSpace color_space = image::InputColorSpace::automatic;
    DecodeLimits limits{};
    DecodeControl control{};
};

struct DecodeReport {
    ImageFileFormat detected_format;
    bool extension_mismatch;
    ColorSpaceSource color_space_source;
    std::vector<std::string> diagnostics;
    std::size_t estimated_peak_working_bytes{};
    std::size_t progress_event_count{};
};

struct DecodedImage {
    image::TiledImage pixels;
    image::ColorSpace source_color_space;
    DecodeReport report;
};

enum class LayeredDecodeMode : std::uint8_t { composite, individual };

struct LayeredDecodeRequest {
    DecodeRequest image;
    LayeredDecodeMode mode = LayeredDecodeMode::composite;
    std::size_t maximum_image_count = 256;
};

struct DecodedImageLayer {
    std::string name;
    std::int32_t origin_x{};
    std::int32_t origin_y{};
    DecodedImage image;
};

struct LayeredDecodedImage {
    ImageFileFormat format{};
    bool source_was_layered{};
    std::vector<DecodedImageLayer> images;
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
[[nodiscard]] LayeredDecodedImage decode_layered_image_memory(const LayeredDecodeRequest& request);
[[nodiscard]] std::vector<std::byte> encode_png_memory(const image::TiledImage& image,
                                                       PngEncodeOptions options = {});

}  // namespace ctex::io

#endif
