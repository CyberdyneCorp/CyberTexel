#ifndef CTEX_PICK_SPATIAL_INDEX_HPP
#define CTEX_PICK_SPATIAL_INDEX_HPP

#include <cstddef>
#include <cstdint>
#include <ctex/mesh/mesh.hpp>
#include <ctex/pick/ray.hpp>
#include <span>
#include <vector>

namespace ctex::pick {

struct RayCandidateQuery {
    std::vector<std::uint32_t> triangle_indices;
    std::size_t visited_nodes{};
    std::size_t tested_leaf_triangles{};
};

using PointCandidateQuery = RayCandidateQuery;
using RegionCandidateQuery = RayCandidateQuery;

// The kept half-space is normal . point + offset >= 0.
struct HalfSpace {
    mesh::Vec3f normal;
    float offset;
};

namespace detail {

struct SpatialBounds {
    mesh::Vec3f minimum;
    mesh::Vec3f maximum;
};

struct SpatialNode {
    SpatialBounds bounds;
    std::uint32_t first;
    std::uint32_t count;
    std::uint32_t right_child;
};

struct TriangleBuildData {
    SpatialBounds bounds;
    mesh::Vec3f centroid;
};

}  // namespace detail

class SpatialIndex {
public:
    explicit SpatialIndex(const mesh::MeshBinding& mesh);

    // Returns true only when a changed mesh revision caused a rebuild.
    bool synchronize(const mesh::MeshBinding& mesh);

    [[nodiscard]] RayCandidateQuery query_ray_candidates(const mesh::MeshBinding& mesh, Ray ray,
                                                         float maximum_distance);
    [[nodiscard]] PointCandidateQuery query_point_candidates(const mesh::MeshBinding& mesh,
                                                             mesh::Vec3f point,
                                                             float maximum_distance);
    [[nodiscard]] RegionCandidateQuery query_box_candidates(const mesh::MeshBinding& mesh,
                                                            mesh::Vec3f minimum,
                                                            mesh::Vec3f maximum);
    [[nodiscard]] RegionCandidateQuery query_half_space_candidates(
        const mesh::MeshBinding& mesh, std::span<const HalfSpace> half_spaces);
    [[nodiscard]] mesh::MeshRevision mesh_revision() const noexcept { return mesh_revision_; }
    [[nodiscard]] std::size_t build_count() const noexcept { return build_count_; }
    [[nodiscard]] std::size_t node_count() const noexcept { return nodes_.size(); }
    [[nodiscard]] std::size_t triangle_count() const noexcept { return triangle_order_.size(); }

private:
    void build(const mesh::MeshBinding& mesh);
    std::uint32_t build_node(std::uint32_t first, std::uint32_t count,
                             const std::vector<detail::TriangleBuildData>& triangles);

    std::vector<detail::SpatialNode> nodes_;
    std::vector<std::uint32_t> triangle_order_;
    mesh::MeshRevision mesh_revision_{};
    std::size_t build_count_{};
};

}  // namespace ctex::pick

#endif
