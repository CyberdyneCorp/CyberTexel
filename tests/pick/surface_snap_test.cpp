#include <array>
#include <cmath>
#include <ctex/mesh/mesh.hpp>
#include <ctex/pick/hit.hpp>
#include <ctex/pick/spatial_index.hpp>
#include <iostream>
#include <string_view>

namespace {

struct SnapMesh {
    std::array<ctex::mesh::Vec3f, 6> positions{
        ctex::mesh::Vec3f{0.0F, 0.0F, 0.0F}, ctex::mesh::Vec3f{1.0F, 0.0F, 0.0F},
        ctex::mesh::Vec3f{0.0F, 1.0F, 0.0F}, ctex::mesh::Vec3f{0.0F, 0.0F, 2.0F},
        ctex::mesh::Vec3f{1.0F, 0.0F, 2.0F}, ctex::mesh::Vec3f{0.0F, 1.0F, 2.0F},
    };
    std::array<ctex::mesh::Vec3f, 6> normals{
        ctex::mesh::Vec3f{0.0F, 0.0F, 1.0F}, ctex::mesh::Vec3f{0.0F, 0.0F, 1.0F},
        ctex::mesh::Vec3f{0.0F, 0.0F, 1.0F}, ctex::mesh::Vec3f{0.0F, 0.0F, 1.0F},
        ctex::mesh::Vec3f{0.0F, 0.0F, 1.0F}, ctex::mesh::Vec3f{0.0F, 0.0F, 1.0F},
    };
    std::array<ctex::mesh::Vec2f, 6> uv{
        ctex::mesh::Vec2f{0.0F, 0.0F}, ctex::mesh::Vec2f{1.0F, 0.0F}, ctex::mesh::Vec2f{0.0F, 1.0F},
        ctex::mesh::Vec2f{0.0F, 0.0F}, ctex::mesh::Vec2f{1.0F, 0.0F}, ctex::mesh::Vec2f{0.0F, 1.0F},
    };
    std::array<std::uint32_t, 6> indices{0, 1, 2, 3, 4, 5};
    std::array<ctex::mesh::UvSetView, 1> uv_sets{ctex::mesh::UvSetView{"paint", uv}};
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

bool close(float left, float right, float tolerance = 1.0e-5F) {
    return std::abs(left - right) <= tolerance;
}

bool snaps_to_the_nearest_surface() {
    SnapMesh buffers;
    const ctex::mesh::MeshBinding mesh(buffers.descriptor());
    ctex::pick::SpatialIndex index(mesh);
    constexpr std::array bindings{ctex::pick::TextureSetBindingView{0, "paint"}};
    const auto hit = ctex::pick::snap_to_surface(index, mesh, {0.25F, 0.25F, 1.8F}, 1.0F, bindings);
    if (!expect(hit.has_value(), "nearby surface point reported a miss")) {
        return false;
    }
    return expect(hit->triangle_index == 1, "snap selected the farther triangle") &&
           expect(close(hit->position.x, 0.25F) && close(hit->position.y, 0.25F) &&
                      close(hit->position.z, 2.0F),
                  "snap returned the wrong surface position") &&
           expect(close(hit->distance, 0.2F), "snap returned the wrong world-space distance") &&
           expect(close(hit->uv.x, 0.25F) && close(hit->uv.y, 0.25F),
                  "snap returned the wrong UV") &&
           expect(close(hit->interpolated_normal.z, 1.0F), "snap returned the wrong normal") &&
           expect(hit->material_id == 20, "snap returned the wrong material identifier");
}

bool snaps_to_edges_and_respects_the_limit() {
    SnapMesh buffers;
    const ctex::mesh::MeshBinding mesh(buffers.descriptor());
    ctex::pick::SpatialIndex index(mesh);
    constexpr std::array bindings{ctex::pick::TextureSetBindingView{0, "paint"}};
    const auto edge =
        ctex::pick::snap_to_surface(index, mesh, {0.75F, 0.75F, 2.0F}, 0.4F, bindings);
    const auto too_far =
        ctex::pick::snap_to_surface(index, mesh, {0.75F, 0.75F, 2.0F}, 0.3F, bindings);
    return expect(edge.has_value() && close(edge->position.x, 0.5F) &&
                      close(edge->position.y, 0.5F) && close(edge->position.z, 2.0F),
                  "snap did not project onto the nearest triangle edge") &&
           expect(!too_far.has_value(), "snap ignored the caller-supplied maximum distance");
}

bool degenerate_triangle_snaps_to_its_segments() {
    SnapMesh buffers;
    buffers.positions[3] = {0.0F, 0.0F, 2.0F};
    buffers.positions[4] = {1.0F, 0.0F, 2.0F};
    buffers.positions[5] = {2.0F, 0.0F, 2.0F};
    const ctex::mesh::MeshBinding mesh(buffers.descriptor());
    ctex::pick::SpatialIndex index(mesh);
    constexpr std::array bindings{ctex::pick::TextureSetBindingView{0, "paint"}};
    const auto hit = ctex::pick::snap_to_surface(index, mesh, {1.5F, 0.5F, 2.0F}, 0.6F, bindings);
    return expect(hit.has_value() && close(hit->position.x, 1.5F) && close(hit->position.y, 0.0F) &&
                      close(hit->position.z, 2.0F) && close(hit->distance, 0.5F),
                  "degenerate triangle did not fall back to segment snapping");
}

}  // namespace

int main() {
    return snaps_to_the_nearest_surface() && snaps_to_edges_and_respects_the_limit() &&
                   degenerate_triangle_snaps_to_its_segments()
               ? 0
               : 1;
}
