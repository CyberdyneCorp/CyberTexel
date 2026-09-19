#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <ctex/mesh/mesh.hpp>
#include <ctex/pick/hit.hpp>
#include <ctex/pick/uv_index.hpp>
#include <iostream>
#include <stdexcept>
#include <string_view>
#include <vector>

namespace {

struct LayeredUvMesh {
    explicit LayeredUvMesh(float offset) {
        for (std::size_t triangle = 0; triangle < 2; ++triangle) {
            const std::size_t first = triangle * 3;
            lightmap[first] = {offset, offset};
            lightmap[first + 1] = {offset + 1.0F, offset};
            lightmap[first + 2] = {offset, offset + 1.0F};
        }
        uv_sets[1] = {"lightmap", lightmap};
    }

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
    std::array<ctex::mesh::Vec2f, 6> paint{
        ctex::mesh::Vec2f{0.0F, 0.0F}, ctex::mesh::Vec2f{1.0F, 0.0F}, ctex::mesh::Vec2f{0.0F, 1.0F},
        ctex::mesh::Vec2f{0.0F, 0.0F}, ctex::mesh::Vec2f{1.0F, 0.0F}, ctex::mesh::Vec2f{0.0F, 1.0F},
    };
    std::array<ctex::mesh::Vec2f, 6> lightmap{};
    std::array<std::uint32_t, 6> indices{0, 1, 2, 3, 4, 5};
    std::array<ctex::mesh::UvSetView, 2> uv_sets{
        ctex::mesh::UvSetView{"paint", paint},
        ctex::mesh::UvSetView{},
    };
    std::array<ctex::mesh::MeshPartition, 2> partitions{
        ctex::mesh::MeshPartition{ctex::mesh::PartitionKind::material, "lower", "Lower"},
        ctex::mesh::MeshPartition{ctex::mesh::PartitionKind::material, "upper", "Upper"},
    };
    std::array<std::uint32_t, 2> face_partitions{0, 1};
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

class UvStrip {
public:
    explicit UvStrip(std::size_t triangle_count) {
        positions_.reserve(triangle_count * 3);
        normals_.reserve(triangle_count * 3);
        uv_.reserve(triangle_count * 3);
        indices_.reserve(triangle_count * 3);
        face_partitions_.resize(triangle_count, 0);
        face_materials_.resize(triangle_count, 1);
        for (std::size_t triangle = 0; triangle < triangle_count; ++triangle) {
            const float x = static_cast<float>(triangle) * 2.0F;
            positions_.insert(positions_.end(),
                              {{x, 0.0F, 0.0F}, {x + 0.5F, 0.0F, 0.0F}, {x, 0.5F, 0.0F}});
            normals_.insert(normals_.end(), 3, {0.0F, 0.0F, 1.0F});
            uv_.insert(uv_.end(), {{x, 0.0F}, {x + 0.5F, 0.0F}, {x, 0.5F}});
            const auto first = static_cast<std::uint32_t>(triangle * 3);
            indices_.insert(indices_.end(), {first, first + 1, first + 2});
        }
        uv_sets_[0] = {"paint", uv_};
    }

    [[nodiscard]] ctex::mesh::MeshDescriptor descriptor() const {
        return {
            .positions = positions_,
            .normals = normals_,
            .vertex_colors = {},
            .triangle_indices = indices_,
            .uv_sets = uv_sets_,
            .default_uv_set = "paint",
            .partitions = partitions_,
            .face_partition_indices = face_partitions_,
            .face_material_ids = face_materials_,
        };
    }

private:
    std::vector<ctex::mesh::Vec3f> positions_;
    std::vector<ctex::mesh::Vec3f> normals_;
    std::vector<ctex::mesh::Vec2f> uv_;
    std::vector<std::uint32_t> indices_;
    std::vector<std::uint32_t> face_partitions_;
    std::vector<std::uint32_t> face_materials_;
    std::array<ctex::mesh::UvSetView, 1> uv_sets_{};
    std::array<ctex::mesh::MeshPartition, 1> partitions_{
        ctex::mesh::MeshPartition{ctex::mesh::PartitionKind::material, "strip", "Strip"},
    };
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

bool resolves_named_uv_within_the_texture_set() {
    LayeredUvMesh buffers(10.0F);
    const ctex::mesh::MeshBinding mesh(buffers.descriptor());
    ctex::pick::UvSpatialIndex index(mesh, "lightmap");
    const auto hit = ctex::pick::pick_uv(index, mesh, {10.25F, 10.25F}, {1, "lightmap"});
    const auto miss = ctex::pick::pick_uv(index, mesh, {12.0F, 12.0F}, {1, "lightmap"});

    if (!expect(hit.has_value(), "owned UV coordinate reported a miss")) {
        return false;
    }
    return expect(hit->triangle_index == 1, "UV pick ignored the texture-set partition") &&
           expect(close(hit->position.x, 0.25F) && close(hit->position.y, 0.25F) &&
                      close(hit->position.z, 2.0F),
                  "UV pick returned the wrong surface position") &&
           expect(close(hit->interpolated_normal.z, 1.0F),
                  "UV pick returned the wrong surface normal") &&
           expect(hit->texture_set_id == "material/5:upper/uv/8:lightmap",
                  "UV pick returned the wrong texture-set identity") &&
           expect(hit->material_id == 20, "UV pick returned the wrong material") &&
           expect(!miss.has_value(), "unowned UV coordinate did not return a miss");
}

bool rebuilds_before_querying_replacement_uvs() {
    LayeredUvMesh original(10.0F);
    LayeredUvMesh replacement(30.0F);
    ctex::mesh::MeshBinding mesh(original.descriptor());
    ctex::pick::UvSpatialIndex index(mesh, "lightmap");
    mesh.replace(replacement.descriptor());

    const auto stale = ctex::pick::pick_uv(index, mesh, {10.25F, 10.25F}, {1, "lightmap"});
    const auto current = ctex::pick::pick_uv(index, mesh, {30.25F, 30.25F}, {1, "lightmap"});
    return expect(!stale.has_value(), "UV index returned stale replacement coordinates") &&
           expect(current.has_value() && current->triangle_index == 1,
                  "UV index omitted replacement coordinates") &&
           expect(index.build_count() == 2, "UV index rebuilt an unexpected number of times");
}

bool uv_query_does_not_scan_every_triangle() {
    constexpr std::size_t triangle_count = 1024;
    constexpr std::size_t target = 700;
    UvStrip buffers(triangle_count);
    const ctex::mesh::MeshBinding mesh(buffers.descriptor());
    ctex::pick::UvSpatialIndex index(mesh, "paint");
    const auto candidates =
        index.query_candidates(mesh, {static_cast<float>(target) * 2.0F + 0.1F, 0.1F});
    return expect(index.node_count() > 1, "UV index did not build a hierarchy") &&
           expect(candidates.tested_leaf_triangles <= 4,
                  "UV query scanned more than one bounded leaf") &&
           expect(std::find(candidates.triangle_indices.begin(), candidates.triangle_indices.end(),
                            target) != candidates.triangle_indices.end(),
                  "UV query omitted its owning triangle");
}

}  // namespace

int main() {
    return resolves_named_uv_within_the_texture_set() &&
                   rebuilds_before_querying_replacement_uvs() &&
                   uv_query_does_not_scan_every_triangle()
               ? 0
               : 1;
}
