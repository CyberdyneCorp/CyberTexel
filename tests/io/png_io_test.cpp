#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <ctex/io/image_io.hpp>
#include <ctex/io/texture_encode.hpp>
#include <exception>
#include <iostream>
#include <span>
#include <string>
#include <string_view>

namespace {

using ctex::image::ChannelSemantic;
using ctex::image::ChannelType;
using ctex::image::ColorSpace;
using ctex::image::InputColorSpace;
using ctex::image::PixelFormat;
using ctex::image::TiledImage;
using ctex::io::ColorSpaceSource;
using ctex::io::DecodeLimits;
using ctex::io::DecodeRequest;
using ctex::io::ImageFileFormat;
using ctex::io::ImageIoError;
using ctex::io::ImageIoErrorCode;

bool expect(bool condition, std::string_view message) {
    if (!condition) {
        std::cerr << message << '\n';
    }
    return condition;
}

template <typename Callable>
bool expect_error(Callable&& callable, ImageIoErrorCode code, std::string_view message_part,
                  ImageFileFormat format = ImageFileFormat::unknown) {
    try {
        callable();
    } catch (const ImageIoError& error) {
        return expect(error.code() == code, "wrong image IO error code") &&
               expect(format == ImageFileFormat::unknown || error.format() == format,
                      "wrong image IO error format") &&
               expect(std::string_view(error.what()).find(message_part) != std::string_view::npos,
                      "image IO diagnostic did not name the failure");
    } catch (...) {
    }
    return expect(false, "expected image IO error was not thrown");
}

std::array<std::byte, 2> bytes_of(std::uint16_t value) {
    std::array<std::byte, 2> bytes{};
    std::memcpy(bytes.data(), &value, sizeof(value));
    return bytes;
}

std::uint16_t uint16_from(std::span<const std::byte> bytes) {
    std::uint16_t value = 0;
    std::memcpy(&value, bytes.data(), sizeof(value));
    return value;
}

float float_from(std::span<const std::byte> bytes, std::size_t component) {
    float value = 0.0F;
    std::memcpy(&value, bytes.data() + component * sizeof(value), sizeof(value));
    return value;
}

std::vector<std::byte> make_radiance_hdr() {
    constexpr std::string_view header = "#?RADIANCE\nFORMAT=32-bit_rle_rgbe\n\n-Y 1 +X 2\n";
    std::vector<std::byte> result;
    result.reserve(header.size() + 8);
    std::transform(header.begin(), header.end(), std::back_inserter(result),
                   [](char value) { return static_cast<std::byte>(value); });
    constexpr std::array pixels{
        std::byte{128}, std::byte{64}, std::byte{32},  std::byte{130},
        std::byte{32},  std::byte{64}, std::byte{128}, std::byte{129},
    };
    result.insert(result.end(), pixels.begin(), pixels.end());
    return result;
}

std::vector<std::byte> make_gray8_png(ColorSpace color_space = ColorSpace::linear_rec709) {
    TiledImage source(2, 1, PixelFormat{ChannelType::uint8_unorm, 1});
    const std::array first{std::byte{17}};
    const std::array second{std::byte{230}};
    source.write_pixel(0, 0, first);
    source.write_pixel(1, 0, second);
    return ctex::io::encode_png_memory(source, {.color_space = color_space});
}

bool grayscale_memory_decode_and_mismatch() {
    const std::vector<std::byte> encoded = make_gray8_png();
    const auto decoded = ctex::io::decode_image_memory({
        .bytes = encoded,
        .source_name = "brush.jpg",
        .intended_channel = ChannelSemantic::roughness,
    });
    return expect(decoded.report.detected_format == ImageFileFormat::png,
                  "PNG not detected by content") &&
           expect(decoded.report.extension_mismatch, "extension mismatch was not reported") &&
           expect(decoded.pixels.format() == PixelFormat{ChannelType::uint8_unorm, 1},
                  "grayscale channel count or depth changed") &&
           expect(decoded.pixels.read_pixel(0, 0)[0] == std::byte{17},
                  "first grayscale pixel changed") &&
           expect(decoded.pixels.read_pixel(1, 0)[0] == std::byte{230},
                  "second grayscale pixel changed") &&
           expect(decoded.source_color_space == ColorSpace::linear_rec709,
                  "data channel automatic color rule was not applied") &&
           expect(decoded.report.color_space_source == ColorSpaceSource::automatic_rule,
                  "automatic color source was not reported");
}

bool sixteen_bit_round_trip() {
    TiledImage source(2, 1, PixelFormat{ChannelType::uint16_unorm, 1});
    const auto first = bytes_of(0x1234);
    const auto second = bytes_of(0xabcd);
    source.write_pixel(0, 0, first);
    source.write_pixel(1, 0, second);
    const std::vector<std::byte> encoded = ctex::io::encode_png_memory(source);
    const auto decoded = ctex::io::decode_image_memory({
        .bytes = encoded,
        .source_name = "height.png",
        .intended_channel = ChannelSemantic::height,
    });
    return expect(decoded.pixels.format() == PixelFormat{ChannelType::uint16_unorm, 1},
                  "16-bit PNG was reduced") &&
           expect(uint16_from(decoded.pixels.read_pixel(0, 0)) == 0x1234,
                  "first 16-bit sample changed") &&
           expect(uint16_from(decoded.pixels.read_pixel(1, 0)) == 0xabcd,
                  "second 16-bit sample changed");
}

bool embedded_space_and_caller_override() {
    const std::vector<std::byte> encoded = make_gray8_png(ColorSpace::srgb_rec709);
    const auto embedded = ctex::io::decode_image_memory({
        .bytes = encoded,
        .source_name = "base.png",
        .intended_channel = ChannelSemantic::base_color,
    });
    const auto overridden = ctex::io::decode_image_memory({
        .bytes = encoded,
        .source_name = "base.png",
        .intended_channel = ChannelSemantic::base_color,
        .color_space = InputColorSpace::linear_rec709,
    });
    return expect(embedded.source_color_space == ColorSpace::srgb_rec709,
                  "embedded sRGB was ignored") &&
           expect(embedded.report.color_space_source == ColorSpaceSource::embedded_srgb,
                  "embedded source was not reported") &&
           expect(overridden.source_color_space == ColorSpace::linear_rec709,
                  "caller declaration did not override profile") &&
           expect(overridden.report.color_space_source == ColorSpaceSource::caller,
                  "caller source was not reported");
}

bool hostile_input_is_bounded_and_named() {
    const std::vector<std::byte> encoded = make_gray8_png();
    const bool bounded = expect_error(
        [&] {
            static_cast<void>(ctex::io::decode_image_memory({
                .bytes = encoded,
                .source_name = "large.png",
                .limits =
                    DecodeLimits{
                        .maximum_width = 1, .maximum_height = 1, .maximum_decoded_bytes = 1},
            }));
        },
        ImageIoErrorCode::over_limit, "2x1");
    std::vector<std::byte> truncated = encoded;
    truncated.resize(truncated.size() / 2);
    const bool truncation = expect_error(
        [&] {
            static_cast<void>(ctex::io::decode_image_memory({
                .bytes = truncated,
                .source_name = "truncated.png",
            }));
        },
        ImageIoErrorCode::decode_failed, "PNG decode failed");
    return bounded && truncation;
}

bool unsupported_content_is_named() {
    const std::array jpeg{std::byte{0xff}, std::byte{0xd8}, std::byte{0xff}, std::byte{0x00}};
    return expect_error(
        [&] {
            static_cast<void>(ctex::io::decode_image_memory({
                .bytes = jpeg,
                .source_name = "photo.bin",
            }));
        },
        ImageIoErrorCode::unsupported_format, "JPEG");
}

bool float_png_is_refused() {
    TiledImage source(1, 1, PixelFormat{ChannelType::float32, 1});
    return expect_error([&] { static_cast<void>(ctex::io::encode_png_memory(source)); },
                        ImageIoErrorCode::unsupported_pixel_format, "32-bit floating-point");
}

bool radiance_hdr_preserves_unclamped_float_values() {
    const std::vector<std::byte> encoded = make_radiance_hdr();
    const auto decoded = ctex::io::decode_image_memory({
        .bytes = encoded,
        .source_name = "environment.png",
        .intended_channel = ChannelSemantic::base_color,
    });
    const auto first = decoded.pixels.read_pixel(0, 0);
    const auto second = decoded.pixels.read_pixel(1, 0);
    return expect(decoded.report.detected_format == ImageFileFormat::radiance_hdr,
                  "Radiance HDR was not detected by content") &&
           expect(decoded.report.extension_mismatch,
                  "Radiance HDR extension mismatch was not reported") &&
           expect(decoded.pixels.format() == PixelFormat{ChannelType::float32, 3},
                  "Radiance HDR did not decode to RGB float32") &&
           expect(float_from(first, 0) == 2.0F && float_from(first, 1) == 1.0F &&
                      float_from(first, 2) == 0.5F,
                  "Radiance HDR values above one were changed") &&
           expect(float_from(second, 0) == 0.25F && float_from(second, 1) == 0.5F &&
                      float_from(second, 2) == 1.0F,
                  "Radiance HDR values were not decoded exactly") &&
           expect(decoded.source_color_space == ColorSpace::linear_rec709 &&
                      decoded.report.color_space_source == ColorSpaceSource::automatic_rule,
                  "Radiance HDR automatic linear colour space was not reported");
}

bool openexr_preserves_unclamped_float_values() {
    TiledImage source(1, 1, PixelFormat{ChannelType::float32, 4});
    constexpr std::array values{4.0F, 2.0F, 0.5F, 1.0F};
    std::array<std::byte, sizeof(values)> pixel{};
    std::memcpy(pixel.data(), values.data(), pixel.size());
    source.write_pixel(0, 0, pixel);
    const std::vector<std::byte> encoded =
        ctex::io::encode_texture_memory(source, {.format = ctex::io::ExportImageFormat::openexr,
                                                 .bit_depth = ctex::io::ExportBitDepth::bits_32,
                                                 .color_space = ColorSpace::linear_rec709});
    const auto decoded = ctex::io::decode_image_memory({
        .bytes = encoded,
        .source_name = "environment.exr",
        .intended_channel = ChannelSemantic::base_color,
    });
    const auto decoded_pixel = decoded.pixels.read_pixel(0, 0);
    const bool bounded = expect_error(
        [&] {
            static_cast<void>(ctex::io::decode_image_memory({
                .bytes = encoded,
                .source_name = "limited.exr",
                .limits =
                    DecodeLimits{
                        .maximum_width = 1, .maximum_height = 1, .maximum_decoded_bytes = 15},
            }));
        },
        ImageIoErrorCode::over_limit, "1x1", ImageFileFormat::openexr);
    std::vector<std::byte> truncated = encoded;
    truncated.resize(truncated.size() - 4);
    const bool truncation = expect_error(
        [&] {
            static_cast<void>(ctex::io::decode_image_memory({
                .bytes = truncated,
                .source_name = "truncated.exr",
            }));
        },
        ImageIoErrorCode::decode_failed, "OpenEXR decode failed");
    return expect(decoded.report.detected_format == ImageFileFormat::openexr,
                  "OpenEXR was not detected by content") &&
           expect(decoded.pixels.format() == PixelFormat{ChannelType::float32, 4},
                  "OpenEXR did not decode to RGBA float32") &&
           expect(float_from(decoded_pixel, 0) == 4.0F && float_from(decoded_pixel, 1) == 2.0F &&
                      float_from(decoded_pixel, 2) == 0.5F && float_from(decoded_pixel, 3) == 1.0F,
                  "OpenEXR values above one were changed") &&
           bounded && truncation;
}

bool hdr_limits_are_checked_before_decode() {
    const std::vector<std::byte> encoded = make_radiance_hdr();
    return expect_error(
        [&] {
            static_cast<void>(ctex::io::decode_image_memory({
                .bytes = encoded,
                .source_name = "large.hdr",
                .limits =
                    DecodeLimits{
                        .maximum_width = 1, .maximum_height = 1, .maximum_decoded_bytes = 4},
            }));
        },
        ImageIoErrorCode::over_limit, "2x1", ImageFileFormat::radiance_hdr);
}

}  // namespace

int main() {
    return grayscale_memory_decode_and_mismatch() && sixteen_bit_round_trip() &&
                   embedded_space_and_caller_override() && hostile_input_is_bounded_and_named() &&
                   unsupported_content_is_named() && float_png_is_refused() &&
                   radiance_hdr_preserves_unclamped_float_values() &&
                   openexr_preserves_unclamped_float_values() &&
                   hdr_limits_are_checked_before_decode()
               ? 0
               : 1;
}
