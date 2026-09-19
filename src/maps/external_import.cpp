#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <ctex/maps/external_import.hpp>
#include <span>
#include <stdexcept>

#include "pixel_buffer_copy.hpp"

namespace ctex::maps {
namespace {

void require_channel_count(MeshMapKind kind, std::uint8_t actual, std::uint8_t minimum,
                           std::uint8_t maximum) {
    if (actual < minimum || actual > maximum) {
        throw std::invalid_argument("external mesh map '" + std::string(mesh_map_name(kind)) +
                                    "' has an incompatible channel count");
    }
}

MeshMapChannelMeaning expected_meaning(MeshMapKind kind, std::uint8_t channel_count) {
    switch (kind) {
        case MeshMapKind::tangent_space_normal:
        case MeshMapKind::object_space_normal:
        case MeshMapKind::bent_normal:
            require_channel_count(kind, channel_count, 3, 3);
            return MeshMapChannelMeaning::normal_xyz;
        case MeshMapKind::world_space_direction:
            require_channel_count(kind, channel_count, 3, 3);
            return MeshMapChannelMeaning::direction_xyz;
        case MeshMapKind::position:
            require_channel_count(kind, channel_count, 3, 3);
            return MeshMapChannelMeaning::position_xyz;
        case MeshMapKind::material_id:
        case MeshMapKind::object_id:
            require_channel_count(kind, channel_count, 1, 1);
            return MeshMapChannelMeaning::identifier;
        case MeshMapKind::vertex_colour:
            require_channel_count(kind, channel_count, 3, 4);
            return channel_count == 4 ? MeshMapChannelMeaning::colour_rgba
                                      : MeshMapChannelMeaning::colour_rgb;
        case MeshMapKind::ambient_occlusion:
        case MeshMapKind::curvature:
        case MeshMapKind::thickness:
        case MeshMapKind::height:
        case MeshMapKind::uv_density:
            require_channel_count(kind, channel_count, 1, 1);
            return MeshMapChannelMeaning::scalar_data;
    }
    throw std::invalid_argument("mesh map kind is invalid");
}

bool is_colour(MeshMapChannelMeaning meaning) {
    return meaning == MeshMapChannelMeaning::colour_rgb ||
           meaning == MeshMapChannelMeaning::colour_rgba;
}

double decode_component(std::span<const std::byte> pixel, image::PixelFormat format,
                        std::size_t component) {
    const std::size_t offset = component * format.bytes_per_channel();
    switch (format.channel_type) {
        case image::ChannelType::uint8_unorm:
            return std::to_integer<std::uint8_t>(pixel[offset]) / 255.0;
        case image::ChannelType::uint16_unorm: {
            std::uint16_t value{};
            std::memcpy(&value, pixel.data() + offset, sizeof(value));
            return value / 65535.0;
        }
        case image::ChannelType::float32: {
            float value{};
            std::memcpy(&value, pixel.data() + offset, sizeof(value));
            if (!std::isfinite(value)) {
                throw std::invalid_argument("colour mesh-map buffer contains a non-finite value");
            }
            return value;
        }
    }
    throw std::invalid_argument("mesh-map pixel buffer has an invalid format");
}

void encode_component(std::span<std::byte> pixel, image::PixelFormat format, std::size_t component,
                      double value) {
    const std::size_t offset = component * format.bytes_per_channel();
    switch (format.channel_type) {
        case image::ChannelType::uint8_unorm:
            pixel[offset] =
                static_cast<std::byte>(std::lround(std::clamp(value, 0.0, 1.0) * 255.0));
            return;
        case image::ChannelType::uint16_unorm: {
            const auto encoded =
                static_cast<std::uint16_t>(std::lround(std::clamp(value, 0.0, 1.0) * 65535.0));
            std::memcpy(pixel.data() + offset, &encoded, sizeof(encoded));
            return;
        }
        case image::ChannelType::float32: {
            const float encoded = static_cast<float>(value);
            std::memcpy(pixel.data() + offset, &encoded, sizeof(encoded));
            return;
        }
    }
    throw std::invalid_argument("mesh-map pixel buffer has an invalid format");
}

void convert_colour_to_working_space(image::TiledImage& pixels, image::ColorSpace source) {
    if (source == image::working_color_space()) {
        return;
    }
    const image::PixelFormat format = pixels.format();
    std::array<std::byte, 16> converted{};
    for (std::uint32_t y = 0; y < pixels.height(); ++y) {
        for (std::uint32_t x = 0; x < pixels.width(); ++x) {
            const std::span<const std::byte> input = pixels.read_pixel(x, y);
            std::copy(input.begin(), input.end(), converted.begin());
            const image::RgbColor colour = image::convert_color(
                {decode_component(input, format, 0), decode_component(input, format, 1),
                 decode_component(input, format, 2)},
                source, image::working_color_space());
            const std::span output(converted.data(), input.size());
            encode_component(output, format, 0, colour.red);
            encode_component(output, format, 1, colour.green);
            encode_component(output, format, 2, colour.blue);
            pixels.write_pixel(x, y, output);
        }
    }
}

void validate_declarations(const ExternalMeshMapImport& request) {
    static_cast<void>(mesh_map_name(request.kind));
    if (!request.channel_meaning) {
        throw std::invalid_argument("external mesh map requires a channel-meaning declaration");
    }
    if (!request.color_space) {
        throw std::invalid_argument("external mesh map requires a colour-space declaration");
    }
    static_cast<void>(mesh_map_channel_meaning_name(*request.channel_meaning));
    static_cast<void>(image::color_space_name(*request.color_space));
    if (mesh_map_uses_normal_convention(request.kind) && !request.normal_convention) {
        throw std::invalid_argument("external normal map requires an OpenGL or DirectX convention");
    }
    if (!mesh_map_uses_normal_convention(request.kind) && request.normal_convention) {
        throw std::invalid_argument("external non-normal map declares a normal-map convention");
    }
    if (request.normal_convention) {
        static_cast<void>(normal_map_convention_name(*request.normal_convention));
    }
    if (!request.buffer.format.is_valid()) {
        throw std::invalid_argument("external mesh map has an invalid pixel format");
    }
    const MeshMapChannelMeaning expected =
        expected_meaning(request.kind, request.buffer.format.channel_count);
    if (*request.channel_meaning != expected) {
        throw std::invalid_argument(
            "external mesh map '" + std::string(mesh_map_name(request.kind)) +
            "' declares channel meaning '" +
            std::string(mesh_map_channel_meaning_name(*request.channel_meaning)) + "', expected '" +
            std::string(mesh_map_channel_meaning_name(expected)) + "'");
    }
}

}  // namespace

std::string_view mesh_map_channel_meaning_name(MeshMapChannelMeaning meaning) {
    switch (meaning) {
        case MeshMapChannelMeaning::scalar_data:
            return "scalar-data";
        case MeshMapChannelMeaning::normal_xyz:
            return "normal-xyz";
        case MeshMapChannelMeaning::direction_xyz:
            return "direction-xyz";
        case MeshMapChannelMeaning::position_xyz:
            return "position-xyz";
        case MeshMapChannelMeaning::identifier:
            return "identifier";
        case MeshMapChannelMeaning::colour_rgb:
            return "colour-rgb";
        case MeshMapChannelMeaning::colour_rgba:
            return "colour-rgba";
    }
    throw std::invalid_argument("mesh-map channel meaning is invalid");
}

ExternalMeshMapImportResult import_external_mesh_map(MeshMapSet& target,
                                                     const ExternalMeshMapImport& request) {
    validate_declarations(request);
    auto pixels = detail::copy_mesh_map_pixel_buffer(request.buffer);
    const bool converted =
        is_colour(*request.channel_meaning) && *request.color_space != image::working_color_space();
    if (converted) {
        convert_colour_to_working_space(*pixels, *request.color_space);
    }
    MeshMapBindResult binding = target.bind({.kind = request.kind,
                                             .texture_set_id = request.texture_set_id,
                                             .uv_set = request.uv_set,
                                             .mesh_revision = request.mesh_revision,
                                             .normal_convention = request.normal_convention,
                                             .tangent_frame = request.tangent_frame,
                                             .pixels = std::move(pixels)});
    return {.binding = std::move(binding),
            .channel_meaning = *request.channel_meaning,
            .declared_color_space = *request.color_space,
            .storage_color_space = image::working_color_space(),
            .normal_convention = request.normal_convention,
            .tangent_frame = request.tangent_frame,
            .converted_to_working_space = converted};
}

}  // namespace ctex::maps
