#ifndef CTEX_CLI_OBJ_MESH_HPP
#define CTEX_CLI_OBJ_MESH_HPP

#include <cstddef>
#include <ctex/mesh/mesh.hpp>
#include <span>
#include <string>
#include <vector>

namespace ctex::cli {

struct ObjMesh {
    std::vector<mesh::Vec3f> positions;
    std::vector<mesh::Vec3f> normals;
    std::vector<mesh::Vec2f> uv;
    std::vector<std::uint32_t> triangle_indices;
    std::vector<std::string> material_names;
    std::vector<std::uint32_t> face_partitions;
    std::vector<std::uint32_t> face_materials;
    mutable std::vector<mesh::UvSetView> uv_sets;
    mutable std::vector<mesh::MeshPartition> partitions;

    [[nodiscard]] mesh::MeshDescriptor descriptor() const;
};

[[nodiscard]] ObjMesh parse_obj_mesh(std::span<const std::byte> bytes, std::size_t maximum_vertices,
                                     std::size_t maximum_triangles);

}  // namespace ctex::cli

#endif
