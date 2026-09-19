#ifndef CTEX_PICK_REGION_HPP
#define CTEX_PICK_REGION_HPP

#include <cstddef>
#include <cstdint>
#include <ctex/mesh/mesh.hpp>
#include <ctex/pick/ray.hpp>
#include <span>
#include <vector>

namespace ctex::pick {

class SpatialIndex;

struct ScreenRegionView {
    ViewportSize viewport;
    Mat4f view;
    Mat4f projection;
};

struct ScreenRectangle {
    ScreenPosition minimum;
    ScreenPosition maximum;
};

struct WorldSphere {
    mesh::Vec3f center;
    float radius;
};

struct WorldBox {
    mesh::Vec3f minimum;
    mesh::Vec3f maximum;
};

struct RegionQueryResult {
    // Sorted by triangle index, independent of BVH layout and traversal order.
    std::vector<std::uint32_t> triangle_indices;
    std::size_t visited_nodes{};
    std::size_t tested_leaf_triangles{};
};

[[nodiscard]] RegionQueryResult query_screen_rectangle(SpatialIndex& index,
                                                       const mesh::MeshBinding& mesh,
                                                       ScreenRectangle rectangle,
                                                       const ScreenRegionView& view);

[[nodiscard]] RegionQueryResult query_screen_lasso(SpatialIndex& index,
                                                   const mesh::MeshBinding& mesh,
                                                   std::span<const ScreenPosition> points,
                                                   const ScreenRegionView& view);

[[nodiscard]] RegionQueryResult query_world_sphere(SpatialIndex& index,
                                                   const mesh::MeshBinding& mesh,
                                                   WorldSphere sphere);

[[nodiscard]] RegionQueryResult query_world_box(SpatialIndex& index, const mesh::MeshBinding& mesh,
                                                WorldBox box);

}  // namespace ctex::pick

#endif
