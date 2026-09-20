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

TiledImage red_green_rgb8() {
    TiledImage source(2, 1, PixelFormat{ChannelType::uint8_unorm, 3});
    const std::array red{std::byte{255}, std::byte{0}, std::byte{0}};
    const std::array green{std::byte{0}, std::byte{255}, std::byte{0}};
    source.write_pixel(0, 0, red);
    source.write_pixel(1, 0, green);
    return source;
}

void append_be16(std::vector<std::byte>& bytes, std::uint16_t value) {
    bytes.push_back(static_cast<std::byte>(value >> 8U));
    bytes.push_back(static_cast<std::byte>(value));
}

void append_be32(std::vector<std::byte>& bytes, std::uint32_t value) {
    bytes.push_back(static_cast<std::byte>(value >> 24U));
    bytes.push_back(static_cast<std::byte>(value >> 16U));
    bytes.push_back(static_cast<std::byte>(value >> 8U));
    bytes.push_back(static_cast<std::byte>(value));
}

std::vector<std::byte> make_rgb8_psd() {
    std::vector<std::byte> bytes;
    for (const char value : std::string_view("8BPS")) {
        bytes.push_back(static_cast<std::byte>(value));
    }
    append_be16(bytes, 1);
    bytes.insert(bytes.end(), 6, std::byte{0});
    append_be16(bytes, 3);
    append_be32(bytes, 1);
    append_be32(bytes, 2);
    append_be16(bytes, 8);
    append_be16(bytes, 3);
    append_be32(bytes, 0);
    append_be32(bytes, 0);
    append_be32(bytes, 0);
    append_be16(bytes, 0);
    const std::array planar{std::byte{255}, std::byte{0}, std::byte{0},
                            std::byte{255}, std::byte{0}, std::byte{0}};
    bytes.insert(bytes.end(), planar.begin(), planar.end());
    return bytes;
}

std::vector<std::byte> make_rgb16_psd() {
    std::vector<std::byte> bytes;
    for (const char value : std::string_view("8BPS")) {
        bytes.push_back(static_cast<std::byte>(value));
    }
    append_be16(bytes, 1);
    bytes.insert(bytes.end(), 6, std::byte{0});
    append_be16(bytes, 3);
    append_be32(bytes, 1);
    append_be32(bytes, 1);
    append_be16(bytes, 16);
    append_be16(bytes, 3);
    append_be32(bytes, 0);
    append_be32(bytes, 0);
    append_be32(bytes, 0);
    append_be16(bytes, 0);
    append_be16(bytes, 0x1234);
    append_be16(bytes, 0x5678);
    append_be16(bytes, 0xabcd);
    return bytes;
}

std::vector<std::byte> make_rgb8_bmp() {
    constexpr std::array<std::uint8_t, 62> values{
        'B', 'M', 62, 0, 0, 0, 0, 0, 0,  0, 54, 0, 0, 0, 40,  0, 0,   0, 2, 0,
        0,   0,   1,  0, 0, 0, 1, 0, 24, 0, 0,  0, 0, 0, 8,   0, 0,   0, 0, 0,
        0,   0,   0,  0, 0, 0, 0, 0, 0,  0, 0,  0, 0, 0, 255, 0, 255, 0, 0, 0,
    };
    std::vector<std::byte> result;
    result.reserve(values.size());
    std::transform(values.begin(), values.end(), std::back_inserter(result),
                   [](std::uint8_t value) { return static_cast<std::byte>(value); });
    return result;
}

std::vector<std::byte> make_rgb8_rle_tga() {
    std::vector<std::byte> bytes(18, std::byte{0});
    bytes[2] = std::byte{10};
    bytes[12] = std::byte{2};
    bytes[14] = std::byte{1};
    bytes[16] = std::byte{24};
    bytes[17] = std::byte{0x20};
    const std::array payload{std::byte{1}, std::byte{0},   std::byte{0}, std::byte{255},
                             std::byte{0}, std::byte{255}, std::byte{0}};
    bytes.insert(bytes.end(), payload.begin(), payload.end());
    return bytes;
}

