#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <ctex/io/texture_encode.hpp>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace {

using namespace ctex;
using namespace ctex::io;

bool expect(bool condition, std::string_view message) {
    if (!condition) {
        std::cerr << message << '\n';
    }
    return condition;
}

std::uint8_t byte(std::span<const std::byte> bytes, std::size_t offset) {
    return std::to_integer<std::uint8_t>(bytes[offset]);
}

std::uint16_t little_u16(std::span<const std::byte> bytes, std::size_t offset) {
    return static_cast<std::uint16_t>(byte(bytes, offset) | (byte(bytes, offset + 1) << 8U));
}

std::uint32_t little_u32(std::span<const std::byte> bytes, std::size_t offset) {
    std::uint32_t value = 0;
    for (unsigned index = 0; index < 4; ++index) {
        value |= static_cast<std::uint32_t>(byte(bytes, offset + index)) << (index * 8U);
    }
    return value;
}

image::TiledImage sample_image(std::uint8_t channels = 4) {
    image::TiledImage result(2, 2, {image::ChannelType::uint8_unorm, channels});
    constexpr std::array<std::array<std::byte, 4>, 4> pixels{{
        {std::byte{255}, std::byte{0}, std::byte{0}, std::byte{255}},
        {std::byte{0}, std::byte{255}, std::byte{0}, std::byte{192}},
        {std::byte{0}, std::byte{0}, std::byte{255}, std::byte{128}},
        {std::byte{64}, std::byte{128}, std::byte{192}, std::byte{32}},
    }};
    for (std::uint32_t y = 0; y < result.height(); ++y) {
        for (std::uint32_t x = 0; x < result.width(); ++x) {
            result.write_pixel(x, y, std::span(pixels[y * result.width() + x]).first(channels));
        }
    }
    return result;
}

bool valid_signature(ExportImageFormat format, ExportBitDepth depth,
                     std::span<const std::byte> bytes) {
    switch (format) {
        case ExportImageFormat::png:
            return bytes.size() > 24 && byte(bytes, 0) == 0x89 && byte(bytes, 1) == 'P' &&
                   byte(bytes, 2) == 'N' && byte(bytes, 3) == 'G' &&
                   byte(bytes, 24) == static_cast<unsigned>(depth);
        case ExportImageFormat::jpeg:
            return bytes.size() >= 4 && byte(bytes, 0) == 0xff && byte(bytes, 1) == 0xd8 &&
                   byte(bytes, bytes.size() - 2) == 0xff && byte(bytes, bytes.size() - 1) == 0xd9;
        case ExportImageFormat::tga:
            return bytes.size() > 18 && (byte(bytes, 2) == 2 || byte(bytes, 2) == 10) &&
                   byte(bytes, 16) == 32;
        case ExportImageFormat::tiff:
            return bytes.size() > 8 && byte(bytes, 0) == 'I' && byte(bytes, 1) == 'I' &&
                   little_u16(bytes, 2) == 42;
        case ExportImageFormat::openexr:
            return bytes.size() > 8 && little_u32(bytes, 0) == 20000630U &&
                   little_u32(bytes, 4) == 2 &&
                   little_u32(bytes, 30) == (depth == ExportBitDepth::bits_16 ? 1U : 2U);
    }
    return false;
}

bool format_depth_matrix_is_enforced() {
    constexpr std::array formats{ExportImageFormat::png, ExportImageFormat::jpeg,
                                 ExportImageFormat::tga, ExportImageFormat::tiff,
                                 ExportImageFormat::openexr};
    constexpr std::array depths{ExportBitDepth::bits_8, ExportBitDepth::bits_16,
                                ExportBitDepth::bits_32};
    const image::TiledImage source = sample_image();
    bool result = true;
    for (const ExportImageFormat format : formats) {
        for (const ExportBitDepth depth : depths) {
            const TextureEncodeOptions options{.format = format,
                                               .bit_depth = depth,
                                               .color_space = image::ColorSpace::srgb_rec709};
            if (export_format_supports_bit_depth(format, depth)) {
                const std::vector<std::byte> encoded = encode_texture_memory(source, options);
                result = expect(valid_signature(format, depth, encoded),
                                "supported format produced an invalid header") &&
                         expect(encoded == encode_texture_memory(source, options),
                                "texture encoding was not byte-deterministic") &&
                         result;
                continue;
            }
            try {
                static_cast<void>(encode_texture_memory(source, options));
                result =
                    expect(false, "unsupported format/depth combination was encoded") && result;
            } catch (const TextureEncodeError& error) {
                const std::string_view diagnostic(error.what());
                result = expect(error.code() == TextureEncodeErrorCode::unsupported_combination,
                                "unsupported combination returned the wrong error code") &&
                         expect(diagnostic.find(export_image_format_name(format)) !=
                                    std::string_view::npos,
                                "unsupported-combination diagnostic omitted the format") &&
                         expect(diagnostic.find(std::to_string(static_cast<unsigned>(depth))) !=
                                    std::string_view::npos,
                                "unsupported-combination diagnostic omitted the depth") &&
                         result;
            }
        }
    }
    return result;
}

