#include <tinyexr.h>

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <ctex/io/image_io.hpp>
#include <iostream>
#include <span>
#include <string_view>
#include <vector>

#include "icc_test_profile.hpp"

namespace {

bool expect(bool condition, std::string_view message) {
    if (!condition) std::cerr << message << '\n';
    return condition;
}

void append_ascii(std::vector<std::byte>& output, std::string_view value) {
    for (char character : value) output.push_back(static_cast<std::byte>(character));
}

void append_u16(std::vector<std::byte>& output, std::uint16_t value) {
    output.push_back(static_cast<std::byte>(value >> 8U));
    output.push_back(static_cast<std::byte>(value));
}

void append_i16(std::vector<std::byte>& output, std::int16_t value) {
    append_u16(output, static_cast<std::uint16_t>(value));
}

void append_u32(std::vector<std::byte>& output, std::uint32_t value) {
    output.push_back(static_cast<std::byte>(value >> 24U));
    output.push_back(static_cast<std::byte>(value >> 16U));
    output.push_back(static_cast<std::byte>(value >> 8U));
    output.push_back(static_cast<std::byte>(value));
}

void append_i32(std::vector<std::byte>& output, std::int32_t value) {
    append_u32(output, static_cast<std::uint32_t>(value));
}

void append_psd_layer_record(std::vector<std::byte>& output, std::string_view name) {
    append_i32(output, 0);
    append_i32(output, 0);
    append_i32(output, 1);
    append_i32(output, 1);
    append_u16(output, 4);
    for (const std::int16_t channel : std::array<std::int16_t, 4>{0, 1, 2, -1}) {
        append_i16(output, channel);
        append_u32(output, 3);  // compression word plus one sample
    }
    append_ascii(output, "8BIMnorm");
    output.insert(output.end(), {std::byte{255}, std::byte{0}, std::byte{0}, std::byte{0}});
    const std::size_t padded_name_size = ((name.size() + 1 + 3) / 4) * 4;
    append_u32(output, static_cast<std::uint32_t>(8 + padded_name_size));
    append_u32(output, 0);  // layer mask
    append_u32(output, 0);  // blending ranges
    output.push_back(static_cast<std::byte>(name.size()));
    append_ascii(output, name);
    output.insert(output.end(), padded_name_size - name.size() - 1, std::byte{0});
}

void append_psd_layer_pixels(std::vector<std::byte>& output, std::array<std::uint8_t, 4> rgba) {
    for (std::uint8_t sample : rgba) {
        append_u16(output, 0);  // raw compression
        output.push_back(static_cast<std::byte>(sample));
    }
}

std::vector<std::byte> make_layered_psd() {
    std::vector<std::byte> layer_info;
    append_i16(layer_info, 2);
    append_psd_layer_record(layer_info, "Top");
    append_psd_layer_record(layer_info, "Bottom");
    append_psd_layer_pixels(layer_info, {255, 0, 0, 128});
    append_psd_layer_pixels(layer_info, {0, 0, 255, 255});

    std::vector<std::byte> layer_mask;
    append_u32(layer_mask, static_cast<std::uint32_t>(layer_info.size()));
    layer_mask.insert(layer_mask.end(), layer_info.begin(), layer_info.end());

    std::vector<std::byte> result;
    append_ascii(result, "8BPS");
    append_u16(result, 1);
    result.insert(result.end(), 6, std::byte{0});
    append_u16(result, 4);
    append_u32(result, 1);
    append_u32(result, 1);
    append_u16(result, 8);
    append_u16(result, 3);
    append_u32(result, 0);  // color-mode data
    append_u32(result, 0);  // image resources
    append_u32(result, static_cast<std::uint32_t>(layer_mask.size()));
    result.insert(result.end(), layer_mask.begin(), layer_mask.end());
    append_u16(result, 0);  // raw flattened composite
    for (std::uint8_t sample : std::array<std::uint8_t, 4>{128, 0, 127, 255}) {
        result.push_back(static_cast<std::byte>(sample));
    }
    return result;
}

std::vector<std::byte> add_psd_icc_profile(std::span<const std::byte> psd,
                                           std::span<const unsigned char> profile) {
    std::vector<std::byte> resource;
    append_ascii(resource, "8BIM");
    append_u16(resource, 0x040f);
    resource.insert(resource.end(), {std::byte{0}, std::byte{0}});
    append_u32(resource, static_cast<std::uint32_t>(profile.size()));
    for (const unsigned char value : profile) resource.push_back(static_cast<std::byte>(value));
    if ((resource.size() & 1U) != 0) resource.push_back(std::byte{0});

    std::vector<std::byte> result(psd.begin(), psd.begin() + 30);
    append_u32(result, static_cast<std::uint32_t>(resource.size()));
    result.insert(result.end(), resource.begin(), resource.end());
    result.insert(result.end(), psd.begin() + 34, psd.end());
    return result;
}

std::vector<std::byte> make_multipart_exr() {
    std::array<EXRHeader, 2> headers{};
    std::array<EXRImage, 2> images{};
    std::array<std::array<EXRChannelInfo, 4>, 2> channels{};
    std::array<std::array<int, 4>, 2> pixel_types{};
    std::array<std::array<int, 4>, 2> requested_types{};
    std::array<std::array<std::array<float, 1>, 4>, 2> samples{};
    std::array<std::array<unsigned char*, 4>, 2> sample_pointers{};
    std::array<const EXRHeader*, 2> header_pointers{};
    constexpr std::array<std::string_view, 4> channel_names{"A", "B", "G", "R"};
    constexpr std::array<std::string_view, 2> part_names{"Ground", "Glow"};
    constexpr std::array<std::array<float, 4>, 2> part_values{
        std::array{1.0F, 0.0F, 0.0F, 1.0F},
        std::array{0.5F, 1.0F, 0.0F, 0.0F},
    };
    for (std::size_t part = 0; part < headers.size(); ++part) {
        InitEXRHeader(&headers[part]);
        InitEXRImage(&images[part]);
        headers[part].num_channels = 4;
        headers[part].channels = channels[part].data();
        headers[part].pixel_types = pixel_types[part].data();
        headers[part].requested_pixel_types = requested_types[part].data();
        headers[part].compression_type = TINYEXR_COMPRESSIONTYPE_ZIP;
        headers[part].data_window = {0, 0, 0, 0};
        headers[part].display_window = {0, 0, 0, 0};
        EXRSetNameAttr(&headers[part], part_names[part].data());
        for (std::size_t channel = 0; channel < channel_names.size(); ++channel) {
            std::strncpy(channels[part][channel].name, channel_names[channel].data(), 255);
            channels[part][channel].pixel_type = TINYEXR_PIXELTYPE_FLOAT;
            channels[part][channel].x_sampling = 1;
            channels[part][channel].y_sampling = 1;
            pixel_types[part][channel] = TINYEXR_PIXELTYPE_FLOAT;
            requested_types[part][channel] = TINYEXR_PIXELTYPE_FLOAT;
            samples[part][channel][0] = part_values[part][channel];
            sample_pointers[part][channel] =
                reinterpret_cast<unsigned char*>(samples[part][channel].data());
        }
        images[part].num_channels = 4;
        images[part].width = 1;
        images[part].height = 1;
        images[part].images = sample_pointers[part].data();
        header_pointers[part] = &headers[part];
    }
    unsigned char* encoded = nullptr;
    const char* error = nullptr;
    const std::size_t size =
        SaveEXRMultipartImageToMemory(images.data(), header_pointers.data(),
                                      static_cast<unsigned>(headers.size()), &encoded, &error);
    if (size == 0) {
        if (error != nullptr) {
            std::cerr << error << '\n';
            FreeEXRErrorMessage(error);
        }
        return {};
    }
    std::vector<std::byte> result(size);
    std::memcpy(result.data(), encoded, size);
    std::free(encoded);
    return result;
}

float component(std::span<const std::byte> pixel, std::size_t index) {
    float result = 0.0F;
    std::memcpy(&result, pixel.data() + index * sizeof(float), sizeof(result));
    return result;
}

bool layered_psd_supports_both_modes() {
    const auto profile = ctex::io::test::make_rec709_icc_profile(false);
    const auto encoded = add_psd_icc_profile(make_layered_psd(), profile);
    const auto individual = ctex::io::decode_layered_image_memory(
        {.image = {.bytes = encoded, .source_name = "layers.psd"},
         .mode = ctex::io::LayeredDecodeMode::individual});
    const auto overridden = ctex::io::decode_layered_image_memory(
        {.image = {.bytes = encoded,
                   .source_name = "layers.psd",
                   .color_space = ctex::image::InputColorSpace::linear_rec709},
         .mode = ctex::io::LayeredDecodeMode::individual});
    const auto composite = ctex::io::decode_layered_image_memory(
        {.image = {.bytes = encoded, .source_name = "layers.psd"},
         .mode = ctex::io::LayeredDecodeMode::composite});
    return expect(individual.source_was_layered && individual.images.size() == 2,
                  "PSD layers were not separately addressable") &&
           expect(individual.images[0].name == "Top" && individual.images[1].name == "Bottom",
                  "PSD source layer names were not preserved") &&
           expect(individual.images[0].image.pixels.read_pixel(0, 0)[0] == std::byte{255} &&
                      individual.images[0].image.pixels.read_pixel(0, 0)[3] == std::byte{128} &&
                      individual.images[1].image.pixels.read_pixel(0, 0)[2] == std::byte{255},
                  "PSD layer pixels changed") &&
           expect(individual.images[0].image.source_color_space ==
                          ctex::image::ColorSpace::srgb_rec709 &&
                      individual.images[0].image.report.color_space_source ==
                          ctex::io::ColorSpaceSource::embedded_profile &&
                      individual.images[1].image.report.color_space_source ==
                          ctex::io::ColorSpaceSource::embedded_profile,
                  "PSD ICC profile was not applied to every individual layer") &&
           expect(overridden.images[0].image.source_color_space ==
                          ctex::image::ColorSpace::linear_rec709 &&
                      overridden.images[0].image.report.color_space_source ==
                          ctex::io::ColorSpaceSource::caller,
                  "caller declaration did not override the layered PSD ICC profile") &&
           expect(composite.images.size() == 1 && composite.images[0].name == "Composite",
                  "PSD composited mode did not return one image") &&
           expect(composite.images[0].image.pixels.read_pixel(0, 0)[0] == std::byte{128} &&
                      composite.images[0].image.pixels.read_pixel(0, 0)[2] == std::byte{127},
                  "PSD composited mode did not use the flattened appearance");
}

bool multipart_exr_supports_both_modes() {
    const auto encoded = make_multipart_exr();
    if (!expect(!encoded.empty(), "multipart EXR fixture could not be encoded")) return false;
    const auto individual = ctex::io::decode_layered_image_memory(
        {.image = {.bytes = encoded, .source_name = "parts.exr"},
         .mode = ctex::io::LayeredDecodeMode::individual});
    const auto composite = ctex::io::decode_layered_image_memory(
        {.image = {.bytes = encoded, .source_name = "parts.exr"},
         .mode = ctex::io::LayeredDecodeMode::composite});
    const auto glow = individual.images[1].image.pixels.read_pixel(0, 0);
    const auto combined = composite.images[0].image.pixels.read_pixel(0, 0);
    return expect(individual.source_was_layered && individual.images.size() == 2,
                  "OpenEXR parts were not separately addressable") &&
           expect(individual.images[0].name == "Ground" && individual.images[1].name == "Glow",
                  "OpenEXR source part names were not preserved") &&
           expect(component(glow, 0) == 0.0F && component(glow, 2) == 1.0F &&
                      component(glow, 3) == 0.5F,
                  "OpenEXR part channels changed") &&
           expect(composite.images.size() == 1 &&
                      std::abs(component(combined, 0) - 0.5F) < 0.0001F &&
                      std::abs(component(combined, 2) - 0.5F) < 0.0001F &&
                      component(combined, 3) == 1.0F,
                  "OpenEXR composited mode changed alpha-over appearance") &&
           expect(composite.images[0].image.report.estimated_peak_working_bytes >
                      individual.images[0].image.report.estimated_peak_working_bytes,
                  "OpenEXR composite output was omitted from its working-memory bound");
}

bool layered_limits_are_enforced_before_publication() {
    const auto encoded = make_layered_psd();
    try {
        static_cast<void>(ctex::io::decode_layered_image_memory(
            {.image = {.bytes = encoded, .source_name = "layers.psd"},
             .mode = ctex::io::LayeredDecodeMode::individual,
             .maximum_image_count = 1}));
    } catch (const ctex::io::ImageIoError& error) {
        if (!expect(error.code() == ctex::io::ImageIoErrorCode::over_limit,
                    "layer-count refusal used the wrong error code")) {
            return false;
        }
        try {
            static_cast<void>(ctex::io::decode_layered_image_memory(
                {.image = {.bytes = encoded,
                           .source_name = "layers.psd",
                           .control = {.maximum_working_bytes = 1}},
                 .mode = ctex::io::LayeredDecodeMode::individual}));
        } catch (const ctex::io::ImageIoError& budget_error) {
            return expect(budget_error.code() == ctex::io::ImageIoErrorCode::over_limit,
                          "layered working-memory refusal used the wrong error code");
        }
    }
    return expect(false, "layered decode did not enforce its count or memory limit");
}

}  // namespace

int main() {
    return layered_psd_supports_both_modes() && multipart_exr_supports_both_modes() &&
                   layered_limits_are_enforced_before_publication()
               ? 0
               : 1;
}
