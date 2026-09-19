#ifndef CTEX_MAPS_GENERATORS_HPP
#define CTEX_MAPS_GENERATORS_HPP

#include <array>
#include <cstdint>
#include <ctex/maps/mesh_maps.hpp>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

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

struct MeshMapGeneratorParameterDescriptor {
    std::string_view name;
    double default_value{};
    double minimum{};
    double maximum{};
    std::string_view meaning;
};

struct MeshMapGeneratorParameter {
    std::string_view name;
    double value{};
};

struct MeshMapGeneratorResolvedParameter {
    std::string name;
    double value{};
    friend bool operator==(const MeshMapGeneratorResolvedParameter&,
                           const MeshMapGeneratorResolvedParameter&) = default;
};

struct MeshMapGeneratorParameterClamp {
    std::string name;
    double supplied{};
    double resolved{};
    friend bool operator==(const MeshMapGeneratorParameterClamp&,
                           const MeshMapGeneratorParameterClamp&) = default;
};

struct MeshMapGeneratorParameterReport {
    std::vector<MeshMapGeneratorResolvedParameter> resolved;
    std::vector<MeshMapGeneratorParameterClamp> clamps;

    [[nodiscard]] std::optional<double> value_for(std::string_view name) const noexcept;
    [[nodiscard]] std::optional<MeshMapGeneratorParameterClamp> clamp_for(
        std::string_view name) const;
};

struct MeshMapGeneratorInfo {
    MeshMapGeneratorKind kind{};
    std::string_view name;
    std::span<const MeshMapKind> required_maps;
    std::span<const MeshMapGeneratorParameterDescriptor> parameters;
};

struct MeshMapGeneratorResult {
    std::shared_ptr<const image::TiledImage> mask;
    MeshMapRequirementReport map_report;
    MeshMapGeneratorParameterReport parameter_report;
};

[[nodiscard]] MeshMapGeneratorInfo mesh_map_generator_info(MeshMapGeneratorKind kind);

[[nodiscard]] MeshMapGeneratorResult generate_mesh_map_mask(
    MeshMapGeneratorKind kind, const MeshMapSet& maps, std::uint32_t width, std::uint32_t height,
    std::span<const MeshMapGeneratorParameter> parameters = {});

}  // namespace ctex::maps

#endif