std::uint32_t tiff_entry_value(std::span<const std::byte> bytes, std::uint16_t wanted_tag) {
    const std::size_t directory = little_u32(bytes, 4);
    const std::uint16_t count = little_u16(bytes, directory);
    for (std::uint16_t index = 0; index < count; ++index) {
        const std::size_t entry = directory + 2 + index * 12;
        if (little_u16(bytes, entry) == wanted_tag) {
            return little_u32(bytes, entry + 8);
        }
    }
    return 0;
}

bool tiff_two_channel_metadata_is_complete() {
    const image::TiledImage source = sample_image(2);
    const std::vector<std::byte> encoded = encode_texture_memory(
        source, {.format = ExportImageFormat::tiff, .bit_depth = ExportBitDepth::bits_16});
    const std::uint32_t bits = tiff_entry_value(encoded, 258);
    const std::uint32_t sample_formats = tiff_entry_value(encoded, 339);
    return expect((bits & 0xffffU) == 16 && (bits >> 16U) == 16,
                  "TIFF did not declare the depth of both gray-alpha channels") &&
           expect((sample_formats & 0xffffU) == 1 && (sample_formats >> 16U) == 1,
                  "TIFF did not declare both gray-alpha sample formats");
}

bool invalid_options_and_presets_are_refused() {
    const image::TiledImage source = sample_image();
    bool invalid_quality = false;
    try {
        static_cast<void>(encode_texture_memory(source, {.format = ExportImageFormat::jpeg,
                                                         .bit_depth = ExportBitDepth::bits_8,
                                                         .jpeg_quality = 0}));
    } catch (const TextureEncodeError& error) {
        invalid_quality = error.code() == TextureEncodeErrorCode::invalid_option;
    }
    ExportPreset preset = default_export_preset();
    preset.textures.front().format = ExportImageFormat::png;
    preset.textures.front().bit_depth = ExportBitDepth::bits_32;
    bool invalid_preset = false;
    try {
        validate_export_preset(preset);
    } catch (const ExportPresetError& error) {
        const std::string_view diagnostic(error.what());
        invalid_preset = error.code() == ExportPresetErrorCode::invalid_preset &&
                         diagnostic.find("PNG") != std::string_view::npos &&
                         diagnostic.find("32") != std::string_view::npos;
    }
    return expect(invalid_quality, "invalid JPEG quality was accepted") &&
           expect(invalid_preset,
                  "preset format/depth refusal did not name both the format and depth");
}

std::string extension(ExportImageFormat format) {
    switch (format) {
        case ExportImageFormat::png:
            return "png";
        case ExportImageFormat::jpeg:
            return "jpg";
        case ExportImageFormat::tga:
            return "tga";
        case ExportImageFormat::tiff:
            return "tiff";
        case ExportImageFormat::openexr:
            return "exr";
    }
    return "bin";
}

bool write_fixtures(const std::filesystem::path& directory) {
    constexpr std::array options{
        TextureEncodeOptions{.format = ExportImageFormat::png, .bit_depth = ExportBitDepth::bits_8},
        TextureEncodeOptions{.format = ExportImageFormat::png,
                             .bit_depth = ExportBitDepth::bits_16},
        TextureEncodeOptions{.format = ExportImageFormat::jpeg,
                             .bit_depth = ExportBitDepth::bits_8},
        TextureEncodeOptions{.format = ExportImageFormat::tga, .bit_depth = ExportBitDepth::bits_8},
        TextureEncodeOptions{.format = ExportImageFormat::tiff,
                             .bit_depth = ExportBitDepth::bits_8},
        TextureEncodeOptions{.format = ExportImageFormat::tiff,
                             .bit_depth = ExportBitDepth::bits_16},
        TextureEncodeOptions{.format = ExportImageFormat::tiff,
                             .bit_depth = ExportBitDepth::bits_32},
        TextureEncodeOptions{.format = ExportImageFormat::openexr,
                             .bit_depth = ExportBitDepth::bits_16},
        TextureEncodeOptions{.format = ExportImageFormat::openexr,
                             .bit_depth = ExportBitDepth::bits_32},
    };
    std::filesystem::create_directories(directory);
    const image::TiledImage source = sample_image();
    for (const TextureEncodeOptions& option : options) {
        const std::string name = extension(option.format) +
                                 std::to_string(static_cast<unsigned>(option.bit_depth)) + "." +
                                 extension(option.format);
        const std::vector<std::byte> encoded = encode_texture_memory(source, option);
        std::ofstream output(directory / name, std::ios::binary);
        output.write(reinterpret_cast<const char*>(encoded.data()),
                     static_cast<std::streamsize>(encoded.size()));
        if (!output) {
            return expect(false, "failed to write an encoder validation fixture");
        }
    }
    return true;
}

}  // namespace

int main(int argc, char** argv) {
    if (argc == 3 && std::string_view(argv[1]) == "--write-fixtures") {
        return write_fixtures(argv[2]) ? 0 : 1;
    }
    return format_depth_matrix_is_enforced() && tiff_two_channel_metadata_is_complete() &&
                   invalid_options_and_presets_are_refused()
               ? 0
               : 1;
}