bool decoder_set_covers_flat_formats() {
    const TiledImage source = red_green_rgb8();
    const auto encode = [&](ctex::io::ExportImageFormat format) {
        return ctex::io::encode_texture_memory(source,
                                               {.format = format,
                                                .bit_depth = ctex::io::ExportBitDepth::bits_8,
                                                .color_space = ColorSpace::srgb_rec709,
                                                .jpeg_quality = 100});
    };
    const std::array encoded{
        std::pair{ImageFileFormat::jpeg, encode(ctex::io::ExportImageFormat::jpeg)},
        std::pair{ImageFileFormat::tga, encode(ctex::io::ExportImageFormat::tga)},
        std::pair{ImageFileFormat::tga, make_rgb8_rle_tga()},
        std::pair{ImageFileFormat::bmp, make_rgb8_bmp()},
        std::pair{ImageFileFormat::tiff, encode(ctex::io::ExportImageFormat::tiff)},
        std::pair{ImageFileFormat::psd, make_rgb8_psd()},
    };
    for (const auto& [format, bytes] : encoded) {
        const auto decoded = ctex::io::decode_image_memory({
            .bytes = bytes,
            .source_name = "mislabelled.bin",
            .intended_channel = ChannelSemantic::base_color,
        });
        const bool valid = decoded.report.detected_format == format &&
                           decoded.pixels.width() == 2 && decoded.pixels.height() == 1 &&
                           decoded.pixels.format() == PixelFormat{ChannelType::uint8_unorm, 3} &&
                           decoded.report.extension_mismatch;
        if (!valid) {
            std::cerr << "flat decoder expected format " << static_cast<int>(format)
                      << ", detected " << static_cast<int>(decoded.report.detected_format)
                      << ", dimensions " << decoded.pixels.width() << 'x' << decoded.pixels.height()
                      << ", channels "
                      << static_cast<unsigned>(decoded.pixels.format().channel_count) << '\n';
        }
        if (!expect(valid, "flat image decoder changed the format, layout, or mismatch report")) {
            return false;
        }
        if (!expect_error(
                [&] {
                    static_cast<void>(ctex::io::decode_image_memory({
                        .bytes = bytes,
                        .source_name = "limited.bin",
                        .limits = DecodeLimits{.maximum_width = 1,
                                               .maximum_height = 1,
                                               .maximum_decoded_bytes = 3},
                    }));
                },
                ImageIoErrorCode::over_limit, "2x1", format)) {
            return false;
        }
    }
    return true;
}

bool tiff_preserves_integer_and_float_precision() {
    TiledImage integer_source(2, 1, PixelFormat{ChannelType::uint16_unorm, 1});
    const auto first = bytes_of(0x1234);
    const auto second = bytes_of(0xabcd);
    integer_source.write_pixel(0, 0, first);
    integer_source.write_pixel(1, 0, second);
    const auto integer_encoded = ctex::io::encode_texture_memory(
        integer_source, {.format = ctex::io::ExportImageFormat::tiff,
                         .bit_depth = ctex::io::ExportBitDepth::bits_16});
    const auto integer_decoded =
        ctex::io::decode_image_memory({.bytes = integer_encoded, .source_name = "height.tiff"});

    TiledImage float_source(1, 1, PixelFormat{ChannelType::float32, 1});
    constexpr float value = 4.5F;
    std::array<std::byte, sizeof(value)> pixel{};
    std::memcpy(pixel.data(), &value, sizeof(value));
    float_source.write_pixel(0, 0, pixel);
    const auto float_encoded = ctex::io::encode_texture_memory(
        float_source, {.format = ctex::io::ExportImageFormat::tiff,
                       .bit_depth = ctex::io::ExportBitDepth::bits_32});
    const auto float_decoded =
        ctex::io::decode_image_memory({.bytes = float_encoded, .source_name = "height.tif"});
    return expect(
               integer_decoded.report.detected_format == ImageFileFormat::tiff &&
                   integer_decoded.pixels.format() == PixelFormat{ChannelType::uint16_unorm, 1} &&
                   uint16_from(integer_decoded.pixels.read_pixel(0, 0)) == 0x1234 &&
                   uint16_from(integer_decoded.pixels.read_pixel(1, 0)) == 0xabcd,
               "TIFF decoder changed 16-bit integer samples") &&
           expect(float_decoded.pixels.format() == PixelFormat{ChannelType::float32, 1} &&
                      float_from(float_decoded.pixels.read_pixel(0, 0), 0) == value,
                  "TIFF decoder clamped or changed a floating-point sample");
}

