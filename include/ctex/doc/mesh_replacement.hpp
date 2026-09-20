#ifndef CTEX_DOC_MESH_REPLACEMENT_HPP
#define CTEX_DOC_MESH_REPLACEMENT_HPP

#include <cstddef>
#include <cstdint>
#include <ctex/doc/document.hpp>
#include <ctex/mesh/mesh.hpp>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace ctex::doc {

enum class MeshUvChange : std::uint8_t {
    unchanged,
    changed,
    source_partition_missing,
    replacement_partition_missing,
    uv_set_missing,
};

struct TextureSetMeshReplacement {
    std::string texture_set_id;
    MeshUvChange change{};
    std::optional<std::uint32_t> source_partition_index;
    std::optional<std::uint32_t> replacement_partition_index;
    std::size_t source_face_count{};
    std::size_t replacement_face_count{};
    friend bool operator==(const TextureSetMeshReplacement&,
                           const TextureSetMeshReplacement&) = default;
};

class MeshReplacementPlan {
public:
    [[nodiscard]] mesh::MeshRevision source_revision() const noexcept { return source_revision_; }
    [[nodiscard]] std::span<const TextureSetMeshReplacement> texture_sets() const noexcept {
        return texture_sets_;
    }

private:
    friend MeshReplacementPlan analyze_mesh_replacement(const TextureDocument& document,
                                                        const mesh::MeshBinding& source,
                                                        const mesh::MeshView& replacement);

    MeshReplacementPlan(mesh::MeshRevision source_revision,
                        std::vector<TextureSetMeshReplacement> texture_sets)
        : source_revision_(source_revision), texture_sets_(std::move(texture_sets)) {}

    mesh::MeshRevision source_revision_{};
    std::vector<TextureSetMeshReplacement> texture_sets_;
};

enum class MeshReplacementPolicy : std::uint8_t { keep_texels, request_reprojection, clear };

struct MeshReplacementDecision {
    std::string_view texture_set_id;
    MeshReplacementPolicy policy{};
};

struct MeshReplacementApplyReport {
    bool replacement_ready{};
    std::vector<std::string> kept_texture_sets;
    std::vector<std::string> cleared_texture_sets;
    std::vector<std::string> reprojection_pending_texture_sets;
};

[[nodiscard]] MeshReplacementPlan analyze_mesh_replacement(const TextureDocument& document,
                                                           const mesh::MeshBinding& source,
                                                           const mesh::MeshView& replacement);

// Applies only document policy. The caller publishes the already validated
// replacement mesh only when replacement_ready is true. Reprojection requests
// preserve every current pixel and leave the whole transaction pending for the
// editable-authoring reprojection path.
[[nodiscard]] MeshReplacementApplyReport apply_mesh_replacement_policies(
    TextureDocument& document, const mesh::MeshBinding& source, const MeshReplacementPlan& plan,
    std::span<const MeshReplacementDecision> decisions);

}  // namespace ctex::doc

#endif
