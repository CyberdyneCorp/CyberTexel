#ifndef CTEX_MAPS_GENERATORS_HPP
#define CTEX_MAPS_GENERATORS_HPP

#include <array>
#include <cstdint>
#include <ctex/maps/mesh_maps.hpp>
#include <memory>
#include <span>
#include <string_view>

namespace ctex::maps {

enum class MeshMapGeneratorKind : std::uint8_t {
    ambient_occlusion,
    curvature,
    thickness,
    position_gradient,
    world_space_direction,
    dirt,
    edge_wear,
    scratches,
};

inline constexpr std::array all_mesh_map_generator_kinds{
    MeshMapGeneratorKind::ambient_occlusion,
    MeshMapGeneratorKind::curvature,
    MeshMapGeneratorKind::thickness,
    MeshMapGeneratorKind::position_gradient,
    MeshMapGeneratorKind::world_space_direction,
    MeshMapGeneratorKind::dirt,
    MeshMapGeneratorKind::edge_wear,
    MeshMapGeneratorKind::scratches,
};

struct MeshMapGeneratorInfo {
    MeshMapGeneratorKind kind{};
    std::string_view name;
    std::span<const MeshMapKind> required_maps;
};

struct MeshMapGeneratorResult {
    std::shared_ptr<const image::TiledImage> mask;
    MeshMapRequirementReport map_report;
};

[[nodiscard]] MeshMapGeneratorInfo mesh_map_generator_info(MeshMapGeneratorKind kind);

[[nodiscard]] MeshMapGeneratorResult generate_mesh_map_mask(MeshMapGeneratorKind kind,
                                                            const MeshMapSet& maps,
                                                            std::uint32_t width,
                                                            std::uint32_t height);

}  // namespace ctex::maps

#endif
