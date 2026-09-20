#ifndef CTEX_MESH_UV_DIAGNOSTICS_HPP
#define CTEX_MESH_UV_DIAGNOSTICS_HPP

#include <cstddef>
#include <cstdint>
#include <ctex/mesh/mesh.hpp>
#include <string_view>
#include <vector>

namespace ctex::mesh {

inline constexpr std::uint64_t maximum_uv_coverage_samples = 16'384ULL * 16'384ULL;

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

struct UvCoverageRequest {
    std::uint32_t width{};
    std::uint32_t height{};
};

struct UvCoverageReport {
    std::vector<std::uint32_t> outside_face_indices;
    std::size_t selected_face_count{};
    std::size_t covered_sample_count{};
    std::size_t uncovered_sample_count{};
    std::size_t tested_sample_count{};
    double uncovered_fraction{};
    friend bool operator==(const UvCoverageReport&, const UvCoverageReport&) = default;
};

// Samples the non-UDIM unit square at texture-texel centres. The returned
// covered and uncovered counts are exact for the requested resolution.
[[nodiscard]] UvCoverageReport analyze_uv_coverage(const MeshView& mesh, std::string_view uv_set,
                                                   std::uint32_t partition_index,
                                                   UvCoverageRequest request);

}  // namespace ctex::mesh

#endif
