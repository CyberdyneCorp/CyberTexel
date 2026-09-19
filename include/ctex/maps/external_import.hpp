#ifndef CTEX_MAPS_EXTERNAL_IMPORT_HPP
#define CTEX_MAPS_EXTERNAL_IMPORT_HPP

#include <cstdint>
#include <ctex/image/color.hpp>
#include <ctex/maps/mesh_maps.hpp>
#include <ctex/maps/pixel_buffer.hpp>
#include <optional>
#include <string>
#include <string_view>

namespace ctex::maps {

enum class MeshMapChannelMeaning : std::uint8_t {
    scalar_data,
    normal_xyz,
    direction_xyz,
    position_xyz,
    identifier,
    colour_rgb,
    colour_rgba,
};

[[nodiscard]] std::string_view mesh_map_channel_meaning_name(MeshMapChannelMeaning meaning);

struct ExternalMeshMapImport {
    MeshMapKind kind{};
    std::optional<MeshMapChannelMeaning> channel_meaning;
    std::optional<image::ColorSpace> color_space;
    std::string texture_set_id;
    std::string uv_set;
    mesh::MeshRevision mesh_revision{};
    MeshMapPixelBufferView buffer;
};

struct ExternalMeshMapImportResult {
    MeshMapBindResult binding;
    MeshMapChannelMeaning channel_meaning{};
    image::ColorSpace declared_color_space{};
    image::ColorSpace storage_color_space{};
    bool converted_to_working_space{};
};

[[nodiscard]] ExternalMeshMapImportResult import_external_mesh_map(
    MeshMapSet& target, const ExternalMeshMapImport& request);

}  // namespace ctex::maps

#endif
