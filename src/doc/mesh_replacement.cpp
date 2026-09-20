#include <algorithm>
#include <array>
#include <ctex/doc/mesh_replacement.hpp>
#include <iterator>
#include <map>
#include <stdexcept>
#include <tuple>
#include <utility>

namespace ctex::doc {
namespace {

mesh::PartitionKind mesh_partition_kind(PartitionSourceKind kind) {
    switch (kind) {
        case PartitionSourceKind::material:
            return mesh::PartitionKind::material;
        case PartitionSourceKind::object:
            return mesh::PartitionKind::object;
        case PartitionSourceKind::submesh:
            return mesh::PartitionKind::submesh;
        case PartitionSourceKind::explicit_faces:
            return mesh::PartitionKind::explicit_faces;
    }
    throw std::invalid_argument("texture-set partition kind is invalid");
}

std::optional<std::uint32_t> partition_index(const mesh::MeshView& mesh_view,
                                             const TextureSetDescriptor& texture_set) {
    const auto& partitions = mesh_view.descriptor().partitions;
    const auto found = std::ranges::find_if(partitions, [&](const mesh::MeshPartition& partition) {
        return partition.kind == mesh_partition_kind(texture_set.partition_kind) &&
               partition.stable_key == texture_set.partition_key;
    });
    if (found == partitions.end()) {
        return std::nullopt;
    }
    return static_cast<std::uint32_t>(std::distance(partitions.begin(), found));
}

const mesh::UvSetView* find_uv_set(const mesh::MeshView& mesh_view, std::string_view name) {
    const auto& sets = mesh_view.descriptor().uv_sets;
    const auto found = std::ranges::find(sets, name, &mesh::UvSetView::name);
    return found == sets.end() ? nullptr : &*found;
}

struct UvPointKey {
    float x{};
    float y{};
    friend bool operator==(UvPointKey, UvPointKey) = default;
};

bool point_less(UvPointKey left, UvPointKey right) {
    return std::tie(left.x, left.y) < std::tie(right.x, right.y);
}

struct UvTriangleKey {
    std::array<UvPointKey, 3> points{};
    friend bool operator==(const UvTriangleKey&, const UvTriangleKey&) = default;
};

bool triangle_less(const UvTriangleKey& left, const UvTriangleKey& right) {
    return std::lexicographical_compare(left.points.begin(), left.points.end(),
                                        right.points.begin(), right.points.end(), point_less);
}

UvPointKey point_key(mesh::Vec2f point) {
    return {.x = point.x == 0.0F ? 0.0F : point.x, .y = point.y == 0.0F ? 0.0F : point.y};
}

std::vector<UvTriangleKey> partition_layout(const mesh::MeshView& mesh_view,
                                            const mesh::UvSetView& uv_set,
                                            std::uint32_t selected_partition) {
    const mesh::MeshDescriptor& descriptor = mesh_view.descriptor();
    std::vector<UvTriangleKey> result;
    for (std::size_t face = 0; face < mesh_view.triangle_count(); ++face) {
        if (descriptor.face_partition_indices[face] != selected_partition) {
            continue;
        }
        const std::size_t first = face * 3;
        UvTriangleKey triangle{{point_key(uv_set.values[descriptor.triangle_indices[first]]),
                                point_key(uv_set.values[descriptor.triangle_indices[first + 1]]),
                                point_key(uv_set.values[descriptor.triangle_indices[first + 2]])}};
        std::ranges::sort(triangle.points, point_less);
        result.push_back(triangle);
    }
    std::ranges::sort(result, triangle_less);
    return result;
}

std::size_t partition_face_count(const mesh::MeshView& mesh_view, std::uint32_t partition) {
    return static_cast<std::size_t>(
        std::ranges::count(mesh_view.descriptor().face_partition_indices, partition));
}

TextureSetMeshReplacement analyze_texture_set(const TextureSet& texture_set,
                                              const mesh::MeshView& source,
                                              const mesh::MeshView& replacement) {
    const TextureSetDescriptor descriptor = texture_set.descriptor();
    TextureSetMeshReplacement report{
        .texture_set_id = texture_set.id(),
        .change = {},
        .source_partition_index = {},
        .replacement_partition_index = {},
        .source_face_count = 0,
        .replacement_face_count = 0,
    };
    report.source_partition_index = partition_index(source, descriptor);
    report.replacement_partition_index = partition_index(replacement, descriptor);
    if (!report.source_partition_index) {
        report.change = MeshUvChange::source_partition_missing;
        return report;
    }
    report.source_face_count = partition_face_count(source, *report.source_partition_index);
    if (!report.replacement_partition_index) {
        report.change = MeshUvChange::replacement_partition_missing;
        return report;
    }
    report.replacement_face_count =
        partition_face_count(replacement, *report.replacement_partition_index);
    const mesh::UvSetView* source_uv = find_uv_set(source, descriptor.uv_set);
    const mesh::UvSetView* replacement_uv = find_uv_set(replacement, descriptor.uv_set);
    if (source_uv == nullptr || replacement_uv == nullptr) {
        report.change = MeshUvChange::uv_set_missing;
        return report;
    }
    report.change =
        partition_layout(source, *source_uv, *report.source_partition_index) ==
                partition_layout(replacement, *replacement_uv, *report.replacement_partition_index)
            ? MeshUvChange::unchanged
            : MeshUvChange::changed;
    return report;
}

std::map<std::string_view, MeshReplacementPolicy> validated_decisions(
    const TextureDocument& document, const mesh::MeshBinding& source,
    const MeshReplacementPlan& plan, std::span<const MeshReplacementDecision> decisions) {
    if (source.revision() != plan.source_revision()) {
        throw std::invalid_argument("mesh replacement plan is stale");
    }
    std::vector<std::string_view> planned_ids;
    planned_ids.reserve(plan.texture_sets().size());
    for (const TextureSetMeshReplacement& texture_set : plan.texture_sets()) {
        if (texture_set.texture_set_id.empty() ||
            texture_set.change > MeshUvChange::uv_set_missing) {
            throw std::invalid_argument("mesh replacement plan is invalid");
        }
        planned_ids.push_back(texture_set.texture_set_id);
    }
    std::ranges::sort(planned_ids);
    if (std::ranges::adjacent_find(planned_ids) != planned_ids.end()) {
        throw std::invalid_argument("mesh replacement plan repeats a texture set");
    }
    const std::vector<std::string> document_ids = document.texture_set_ids();
    if (!std::ranges::equal(planned_ids, document_ids)) {
        throw std::invalid_argument("mesh replacement plan does not match the document");
    }
    std::map<std::string_view, MeshReplacementPolicy> result;
    for (const MeshReplacementDecision& decision : decisions) {
        if (decision.policy != MeshReplacementPolicy::keep_texels &&
            decision.policy != MeshReplacementPolicy::request_reprojection &&
            decision.policy != MeshReplacementPolicy::clear) {
            throw std::invalid_argument("mesh replacement policy is invalid");
        }
        if (decision.texture_set_id.empty() ||
            !result.emplace(decision.texture_set_id, decision.policy).second) {
            throw std::invalid_argument("mesh replacement decisions require unique texture sets");
        }
    }
    for (const TextureSetMeshReplacement& texture_set : plan.texture_sets()) {
        const auto found = result.find(texture_set.texture_set_id);
        if (texture_set.change == MeshUvChange::unchanged) {
            if (found != result.end()) {
                throw std::invalid_argument(
                    "unchanged texture sets must not receive a replacement decision");
            }
        } else if (found == result.end()) {
            throw std::invalid_argument("changed texture set is missing a replacement decision: " +
                                        texture_set.texture_set_id);
        }
    }
    if (result.size() != decisions.size() ||
        result.size() != static_cast<std::size_t>(std::ranges::count_if(
                             plan.texture_sets(), [](const TextureSetMeshReplacement& value) {
                                 return value.change != MeshUvChange::unchanged;
                             }))) {
        throw std::invalid_argument("mesh replacement decision names an unknown texture set");
    }
    return result;
}

}  // namespace

MeshReplacementPlan analyze_mesh_replacement(const TextureDocument& document,
                                             const mesh::MeshBinding& source,
                                             const mesh::MeshView& replacement) {
    std::vector<TextureSetMeshReplacement> texture_sets;
    for (const std::string& identifier : document.texture_set_ids()) {
        texture_sets.push_back(
            analyze_texture_set(document.texture_set(identifier), source.view(), replacement));
    }
    return MeshReplacementPlan(source.revision(), std::move(texture_sets));
}

MeshReplacementApplyReport apply_mesh_replacement_policies(
    TextureDocument& document, const mesh::MeshBinding& source, const MeshReplacementPlan& plan,
    std::span<const MeshReplacementDecision> decisions) {
    const auto policies = validated_decisions(document, source, plan, decisions);
    MeshReplacementApplyReport report;
    std::vector<std::string> clear_texture_sets;
    for (const TextureSetMeshReplacement& texture_set : plan.texture_sets()) {
        if (texture_set.change == MeshUvChange::unchanged) {
            report.kept_texture_sets.push_back(texture_set.texture_set_id);
            continue;
        }
        switch (policies.at(texture_set.texture_set_id)) {
            case MeshReplacementPolicy::keep_texels:
                report.kept_texture_sets.push_back(texture_set.texture_set_id);
                break;
            case MeshReplacementPolicy::request_reprojection:
                report.reprojection_pending_texture_sets.push_back(texture_set.texture_set_id);
                break;
            case MeshReplacementPolicy::clear:
                clear_texture_sets.push_back(texture_set.texture_set_id);
                break;
        }
    }
    report.replacement_ready = report.reprojection_pending_texture_sets.empty();
    if (!report.replacement_ready) {
        return report;
    }
    for (const std::string& identifier : clear_texture_sets) {
        if (!document.texture_set(identifier).can_clear_channels()) {
            throw std::overflow_error("texture-set channel revision space is exhausted: " +
                                      identifier);
        }
    }
    for (const std::string& identifier : clear_texture_sets) {
        document.texture_set(identifier).clear_channels();
    }
    report.cleared_texture_sets = std::move(clear_texture_sets);
    return report;
}

}  // namespace ctex::doc