bool psd_preserves_sixteen_bit_composite() {
    const std::vector<std::byte> encoded = make_rgb16_psd();
    const auto decoded = ctex::io::decode_image_memory({
        .bytes = encoded,
        .source_name = "composite.psd",
        .intended_channel = ChannelSemantic::base_color,
    });
    const auto pixel = decoded.pixels.read_pixel(0, 0);
    if (!expect(decoded.pixels.format() == PixelFormat{ChannelType::uint16_unorm, 3},
                "PSD decoder reduced its 16-bit composite layout")) {
        return false;
    }
    const std::uint16_t red = uint16_from(pixel.subspan(0, 2));
    const std::uint16_t green = uint16_from(pixel.subspan(2, 2));
    const std::uint16_t blue = uint16_from(pixel.subspan(4, 2));
    if (red != 0x1234 || green != 0x5678 || blue != 0xabcd) {
        std::cerr << "PSD16 decoded values: " << red << ", " << green << ", " << blue << '\n';
    }
    return expect(red == 0x1234 && green == 0x5678 && blue == 0xabcd,
                  "PSD decoder reduced or changed its 16-bit composite");
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
    const std::array gif{std::byte{'G'}, std::byte{'I'}, std::byte{'F'},
                         std::byte{'8'}, std::byte{'9'}, std::byte{'a'}};
    return expect_error(
        [&] {
            static_cast<void>(ctex::io::decode_image_memory({
                .bytes = gif,
                .source_name = "animation.gif",
            }));
        },
        ImageIoErrorCode::unsupported_format, "supported");
}

bool malformed_flat_inputs_are_named() {
    const std::array jpeg{std::byte{0xff}, std::byte{0xd8}, std::byte{0xff}, std::byte{0xe0}};
    const std::array bmp{std::byte{'B'}, std::byte{'M'}};
    const std::array tiff{std::byte{'I'}, std::byte{'I'}, std::byte{42}, std::byte{0},
                          std::byte{8},   std::byte{0},   std::byte{0},  std::byte{0}};
    const std::array psd{std::byte{'8'}, std::byte{'B'}, std::byte{'P'}, std::byte{'S'}};
    std::array<std::byte, 18> tga{};
    tga[2] = std::byte{2};
    tga[12] = std::byte{1};
    tga[14] = std::byte{1};
    tga[16] = std::byte{24};
    const auto malformed = [&](std::span<const std::byte> bytes, std::string_view name,
                               ImageFileFormat format) {
        const bool result = expect_error(
            [&] {
                static_cast<void>(ctex::io::decode_image_memory({
                    .bytes = bytes,
                    .source_name = name,
                }));
            },
            ImageIoErrorCode::malformed_input, ctex::io::image_file_format_name(format), format);
        if (!result) std::cerr << "malformed fixture was not refused as " << name << '\n';
        return result;
    };
    return malformed(jpeg, "broken.jpg", ImageFileFormat::jpeg) &&
           malformed(bmp, "broken.bmp", ImageFileFormat::bmp) &&
           malformed(tiff, "broken.tif", ImageFileFormat::tiff) &&
           malformed(psd, "broken.psd", ImageFileFormat::psd) &&
           malformed(tga, "broken.tga", ImageFileFormat::tga);
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
                   decoder_set_covers_flat_formats() &&
                   tiff_preserves_integer_and_float_precision() &&
                   psd_preserves_sixteen_bit_composite() && embedded_space_and_caller_override() &&
                   hostile_input_is_bounded_and_named() && unsupported_content_is_named() &&
                   malformed_flat_inputs_are_named() && float_png_is_refused() &&
                   radiance_hdr_preserves_unclamped_float_values() &&
                   openexr_preserves_unclamped_float_values() &&
                   hdr_limits_are_checked_before_decode()
               ? 0
               : 1;
}
