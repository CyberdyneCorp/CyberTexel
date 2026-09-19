#include <array>
#include <cmath>
#include <ctex/mesh/mesh.hpp>
#include <ctex/pick/hit.hpp>
#include <ctex/pick/spatial_index.hpp>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string_view>

namespace {

struct HitMesh {
    std::array<ctex::mesh::Vec3f, 3> positions{
        ctex::mesh::Vec3f{0.0F, 0.0F, 0.0F},
        ctex::mesh::Vec3f{2.0F, 0.0F, 0.0F},
        ctex::mesh::Vec3f{0.0F, 2.0F, 0.0F},
    };
    std::array<ctex::mesh::Vec3f, 3> normals{
        ctex::mesh::Vec3f{0.0F, 0.0F, 1.0F},
        ctex::mesh::Vec3f{0.0F, 0.70710677F, 0.70710677F},
        ctex::mesh::Vec3f{0.0F, 0.0F, 1.0F},
    };
    std::array<ctex::mesh::Vec2f, 3> uv{
        ctex::mesh::Vec2f{0.0F, 0.0F},
        ctex::mesh::Vec2f{2.0F, 0.0F},
        ctex::mesh::Vec2f{0.0F, 2.0F},
    };
    std::array<std::uint32_t, 3> indices{0, 1, 2};
    std::array<ctex::mesh::UvSetView, 1> uv_sets{
        ctex::mesh::UvSetView{"paint", uv},
    };
    std::array<ctex::mesh::MeshPartition, 1> partitions{
        ctex::mesh::MeshPartition{ctex::mesh::PartitionKind::material, "body", "Body"},
    };
    std::array<std::uint32_t, 1> face_partitions{0};
    std::array<std::uint32_t, 1> face_materials{77};

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

bool close(float left, float right, float tolerance = 1.0e-5F) {
    return std::abs(left - right) <= tolerance;
}

bool reports_every_hit_field() {
    HitMesh buffers;
    const ctex::mesh::MeshBinding mesh(buffers.descriptor());
    ctex::pick::SpatialIndex index(mesh);
    constexpr std::array bindings{ctex::pick::TextureSetBindingView{0, "paint"}};
    const auto hit =
        ctex::pick::pick_nearest(index, mesh, {{1.5F, 0.25F, 1.0F}, {0.0F, 0.0F, -2.0F}},
                                 std::numeric_limits<float>::infinity(), bindings);

    if (!expect(hit.has_value(), "exact ray/triangle intersection reported a miss")) {
        return false;
    }
    const float barycentric_sum = hit->barycentric.x + hit->barycentric.y + hit->barycentric.z;
    return expect(close(hit->position.x, 1.5F) && close(hit->position.y, 0.25F) &&
                      close(hit->position.z, 0.0F),
                  "hit position is incorrect") &&
           expect(close(hit->geometric_normal.x, 0.0F) && close(hit->geometric_normal.y, 0.0F) &&
                      close(hit->geometric_normal.z, 1.0F),
                  "geometric normal is incorrect") &&
           expect(hit->interpolated_normal.y > 0.0F &&
                      !close(hit->interpolated_normal.y, hit->geometric_normal.y),
                  "smooth interpolated normal was not distinct") &&
           expect(close(hit->uv.x, 1.5F) && close(hit->uv.y, 0.25F),
                  "named texture-set UV is incorrect") &&
           expect(hit->texture_set_id == "material/4:body/uv/5:paint",
                  "stable texture-set identifier is incorrect") &&
           expect(hit->udim_tile.u == 1 && hit->udim_tile.v == 0 && hit->udim_tile.number == 1002,
                  "UDIM tile is incorrect") &&
           expect(hit->triangle_index == 0, "triangle index is incorrect") &&
           expect(close(hit->barycentric.x, 0.125F) && close(hit->barycentric.y, 0.75F) &&
                      close(hit->barycentric.z, 0.125F) && close(barycentric_sum, 1.0F),
                  "barycentric coordinates are incorrect") &&
           expect(hit->material_id == 77, "material identifier is incorrect") &&
           expect(close(hit->distance, 1.0F), "distance was not measured in world-space ray units");
}

bool miss_is_a_distinct_non_error_result() {
    HitMesh buffers;
    const ctex::mesh::MeshBinding mesh(buffers.descriptor());
    ctex::pick::SpatialIndex index(mesh);
    constexpr std::array bindings{ctex::pick::TextureSetBindingView{0, "paint"}};
    const auto miss =
        ctex::pick::pick_nearest(index, mesh, {{4.0F, 4.0F, 1.0F}, {0.0F, 0.0F, -1.0F}},
                                 std::numeric_limits<float>::infinity(), bindings);
    const auto beyond_limit = ctex::pick::pick_nearest(
        index, mesh, {{0.25F, 0.25F, 1.0F}, {0.0F, 0.0F, -1.0F}}, 0.5F, bindings);
    return expect(!miss.has_value(), "background ray did not return a distinct miss") &&
           expect(!beyond_limit.has_value(), "maximum pick distance was ignored");
}

bool hit_requires_a_texture_set_binding() {
    HitMesh buffers;
    const ctex::mesh::MeshBinding mesh(buffers.descriptor());
    ctex::pick::SpatialIndex index(mesh);
    try {
        static_cast<void>(ctex::pick::pick_nearest(
            index, mesh, {{0.25F, 0.25F, 1.0F}, {0.0F, 0.0F, -1.0F}}, 2.0F, {}));
    } catch (const std::invalid_argument&) {
        return true;
    }
    return expect(false, "hit without a texture-set binding was accepted");
}

}  // namespace

int main() {
    return reports_every_hit_field() && miss_is_a_distinct_non_error_result() &&
                   hit_requires_a_texture_set_binding()
               ? 0
               : 1;
}
