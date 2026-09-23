#ifndef CTEX_DOC_MESH_REPROJECTION_HPP
#define CTEX_DOC_MESH_REPROJECTION_HPP

#include <array>
#include <cstddef>
#include <cstdint>
#include <ctex/doc/document.hpp>
#include <ctex/mesh/mesh.hpp>
#include <functional>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace ctex::doc {

enum class ReprojectionMappingStatus : std::uint8_t { mapped, unmapped, ambiguous };
enum class ReprojectionHolePolicy : std::uint8_t { retain_target, channel_default };
enum class ReprojectionAmbiguityPolicy : std::uint8_t { refuse, nearest_then_lowest_triangle };

struct MeshReprojectionLimits {
    double maximum_distance{};
    double maximum_normal_angle_radians{};
    bool require_visibility{};
    double visibility_epsilon{1.0e-5};
    double ambiguity_distance_epsilon{1.0e-6};
    std::size_t maximum_work_items{};
    std::size_t progress_interval{256};
};

struct MeshReprojectionControl {
    std::function<bool()> is_cancelled;
    std::function<void(std::size_t completed_work_items)> report_progress;
};

struct ReprojectionTexelMapping {
    std::string texture_set_id;
    std::uint32_t x{};
    std::uint32_t y{};
    ReprojectionMappingStatus status{};
    std::uint32_t target_triangle{};
    std::uint32_t source_triangle{};
    std::array<double, 3> target_barycentric{};
    std::array<double, 3> source_barycentric{};
    std::array<double, 2> source_uv{};
    double distance{};
    double normal_angle_radians{};
    std::size_t candidate_count{};
    friend bool operator==(const ReprojectionTexelMapping&,
                           const ReprojectionTexelMapping&) = default;
};

struct ReprojectionEntryMapping {
    struct Point {
        ReprojectionMappingStatus status{};
        std::array<double, 3> position{};
        std::array<double, 3> normal{};
        std::uint32_t triangle{};
        std::array<double, 3> barycentric{};
        double distance{};
        std::size_t candidate_count{};
        friend bool operator==(const Point&, const Point&) = default;
    };

    std::string texture_set_id;
    std::string entry_id;
    std::uint64_t entry_revision{};
    std::size_t point_count{};
    std::size_t unmapped_point_count{};
    std::size_t ambiguous_point_count{};
    std::vector<Point> points;
    friend bool operator==(const ReprojectionEntryMapping&,
                           const ReprojectionEntryMapping&) = default;
};

struct ReprojectionChannelState {
    std::string texture_set_id;
    std::string semantic_id;
    bool enabled{};
    ChannelRevisionCursor revision{};
    friend bool operator==(const ReprojectionChannelState&,
                           const ReprojectionChannelState&) = default;
};

struct MeshReprojectionPreflight {
    mesh::MeshRevision source_revision{};
    mesh::MeshRevision replacement_revision{};
    mesh::MeshRevision published_revision{};
    MeshReprojectionLimits limits;
    std::vector<ReprojectionTexelMapping> texels;
    std::vector<ReprojectionEntryMapping> affected_entries;
    std::vector<ReprojectionChannelState> source_channels;
    std::size_t mapped_texel_count{};
    std::size_t unmapped_texel_count{};
    std::size_t ambiguous_texel_count{};
    std::size_t tested_candidate_count{};
};

enum class MeshReprojectionErrorCode : std::uint8_t {
    invalid_request,
    over_budget,
    cancelled,
    stale_preflight,
    unresolved_ambiguity,
    invalid_attachment,
};

class MeshReprojectionError final : public std::runtime_error {
public:
    MeshReprojectionError(MeshReprojectionErrorCode code, std::string message);
    [[nodiscard]] MeshReprojectionErrorCode code() const noexcept { return code_; }

private:
    MeshReprojectionErrorCode code_;
};

struct MeshReprojectionCommitReport {
    std::size_t reprojected_texel_count{};
    std::size_t retained_hole_count{};
    std::size_t defaulted_hole_count{};
    std::size_t resolved_ambiguity_count{};
    std::size_t transformed_tangent_normal_count{};
    std::vector<std::string> reprojected_entries;
};

[[nodiscard]] MeshReprojectionPreflight preflight_mesh_reprojection(
    const TextureDocument& document, const mesh::MeshBinding& source,
    const mesh::MeshBinding& replacement, const MeshReprojectionLimits& limits,
    const MeshReprojectionControl& control = {},
    std::span<const std::string_view> texture_set_ids = {});

// Stages all pixels and editable attachments before publishing any mutation.
// The caller may publish the replacement mesh after this returns successfully.
[[nodiscard]] MeshReprojectionCommitReport commit_mesh_reprojection(
    TextureDocument& document, const mesh::MeshBinding& source,
    const mesh::MeshBinding& replacement, const MeshReprojectionPreflight& preflight,
    ReprojectionHolePolicy hole_policy, ReprojectionAmbiguityPolicy ambiguity_policy);

}  // namespace ctex::doc

#endif
