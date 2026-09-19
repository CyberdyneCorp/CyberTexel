#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <ctex/mesh/mesh.hpp>
#include <ctex/pick/spatial_index.hpp>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string_view>
#include <vector>

namespace {

class TriangleStrip {
public:
    TriangleStrip(std::size_t triangle_count, float x_offset) {
        positions_.reserve(triangle_count * 3);
        normals_.reserve(triangle_count * 3);
        uv_.reserve(triangle_count * 3);
        indices_.reserve(triangle_count * 3);
        face_partitions_.resize(triangle_count, 0);
        face_materials_.resize(triangle_count, 7);
        for (std::size_t triangle = 0; triangle < triangle_count; ++triangle) {
            const float x = x_offset + static_cast<float>(triangle) * 2.0F;
            positions_.insert(positions_.end(),
                              {{x, 0.0F, 0.0F}, {x + 0.5F, 0.0F, 0.0F}, {x, 0.5F, 0.0F}});
            normals_.insert(normals_.end(), 3, {0.0F, 0.0F, 1.0F});
            uv_.insert(uv_.end(), {{0.0F, 0.0F}, {1.0F, 0.0F}, {0.0F, 1.0F}});
            const auto first_vertex = static_cast<std::uint32_t>(triangle * 3);
            indices_.insert(indices_.end(), {first_vertex, first_vertex + 1, first_vertex + 2});
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
        ctex::mesh::MeshPartition{ctex::mesh::PartitionKind::material, "surface", "Surface"},
    };
};

class MillionTriangleGrid {
public:
    explicit MillionTriangleGrid(std::uint32_t side_length) : side_length_(side_length) {
        const std::size_t vertices_per_side = static_cast<std::size_t>(side_length_) + 1;
        const std::size_t vertex_count = vertices_per_side * vertices_per_side;
        const std::size_t triangle_count =
            static_cast<std::size_t>(side_length_) * side_length_ * 2;
        positions_.reserve(vertex_count);
        normals_.reserve(vertex_count);
        uv_.reserve(vertex_count);
        indices_.reserve(triangle_count * 3);
        face_partitions_.resize(triangle_count, 0);
        face_materials_.resize(triangle_count, 7);

        for (std::uint32_t y = 0; y <= side_length_; ++y) {
            for (std::uint32_t x = 0; x <= side_length_; ++x) {
                positions_.push_back({static_cast<float>(x), static_cast<float>(y), 0.0F});
                normals_.push_back({0.0F, 0.0F, 1.0F});
                uv_.push_back(
                    {static_cast<float>(x) / side_length_, static_cast<float>(y) / side_length_});
            }
        }
        for (std::uint32_t y = 0; y < side_length_; ++y) {
            for (std::uint32_t x = 0; x < side_length_; ++x) {
                const std::uint32_t lower_left = y * (side_length_ + 1) + x;
                const std::uint32_t lower_right = lower_left + 1;
                const std::uint32_t upper_left = lower_left + side_length_ + 1;
                const std::uint32_t upper_right = upper_left + 1;
                indices_.insert(indices_.end(), {lower_left, lower_right, upper_left, lower_right,
                                                 upper_right, upper_left});
            }
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
    std::uint32_t side_length_;
    std::vector<ctex::mesh::Vec3f> positions_;
    std::vector<ctex::mesh::Vec3f> normals_;
    std::vector<ctex::mesh::Vec2f> uv_;
    std::vector<std::uint32_t> indices_;
    std::vector<std::uint32_t> face_partitions_;
    std::vector<std::uint32_t> face_materials_;
    std::array<ctex::mesh::UvSetView, 1> uv_sets_{};
    std::array<ctex::mesh::MeshPartition, 1> partitions_{
        ctex::mesh::MeshPartition{ctex::mesh::PartitionKind::material, "grid", "Grid"},
    };
};

bool expect(bool condition, std::string_view message) {
    if (!condition) {
        std::cerr << message << '\n';
    }
    return condition;
}

bool contains(const std::vector<std::uint32_t>& values, std::uint32_t value) {
    return std::find(values.begin(), values.end(), value) != values.end();
}

ctex::pick::Ray ray_for_triangle(std::size_t triangle, float x_offset) {
    return {
        .origin = {x_offset + static_cast<float>(triangle) * 2.0F + 0.1F, 0.1F, 1.0F},
        .direction = {0.0F, 0.0F, -1.0F},
    };
}

bool queries_do_not_scan_the_triangle_array() {
    constexpr std::size_t triangle_count = 4096;
    constexpr std::uint32_t target = 3072;
    TriangleStrip buffers(triangle_count, 0.0F);
    const ctex::mesh::MeshBinding mesh(buffers.descriptor());
    ctex::pick::SpatialIndex index(mesh);

    const auto result = index.query_ray_candidates(mesh, ray_for_triangle(target, 0.0F),
                                                   std::numeric_limits<float>::infinity());
    const auto point_result = index.query_point_candidates(
        mesh, {static_cast<float>(target) * 2.0F + 0.1F, 0.1F, 0.25F}, 0.5F);
    return expect(index.triangle_count() == triangle_count,
                  "spatial index omitted source triangles") &&
           expect(index.node_count() > 1, "spatial index did not build a hierarchy") &&
           expect(contains(result.triangle_indices, target),
                  "ray traversal omitted its target triangle") &&
           expect(result.tested_leaf_triangles <= 4,
                  "ray traversal scanned more than one bounded leaf") &&
           expect(result.tested_leaf_triangles < triangle_count,
                  "ray traversal degraded to a linear triangle scan") &&
           expect(point_result.tested_leaf_triangles <= 4,
                  "point traversal scanned more than one bounded leaf") &&
           expect(contains(point_result.triangle_indices, target),
                  "point traversal omitted its nearest triangle");
}

bool multi_million_triangle_mesh_uses_bounded_leaves() {
    constexpr std::uint32_t side_length = 1024;
    constexpr std::size_t triangle_count = static_cast<std::size_t>(side_length) * side_length * 2;
    constexpr std::uint32_t target_x = 731;
    constexpr std::uint32_t target_y = 619;
    const std::uint32_t target_triangle = (target_y * side_length + target_x) * 2;
    MillionTriangleGrid buffers(side_length);
    const ctex::mesh::MeshBinding mesh(buffers.descriptor());
    ctex::pick::SpatialIndex index(mesh);
    const ctex::pick::Ray ray{
        {static_cast<float>(target_x) + 0.2F, static_cast<float>(target_y) + 0.2F, 1.0F},
        {0.0F, 0.0F, -1.0F},
    };
    const auto result =
        index.query_ray_candidates(mesh, ray, std::numeric_limits<float>::infinity());
    return expect(index.triangle_count() == triangle_count,
                  "multi-million-triangle BVH omitted source geometry") &&
           expect(contains(result.triangle_indices, target_triangle),
                  "multi-million-triangle traversal omitted its target") &&
           expect(result.tested_leaf_triangles <= 4,
                  "multi-million-triangle query scanned beyond one bounded leaf") &&
           expect(result.visited_nodes < triangle_count,
                  "multi-million-triangle query degraded to a linear node scan");
}

bool reuses_and_invalidates_by_mesh_revision() {
    constexpr std::size_t triangle_count = 128;
    constexpr std::uint32_t target = 42;
    TriangleStrip original(triangle_count, 0.0F);
    TriangleStrip replacement(triangle_count, 1000.0F);
    ctex::mesh::MeshBinding mesh(original.descriptor());
    ctex::pick::SpatialIndex index(mesh);
    const auto original_revision = mesh.revision();

    const auto first = index.query_ray_candidates(mesh, ray_for_triangle(target, 0.0F),
                                                  std::numeric_limits<float>::infinity());
    const auto second = index.query_ray_candidates(mesh, ray_for_triangle(target, 0.0F),
                                                   std::numeric_limits<float>::infinity());
    const bool rebuilt_without_change = index.synchronize(mesh);
    const auto build_count_after_reuse = index.build_count();

    mesh.replace(replacement.descriptor());
    const auto stale_location = index.query_ray_candidates(mesh, ray_for_triangle(target, 0.0F),
                                                           std::numeric_limits<float>::infinity());
    const auto build_count_after_change = index.build_count();
    const auto replacement_location = index.query_ray_candidates(
        mesh, ray_for_triangle(target, 1000.0F), std::numeric_limits<float>::infinity());

    return expect(contains(first.triangle_indices, target) &&
                      contains(second.triangle_indices, target),
                  "reused index changed identical query results") &&
           expect(!rebuilt_without_change && build_count_after_reuse == 1,
                  "unchanged revision rebuilt the spatial index") &&
           expect(mesh.revision() != original_revision && build_count_after_change == 2,
                  "first query after mesh replacement did not rebuild the index") &&
           expect(index.build_count() == 2, "replacement index was not reused") &&
           expect(index.mesh_revision() == mesh.revision(),
                  "rebuilt index retained a stale mesh revision") &&
           expect(stale_location.triangle_indices.empty(),
                  "rebuilt index returned geometry from the old mesh") &&
           expect(contains(replacement_location.triangle_indices, target),
                  "rebuilt index omitted replacement geometry");
}

bool rejected_replacement_keeps_the_published_revision() {
    TriangleStrip buffers(8, 0.0F);
    ctex::mesh::MeshBinding mesh(buffers.descriptor());
    const auto revision = mesh.revision();
    auto invalid = buffers.descriptor();
    invalid.face_partition_indices = {};

    bool refused = false;
    try {
        mesh.replace(invalid);
    } catch (const std::invalid_argument&) {
        refused = true;
    }
    return expect(refused, "mesh binding accepted an invalid replacement") &&
           expect(mesh.revision() == revision,
                  "failed mesh replacement advanced the published revision") &&
           expect(mesh.view().triangle_count() == 8,
                  "failed mesh replacement discarded the preceding view");
}

}  // namespace

int main() {
    return queries_do_not_scan_the_triangle_array() &&
                   multi_million_triangle_mesh_uses_bounded_leaves() &&
                   reuses_and_invalidates_by_mesh_revision() &&
                   rejected_replacement_keeps_the_published_revision()
               ? 0
               : 1;
}
