#ifndef CTEX_PICK_UV_INDEX_HPP
#define CTEX_PICK_UV_INDEX_HPP

#include <cstddef>
#include <cstdint>
#include <ctex/mesh/mesh.hpp>
#include <string>
#include <string_view>
#include <vector>

namespace ctex::pick {

struct UvCandidateQuery {
    std::vector<std::uint32_t> triangle_indices;
    std::size_t visited_nodes{};
    std::size_t tested_leaf_triangles{};
};

namespace detail {

struct UvBounds {
    mesh::Vec2f minimum;
    mesh::Vec2f maximum;
};

struct UvNode {
    UvBounds bounds;
    std::uint32_t first;
    std::uint32_t count;
    std::uint32_t right_child;
};

struct UvTriangleBuildData {
    UvBounds bounds;
    mesh::Vec2f centroid;
};

}  // namespace detail

class UvSpatialIndex {
public:
    UvSpatialIndex(const mesh::MeshBinding& mesh, std::string_view uv_set);

    bool synchronize(const mesh::MeshBinding& mesh);
    [[nodiscard]] UvCandidateQuery query_candidates(const mesh::MeshBinding& mesh,
                                                    mesh::Vec2f coordinate);

    [[nodiscard]] std::string_view uv_set() const noexcept { return uv_set_; }
    [[nodiscard]] mesh::MeshRevision mesh_revision() const noexcept { return mesh_revision_; }
    [[nodiscard]] std::size_t build_count() const noexcept { return build_count_; }
    [[nodiscard]] std::size_t node_count() const noexcept { return nodes_.size(); }

private:
    void build(const mesh::MeshBinding& mesh);
    std::uint32_t build_node(std::uint32_t first, std::uint32_t count,
                             const std::vector<detail::UvTriangleBuildData>& triangles);

    std::string uv_set_;
    std::vector<detail::UvNode> nodes_;
    std::vector<std::uint32_t> triangle_order_;
    mesh::MeshRevision mesh_revision_{};
    std::size_t build_count_{};
};

}  // namespace ctex::pick

#endif
