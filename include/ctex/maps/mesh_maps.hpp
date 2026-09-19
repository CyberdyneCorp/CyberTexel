#ifndef CTEX_MAPS_MESH_MAPS_HPP
#define CTEX_MAPS_MESH_MAPS_HPP

#include <array>
#include <cstddef>
#include <cstdint>
#include <ctex/doc/document.hpp>
#include <ctex/image/tiled_image.hpp>
#include <map>
#include <memory>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace ctex::maps {

enum class MeshMapKind : std::uint8_t {
    tangent_space_normal,
    object_space_normal,
    world_space_direction,
    ambient_occlusion,
    curvature,
    thickness,
    position,
    height,
    bent_normal,
    material_id,
    object_id,
    uv_density,
    vertex_colour,
};

inline constexpr std::array all_mesh_map_kinds{
    MeshMapKind::tangent_space_normal,
    MeshMapKind::object_space_normal,
    MeshMapKind::world_space_direction,
    MeshMapKind::ambient_occlusion,
    MeshMapKind::curvature,
    MeshMapKind::thickness,
    MeshMapKind::position,
    MeshMapKind::height,
    MeshMapKind::bent_normal,
    MeshMapKind::material_id,
    MeshMapKind::object_id,
    MeshMapKind::uv_density,
    MeshMapKind::vertex_colour,
};

[[nodiscard]] std::string_view mesh_map_name(MeshMapKind kind);

struct MeshMapDescriptor {
    MeshMapKind kind{};
    std::string texture_set_id;
    std::string uv_set;
    std::shared_ptr<const image::TiledImage> pixels;
};

struct MapResolutionMismatch {
    MeshMapKind kind{};
    std::string texture_set_id;
    std::uint32_t map_width{};
    std::uint32_t map_height{};
    std::uint32_t texture_set_width{};
    std::uint32_t texture_set_height{};

    friend bool operator==(const MapResolutionMismatch&, const MapResolutionMismatch&) = default;
};

struct MeshMapBindResult {
    bool replaced_existing{};
    std::optional<MapResolutionMismatch> resolution_mismatch;
};

struct MeshMapSample {
    std::uint8_t component_count{};
    std::array<double, 4> values{};

    friend bool operator==(const MeshMapSample&, const MeshMapSample&) = default;
};

struct MeshMapRequirementReport {
    std::string consumer;
    std::string texture_set_id;
    std::vector<MeshMapKind> missing_maps;
    std::string message;

    [[nodiscard]] bool satisfied() const noexcept { return missing_maps.empty(); }
    friend bool operator==(const MeshMapRequirementReport&,
                           const MeshMapRequirementReport&) = default;
};

class MissingMeshMapsError : public std::out_of_range {
public:
    explicit MissingMeshMapsError(MeshMapRequirementReport report);

    [[nodiscard]] const MeshMapRequirementReport& report() const noexcept { return report_; }

private:
    MeshMapRequirementReport report_;
};

class MeshMapSet {
public:
    explicit MeshMapSet(const doc::TextureSet& texture_set);

    [[nodiscard]] const std::string& texture_set_id() const noexcept { return texture_set_id_; }
    [[nodiscard]] const std::string& uv_set() const noexcept { return uv_set_; }
    [[nodiscard]] std::uint32_t texture_set_width() const noexcept { return texture_set_width_; }
    [[nodiscard]] std::uint32_t texture_set_height() const noexcept { return texture_set_height_; }

    [[nodiscard]] MeshMapBindResult bind(MeshMapDescriptor descriptor);
    [[nodiscard]] bool contains(MeshMapKind kind) const noexcept;
    [[nodiscard]] const MeshMapDescriptor& map(MeshMapKind kind) const;
    [[nodiscard]] std::vector<MeshMapKind> bound_maps() const;
    [[nodiscard]] std::size_t size() const noexcept { return maps_.size(); }
    [[nodiscard]] MeshMapRequirementReport check_required_maps(
        std::string_view consumer, std::span<const MeshMapKind> required) const;
    void require_maps(std::string_view consumer, std::span<const MeshMapKind> required) const;
    [[nodiscard]] MeshMapSample sample(MeshMapKind kind, double u, double v) const;

private:
    std::string texture_set_id_;
    std::string uv_set_;
    std::uint32_t texture_set_width_{};
    std::uint32_t texture_set_height_{};
    std::map<MeshMapKind, MeshMapDescriptor> maps_;
};

}  // namespace ctex::maps

#endif
