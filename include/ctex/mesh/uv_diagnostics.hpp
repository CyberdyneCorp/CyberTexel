#ifndef CTEX_MESH_UV_DIAGNOSTICS_HPP
#define CTEX_MESH_UV_DIAGNOSTICS_HPP

#include <cstddef>
#include <cstdint>
#include <ctex/mesh/mesh.hpp>
#include <string_view>
#include <vector>

namespace ctex::mesh {

struct UvOverlapReport {
    std::vector<std::uint32_t> face_indices;
    std::size_t overlap_pair_count{};
    std::size_t candidate_pair_count{};
    friend bool operator==(const UvOverlapReport&, const UvOverlapReport&) = default;
};

// Finds positive-area overlap within one texture-set partition. Boundary-only
// contact and degenerate UV triangles are not overlaps. Face indices are unique
// and sorted; the work counters make the broad-phase cost inspectable.
[[nodiscard]] UvOverlapReport analyze_uv_overlaps(const MeshView& mesh, std::string_view uv_set,
                                                  std::uint32_t partition_index);

}  // namespace ctex::mesh

#endif
