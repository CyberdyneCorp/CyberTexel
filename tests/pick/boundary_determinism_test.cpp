#include <array>
#include <cstdint>
#include <ctex/mesh/mesh.hpp>
#include <ctex/pick/hit.hpp>
#include <ctex/pick/spatial_index.hpp>
#include <iostream>
#include <limits>
#include <string_view>

namespace {

struct SharedBoundaryMesh {
    std::array<ctex::mesh::Vec3f, 4> positions{
        ctex::mesh::Vec3f{0.0F, 0.0F, 0.0F},
        ctex::mesh::Vec3f{1.0F, 0.0F, 0.0F},
        ctex::mesh::Vec3f{0.0F, 1.0F, 0.0F},
        ctex::mesh::Vec3f{1.0F, 1.0F, 0.0F},
    };
    std::array<ctex::mesh::Vec3f, 4> normals{
        ctex::mesh::Vec3f{0.0F, 0.0F, 1.0F},
        ctex::mesh::Vec3f{0.0F, 0.0F, 1.0F},
        ctex::mesh::Vec3f{0.0F, 0.0F, 1.0F},
        ctex::mesh::Vec3f{0.0F, 0.0F, 1.0F},
    };
    std::array<ctex::mesh::Vec2f, 4> uv{
        ctex::mesh::Vec2f{0.0F, 0.0F},
        ctex::mesh::Vec2f{1.0F, 0.0F},
        ctex::mesh::Vec2f{0.0F, 1.0F},
        ctex::mesh::Vec2f{1.0F, 1.0F},
    };
    // Both triangles share edge 1-2 and vertices 1 and 2.
    std::array<std::uint32_t, 6> indices{0, 1, 2, 2, 1, 3};
    std::array<ctex::mesh::UvSetView, 1> uv_sets{ctex::mesh::UvSetView{"paint", uv}};
    std::array<ctex::mesh::MeshPartition, 1> partitions{
        ctex::mesh::MeshPartition{ctex::mesh::PartitionKind::object, "quad", "Quad"},
    };
    std::array<std::uint32_t, 2> face_partitions{0, 0};
    std::array<std::uint32_t, 2> face_materials{10, 20};

    [[nodiscard]] ctex::mesh::MeshDescriptor descriptor() const {
        return {
            .positions = positions,
            .normals = normals,
            .vertex_colors = {},
            .triangle_indices = indices,
            .uv_sets = uv_sets,
            .default_uv_set = "paint",
            .partitions = partitions,
            .face_partition_indices = face_partitions,
            .face_material_ids = face_materials,
        };
    }
};

bool expect(bool condition, std::string_view message) {
    if (!condition) {
        std::cerr << message << '\n';
    }
    return condition;
}

bool repeatedly_selects_lowest_triangle(ctex::pick::SpatialIndex& index,
                                        const ctex::mesh::MeshBinding& mesh,
                                        ctex::mesh::Vec3f origin) {
    constexpr std::array bindings{ctex::pick::TextureSetBindingView{0, "paint"}};
    for (int repetition = 0; repetition < 32; ++repetition) {
        const auto hit = ctex::pick::pick_nearest(index, mesh, {origin, {0.0F, 0.0F, -1.0F}},
                                                  std::numeric_limits<float>::infinity(), bindings);
        if (!hit || hit->triangle_index != 0) {
            return false;
        }
    }
    return true;
}

bool shared_edges_and_vertices_have_one_owner() {
    SharedBoundaryMesh buffers;
    const ctex::mesh::MeshBinding mesh(buffers.descriptor());
    ctex::pick::SpatialIndex index(mesh);
    return expect(repeatedly_selects_lowest_triangle(index, mesh, {0.5F, 0.5F, 1.0F}),
                  "shared edge did not resolve to the lowest triangle index") &&
           expect(repeatedly_selects_lowest_triangle(index, mesh, {1.0F, 0.0F, 1.0F}),
                  "shared vertex did not resolve to the lowest triangle index");
}

bool all_hits_remain_explicit_and_ordered() {
    SharedBoundaryMesh buffers;
    const ctex::mesh::MeshBinding mesh(buffers.descriptor());
    ctex::pick::SpatialIndex index(mesh);
    constexpr std::array bindings{ctex::pick::TextureSetBindingView{0, "paint"}};
    const auto hits = ctex::pick::pick_ray(index, mesh, {{0.5F, 0.5F, 1.0F}, {0.0F, 0.0F, -1.0F}},
                                           {.maximum_distance = 2.0F,
                                            .occlusion = ctex::pick::OcclusionPolicy::all_hits,
                                            .backfaces = ctex::pick::BackfacePolicy::accept},
                                           bindings);
    return expect(hits.size() == 2 && hits[0].triangle_index == 0 && hits[1].triangle_index == 1,
                  "all-hits boundary intersections were not explicitly index ordered");
}

}  // namespace

int main() {
    return shared_edges_and_vertices_have_one_owner() && all_hits_remain_explicit_and_ordered() ? 0
                                                                                                : 1;
}
