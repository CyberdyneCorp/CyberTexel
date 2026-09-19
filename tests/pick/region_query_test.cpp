#include <array>
#include <cstddef>
#include <cstdint>
#include <ctex/mesh/mesh.hpp>
#include <ctex/pick/region.hpp>
#include <ctex/pick/spatial_index.hpp>
#include <iostream>
#include <stdexcept>
#include <string_view>
#include <vector>

namespace {

constexpr ctex::pick::Mat4f identity{
    {1.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1.0F, 0.0F, 0.0F, 0.0F, 0.0F,
     1.0F},
};

class MeshBuffers {
public:
    explicit MeshBuffers(std::vector<ctex::mesh::Vec3f> positions)
        : positions_(std::move(positions)),
          normals_(positions_.size(), {0.0F, 0.0F, 1.0F}),
          uv_(positions_.size(), {0.0F, 0.0F}),
          indices_(positions_.size()),
          face_partitions_(positions_.size() / 3, 0),
          face_materials_(positions_.size() / 3, 1) {
        for (std::size_t index = 0; index < indices_.size(); ++index) {
            indices_[index] = static_cast<std::uint32_t>(index);
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
        ctex::mesh::MeshPartition{ctex::mesh::PartitionKind::object, "region", "Region"},
    };
};

MeshBuffers screen_mesh() {
    return MeshBuffers({
        {-0.8F, -0.2F, 0.0F},
        {-0.4F, -0.2F, 0.0F},
        {-0.6F, 0.2F, 0.0F},
        {-0.2F, 0.0F, 0.0F},
        {0.2F, 0.0F, 0.0F},
        {0.0F, 0.4F, 0.0F},
        {0.4F, -0.2F, 0.0F},
        {0.8F, -0.2F, 0.0F},
        {0.6F, 0.2F, 0.0F},
        {-0.1F, -0.1F, 2.0F},
        {0.1F, -0.1F, 2.0F},
        {0.0F, 0.1F, 2.0F},
    });
}

MeshBuffers world_mesh() {
    return MeshBuffers({
        {0.0F, 0.0F, 0.0F},
        {2.0F, 0.0F, 0.0F},
        {0.0F, 2.0F, 0.0F},
        {1.4F, 1.5F, 0.0F},
        {1.6F, 1.5F, 0.0F},
        {1.5F, 1.7F, 0.0F},
        {20.0F, 0.0F, 0.0F},
        {21.0F, 0.0F, 0.0F},
        {20.0F, 1.0F, 0.0F},
    });
}

class TriangleStrip {
public:
    explicit TriangleStrip(std::size_t triangle_count) {
        positions_.reserve(triangle_count * 3);
        for (std::size_t triangle = 0; triangle < triangle_count; ++triangle) {
            const float x = static_cast<float>(triangle) * 2.0F;
            positions_.insert(positions_.end(),
                              {{x, 0.0F, 0.0F}, {x + 0.5F, 0.0F, 0.0F}, {x, 0.5F, 0.0F}});
        }
        buffers_ = MeshBuffers(std::move(positions_));
    }

