#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <ctex/maps/generators.hpp>
#include <stdexcept>

namespace ctex::maps {
namespace {

constexpr std::array ao_maps{MeshMapKind::ambient_occlusion};
constexpr std::array curvature_maps{MeshMapKind::curvature};
constexpr std::array thickness_maps{MeshMapKind::thickness};
constexpr std::array position_maps{MeshMapKind::position};
constexpr std::array direction_maps{MeshMapKind::world_space_direction};
constexpr std::array dirt_maps{MeshMapKind::ambient_occlusion, MeshMapKind::curvature};
constexpr std::array scratches_maps{MeshMapKind::position, MeshMapKind::world_space_direction};

double component(const MeshMapSet& maps, MeshMapKind kind, double u, double v,
                 std::size_t index = 0) {
    return maps.sample(kind, u, v).sample.values[index];
}

double saturate(double value) { return std::clamp(value, 0.0, 1.0); }

double scratches_value(const MeshMapSet& maps, double u, double v) {
    const MeshMapSample position = maps.sample(MeshMapKind::position, u, v).sample;
    const MeshMapSample direction = maps.sample(MeshMapKind::world_space_direction, u, v).sample;
    const double phase =
        position.values[0] * 31.0 + position.values[1] * 7.0 + position.values[2] * 13.0;
    const double distance_to_line = std::abs((phase - std::floor(phase)) - 0.5);
    const double line = saturate(1.0 - distance_to_line * 24.0);
    const double grazing = 1.0 - std::abs(direction.values[2] * 2.0 - 1.0);
    return line * saturate(grazing);
}

double generator_value(MeshMapGeneratorKind kind, const MeshMapSet& maps, double u, double v) {
    switch (kind) {
        case MeshMapGeneratorKind::ambient_occlusion:
            return saturate(1.0 - component(maps, MeshMapKind::ambient_occlusion, u, v));
        case MeshMapGeneratorKind::curvature:
            return saturate(component(maps, MeshMapKind::curvature, u, v));
        case MeshMapGeneratorKind::thickness:
            return saturate(1.0 - component(maps, MeshMapKind::thickness, u, v));
        case MeshMapGeneratorKind::position_gradient:
            return saturate(component(maps, MeshMapKind::position, u, v, 1));
        case MeshMapGeneratorKind::world_space_direction:
            return saturate(component(maps, MeshMapKind::world_space_direction, u, v, 1));
        case MeshMapGeneratorKind::dirt: {
            const double occlusion = 1.0 - component(maps, MeshMapKind::ambient_occlusion, u, v);
            const double concavity = (0.5 - component(maps, MeshMapKind::curvature, u, v)) * 2.0;
            return saturate(std::max(occlusion, concavity));
        }
        case MeshMapGeneratorKind::edge_wear:
            return saturate((component(maps, MeshMapKind::curvature, u, v) - 0.5) * 2.0);
        case MeshMapGeneratorKind::scratches:
            return scratches_value(maps, u, v);
    }
    throw std::invalid_argument("mesh-map generator kind is invalid");
}

void write_float(image::TiledImage& image, std::uint32_t x, std::uint32_t y, double value) {
    const float encoded = static_cast<float>(value);
    std::array<std::byte, sizeof(encoded)> pixel{};
    std::memcpy(pixel.data(), &encoded, sizeof(encoded));
    image.write_pixel(x, y, pixel);
}

}  // namespace

MeshMapGeneratorInfo mesh_map_generator_info(MeshMapGeneratorKind kind) {
    switch (kind) {
        case MeshMapGeneratorKind::ambient_occlusion:
            return {.kind = kind, .name = "ambient-occlusion", .required_maps = ao_maps};
        case MeshMapGeneratorKind::curvature:
            return {.kind = kind, .name = "curvature", .required_maps = curvature_maps};
        case MeshMapGeneratorKind::thickness:
            return {.kind = kind, .name = "thickness", .required_maps = thickness_maps};
        case MeshMapGeneratorKind::position_gradient:
            return {.kind = kind, .name = "position-gradient", .required_maps = position_maps};
        case MeshMapGeneratorKind::world_space_direction:
            return {.kind = kind, .name = "world-space-direction", .required_maps = direction_maps};
        case MeshMapGeneratorKind::dirt:
            return {.kind = kind, .name = "dirt", .required_maps = dirt_maps};
        case MeshMapGeneratorKind::edge_wear:
            return {.kind = kind, .name = "edge-wear", .required_maps = curvature_maps};
        case MeshMapGeneratorKind::scratches:
            return {.kind = kind, .name = "scratches", .required_maps = scratches_maps};
    }
    throw std::invalid_argument("mesh-map generator kind is invalid");
}

MeshMapGeneratorResult generate_mesh_map_mask(MeshMapGeneratorKind kind, const MeshMapSet& maps,
                                              std::uint32_t width, std::uint32_t height) {
    if (width == 0 || height == 0) {
        throw std::invalid_argument("mesh-map generator output dimensions must be non-zero");
    }
    const MeshMapGeneratorInfo info = mesh_map_generator_info(kind);
    MeshMapRequirementReport report = maps.require_maps(info.name, info.required_maps);
    auto mask = std::make_shared<image::TiledImage>(
        width, height,
        image::PixelFormat{.channel_type = image::ChannelType::float32, .channel_count = 1});
    for (std::uint32_t y = 0; y < height; ++y) {
        const double v = 1.0 - (static_cast<double>(y) + 0.5) / static_cast<double>(height);
        for (std::uint32_t x = 0; x < width; ++x) {
            const double u = (static_cast<double>(x) + 0.5) / static_cast<double>(width);
            write_float(*mask, x, y, generator_value(kind, maps, u, v));
        }
    }
    mask->clear_dirty();
    return {.mask = std::move(mask), .map_report = std::move(report)};
}

}  // namespace ctex::maps
