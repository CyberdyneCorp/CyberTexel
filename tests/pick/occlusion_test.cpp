#include <array>
#include <ctex/mesh/mesh.hpp>
#include <ctex/pick/hit.hpp>
#include <ctex/pick/spatial_index.hpp>
#include <iostream>
#include <limits>
#include <string_view>

namespace {

struct TwoWalls {
    std::array<ctex::mesh::Vec3f, 6> positions{
        ctex::mesh::Vec3f{0.0F, 0.0F, 1.0F}, ctex::mesh::Vec3f{1.0F, 0.0F, 1.0F},
        ctex::mesh::Vec3f{0.0F, 1.0F, 1.0F}, ctex::mesh::Vec3f{0.0F, 0.0F, 0.0F},
        ctex::mesh::Vec3f{0.0F, 1.0F, 0.0F}, ctex::mesh::Vec3f{1.0F, 0.0F, 0.0F},
    };
    std::array<ctex::mesh::Vec3f, 6> normals{
        ctex::mesh::Vec3f{0.0F, 0.0F, 1.0F},  ctex::mesh::Vec3f{0.0F, 0.0F, 1.0F},
        ctex::mesh::Vec3f{0.0F, 0.0F, 1.0F},  ctex::mesh::Vec3f{0.0F, 0.0F, -1.0F},
        ctex::mesh::Vec3f{0.0F, 0.0F, -1.0F}, ctex::mesh::Vec3f{0.0F, 0.0F, -1.0F},
    };
    std::array<ctex::mesh::Vec2f, 6> uv{
        ctex::mesh::Vec2f{0.0F, 0.0F}, ctex::mesh::Vec2f{1.0F, 0.0F}, ctex::mesh::Vec2f{0.0F, 1.0F},
        ctex::mesh::Vec2f{0.0F, 0.0F}, ctex::mesh::Vec2f{0.0F, 1.0F}, ctex::mesh::Vec2f{1.0F, 0.0F},
    };
    std::array<std::uint32_t, 6> indices{0, 1, 2, 3, 4, 5};
    std::array<ctex::mesh::UvSetView, 1> uv_sets{
        ctex::mesh::UvSetView{"paint", uv},
    };
    std::array<ctex::mesh::MeshPartition, 1> partitions{
        ctex::mesh::MeshPartition{ctex::mesh::PartitionKind::object, "shell", "Shell"},
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

bool nearest_is_the_default() {
    TwoWalls buffers;
    const ctex::mesh::MeshBinding mesh(buffers.descriptor());
    ctex::pick::SpatialIndex index(mesh);
    constexpr std::array bindings{ctex::pick::TextureSetBindingView{0, "paint"}};
    const auto hit =
        ctex::pick::pick_nearest(index, mesh, {{0.25F, 0.25F, 2.0F}, {0.0F, 0.0F, -1.0F}},
                                 std::numeric_limits<float>::infinity(), bindings);
    return expect(hit.has_value() && hit->triangle_index == 0 && hit->distance == 1.0F,
                  "default pick did not return the nearest wall");
}

bool all_hits_are_distance_ordered() {
    TwoWalls buffers;
    const ctex::mesh::MeshBinding mesh(buffers.descriptor());
    ctex::pick::SpatialIndex index(mesh);
    constexpr std::array bindings{ctex::pick::TextureSetBindingView{0, "paint"}};
    const auto hits =
        ctex::pick::pick_ray(index, mesh, {{0.25F, 0.25F, 2.0F}, {0.0F, 0.0F, -1.0F}},
                             {.maximum_distance = std::numeric_limits<float>::infinity(),
                              .occlusion = ctex::pick::OcclusionPolicy::all_hits,
                              .backfaces = ctex::pick::BackfacePolicy::accept},
                             bindings);
    return expect(hits.size() == 2, "all-hits pick omitted a wall") &&
           expect(hits[0].triangle_index == 0 && hits[0].distance == 1.0F,
                  "all-hits pick did not return the near wall first") &&
           expect(hits[1].triangle_index == 1 && hits[1].distance == 2.0F,
                  "all-hits pick did not return the far wall second");
}

bool backface_rejection_is_per_call() {
    TwoWalls buffers;
    const ctex::mesh::MeshBinding mesh(buffers.descriptor());
    ctex::pick::SpatialIndex index(mesh);
    constexpr std::array bindings{ctex::pick::TextureSetBindingView{0, "paint"}};
    const auto entering =
        ctex::pick::pick_ray(index, mesh, {{0.25F, 0.25F, 2.0F}, {0.0F, 0.0F, -1.0F}},
                             {.maximum_distance = 4.0F,
                              .occlusion = ctex::pick::OcclusionPolicy::all_hits,
                              .backfaces = ctex::pick::BackfacePolicy::reject},
                             bindings);
    const auto leaving =
        ctex::pick::pick_ray(index, mesh, {{0.25F, 0.25F, -1.0F}, {0.0F, 0.0F, 1.0F}},
                             {.maximum_distance = 4.0F,
                              .occlusion = ctex::pick::OcclusionPolicy::all_hits,
                              .backfaces = ctex::pick::BackfacePolicy::reject},
                             bindings);
    return expect(entering.size() == 1 && entering.front().triangle_index == 0,
                  "entering ray retained a back-facing wall") &&
           expect(leaving.size() == 1 && leaving.front().triangle_index == 1,
                  "reverse ray retained the opposite back-facing wall");
}

}  // namespace

int main() {
    return nearest_is_the_default() && all_hits_are_distance_ordered() &&
                   backface_rejection_is_per_call()
               ? 0
               : 1;
}