    [[nodiscard]] ctex::mesh::MeshDescriptor descriptor() const { return buffers_.descriptor(); }

private:
    std::vector<ctex::mesh::Vec3f> positions_;
    MeshBuffers buffers_{{{0.0F, 0.0F, 0.0F}, {1.0F, 0.0F, 0.0F}, {0.0F, 1.0F, 0.0F}}};
};

bool expect(bool condition, std::string_view message) {
    if (!condition) {
        std::cerr << message << '\n';
    }
    return condition;
}

bool equals(const std::vector<std::uint32_t>& values,
            std::initializer_list<std::uint32_t> expected) {
    return values == std::vector<std::uint32_t>(expected);
}

bool screen_regions_include_partial_triangle_coverage() {
    MeshBuffers buffers = screen_mesh();
    const ctex::mesh::MeshBinding mesh(buffers.descriptor());
    ctex::pick::SpatialIndex index(mesh);
    const ctex::pick::ScreenRegionView view{{100, 100}, identity, identity};
    const auto rectangle =
        ctex::pick::query_screen_rectangle(index, mesh, {{48.0F, 48.0F}, {52.0F, 52.0F}}, view);
    constexpr std::array lasso{
        ctex::pick::ScreenPosition{5.0F, 35.0F},  ctex::pick::ScreenPosition{35.0F, 35.0F},
        ctex::pick::ScreenPosition{25.0F, 50.0F}, ctex::pick::ScreenPosition{35.0F, 65.0F},
        ctex::pick::ScreenPosition{5.0F, 65.0F},
    };
    const auto lasso_result = ctex::pick::query_screen_lasso(index, mesh, lasso, view);
    return expect(equals(rectangle.triangle_indices, {1}),
                  "rectangle did not include an edge-crossing projected triangle") &&
           expect(equals(lasso_result.triangle_indices, {0}),
                  "lasso returned triangles outside its projected region");
}

bool world_regions_use_exact_triangle_intersection() {
    MeshBuffers buffers = world_mesh();
    const ctex::mesh::MeshBinding mesh(buffers.descriptor());
    ctex::pick::SpatialIndex index(mesh);
    const auto sphere = ctex::pick::query_world_sphere(index, mesh, {{1.0F, -0.05F, 0.0F}, 0.1F});
    const auto box =
        ctex::pick::query_world_box(index, mesh, {{1.4F, 1.4F, -0.1F}, {1.6F, 1.6F, 0.1F}});
    return expect(equals(sphere.triangle_indices, {0}),
                  "sphere missed a triangle edge within its radius") &&
           expect(equals(box.triangle_indices, {1}),
                  "box broad-phase overlap was not filtered by exact intersection");
}

bool degenerate_world_triangles_use_their_segments() {
    MeshBuffers buffers({{0.0F, 0.0F, 0.0F}, {1.0F, 0.0F, 0.0F}, {2.0F, 0.0F, 0.0F}});
    const ctex::mesh::MeshBinding mesh(buffers.descriptor());
    ctex::pick::SpatialIndex index(mesh);
    const auto sphere = ctex::pick::query_world_sphere(index, mesh, {{1.5F, 0.05F, 0.0F}, 0.1F});
    const auto box =
        ctex::pick::query_world_box(index, mesh, {{1.4F, -0.1F, -0.1F}, {1.6F, 0.1F, 0.1F}});
    return expect(equals(sphere.triangle_indices, {0}) && equals(box.triangle_indices, {0}),
                  "degenerate triangle segments were omitted from world regions");
}

bool region_queries_prune_large_meshes() {
    constexpr std::size_t triangle_count = 4096;
    constexpr std::uint32_t target = 3072;
    TriangleStrip buffers(triangle_count);
    const ctex::mesh::MeshBinding mesh(buffers.descriptor());
    ctex::pick::SpatialIndex index(mesh);
    const float target_x = static_cast<float>(target) * 2.0F;
    const auto box = ctex::pick::query_world_box(
        index, mesh, {{target_x, 0.0F, -0.1F}, {target_x + 0.5F, 0.5F, 0.1F}});

    auto translated_view = identity;
    translated_view.values[12] = -target_x;
    const ctex::pick::ScreenRegionView view{{100, 100}, translated_view, identity};
    const auto rectangle =
        ctex::pick::query_screen_rectangle(index, mesh, {{49.0F, 24.0F}, {76.0F, 51.0F}}, view);
    return expect(equals(box.triangle_indices, {target}), "box query omitted its target") &&
           expect(box.tested_leaf_triangles <= 4,
                  "box query scanned more than one bounded BVH leaf") &&
           expect(equals(rectangle.triangle_indices, {target}),
                  "screen query omitted its projected target") &&
           expect(rectangle.tested_leaf_triangles <= 4,
                  "screen query scanned more than one bounded BVH leaf");
}

bool invalid_regions_are_rejected() {
    MeshBuffers buffers = screen_mesh();
    const ctex::mesh::MeshBinding mesh(buffers.descriptor());
    ctex::pick::SpatialIndex index(mesh);
    const ctex::pick::ScreenRegionView view{{100, 100}, identity, identity};
    bool lasso_rejected = false;
    bool sphere_rejected = false;
    try {
        constexpr std::array points{ctex::pick::ScreenPosition{1.0F, 1.0F},
                                    ctex::pick::ScreenPosition{2.0F, 2.0F}};
        static_cast<void>(ctex::pick::query_screen_lasso(index, mesh, points, view));
    } catch (const std::invalid_argument&) {
        lasso_rejected = true;
    }
    try {
        static_cast<void>(ctex::pick::query_world_sphere(index, mesh, {{0.0F, 0.0F, 0.0F}, -1.0F}));
    } catch (const std::invalid_argument&) {
        sphere_rejected = true;
    }
    return expect(lasso_rejected && sphere_rejected, "invalid region input was accepted");
}

}  // namespace

int main() {
    return screen_regions_include_partial_triangle_coverage() &&
                   world_regions_use_exact_triangle_intersection() &&
                   degenerate_world_triangles_use_their_segments() &&
                   region_queries_prune_large_meshes() && invalid_regions_are_rejected()
               ? 0
               : 1;
}
