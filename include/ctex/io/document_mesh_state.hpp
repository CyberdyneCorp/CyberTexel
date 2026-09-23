#ifndef CTEX_IO_DOCUMENT_MESH_STATE_HPP
#define CTEX_IO_DOCUMENT_MESH_STATE_HPP

#include <cstddef>
#include <cstdint>
#include <ctex/image/tiled_image.hpp>
#include <ctex/io/project_container.hpp>
#include <ctex/mesh/mesh.hpp>
#include <optional>
#include <string>
#include <vector>

namespace ctex::io {

inline constexpr std::uint32_t current_document_mesh_state_schema = 1;
inline constexpr std::string_view document_mesh_state_asset_kind = "document-mesh-state";

struct StoredDocumentMeshMap {
    std::uint32_t kind{};
    std::string texture_set_id;
    std::string uv_set;
    mesh::MeshRevision produced_mesh_revision{};
    std::optional<std::uint32_t> normal_convention;
    std::optional<mesh::TangentFrameDescriptor> tangent_frame;
    image::TiledImage pixels;
};

struct DocumentMeshState {
    std::string document_asset_id;
    std::string mesh_resource_id;
    mesh::MeshRevision mesh_revision{};
    std::vector<StoredDocumentMeshMap> maps;
};

struct DocumentMeshStateReadLimits {
    std::size_t maximum_payload_bytes{256ULL << 20};
    std::size_t maximum_string_bytes{1ULL << 20};
    std::size_t maximum_maps{1'000'000};
    std::size_t maximum_total_map_pixel_bytes{1ULL << 30};
};

class DocumentMeshStateIoError final : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

[[nodiscard]] std::string document_mesh_state_asset_id(std::string_view document_asset_id);
void upsert_document_mesh_state(ProjectContainer& project, const DocumentMeshState& state);
[[nodiscard]] std::optional<DocumentMeshState> read_document_mesh_state(
    const ProjectContainer& project, std::string_view document_asset_id,
    DocumentMeshStateReadLimits limits = {});

}  // namespace ctex::io

#endif
