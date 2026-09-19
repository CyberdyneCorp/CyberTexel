#include <algorithm>
#include <atomic>
#include <cmath>
#include <ctex/mesh/mesh.hpp>
#include <limits>
#include <set>
#include <stdexcept>
#include <string>

namespace ctex::mesh {
namespace {

std::atomic<MeshRevision> next_mesh_revision{1};

MeshRevision issue_mesh_revision() {
    MeshRevision revision = next_mesh_revision.load(std::memory_order_relaxed);
    while (revision != std::numeric_limits<MeshRevision>::max()) {
        if (next_mesh_revision.compare_exchange_weak(revision, revision + 1,
                                                     std::memory_order_relaxed)) {
            return revision;
        }
    }
    throw std::overflow_error("mesh revision space is exhausted");
}

bool finite(Vec2f value) { return std::isfinite(value.x) && std::isfinite(value.y); }

bool finite(Vec3f value) {
    return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z);
}

bool finite(Vec4f value) {
    return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z) &&
           std::isfinite(value.w);
}

std::string_view partition_kind_name(PartitionKind kind) {
    switch (kind) {
        case PartitionKind::material:
            return "material";
        case PartitionKind::object:
            return "object";
        case PartitionKind::submesh:
            return "submesh";
        case PartitionKind::explicit_faces:
            return "faces";
    }
    throw std::invalid_argument("unsupported partition kind");
}

std::string length_prefixed(std::string_view value) {
    return std::to_string(value.size()) + ":" + std::string(value);
}

void validate_vertex_attributes(const MeshDescriptor& descriptor) {
    if (descriptor.positions.empty()) {
        throw std::invalid_argument("mesh must contain at least one vertex");
    }
    if (descriptor.normals.size() != descriptor.positions.size()) {
        throw std::invalid_argument("mesh normals must match the position count");
    }
    if (!descriptor.vertex_colors.empty() &&
        descriptor.vertex_colors.size() != descriptor.positions.size()) {
        throw std::invalid_argument("mesh vertex colors must be empty or match the position count");
    }
    if (!std::all_of(descriptor.positions.begin(), descriptor.positions.end(),
                     [](Vec3f value) { return finite(value); }) ||
        !std::all_of(descriptor.normals.begin(), descriptor.normals.end(),
                     [](Vec3f value) { return finite(value); }) ||
        !std::all_of(descriptor.vertex_colors.begin(), descriptor.vertex_colors.end(),
                     [](Vec4f value) { return finite(value); })) {
        throw std::invalid_argument("mesh vertex attributes must contain finite values");
    }
}

void validate_indices(const MeshDescriptor& descriptor) {
    if (descriptor.triangle_indices.empty() || descriptor.triangle_indices.size() % 3 != 0) {
        throw std::invalid_argument("mesh indices must describe one or more complete triangles");
    }
    for (std::uint32_t index : descriptor.triangle_indices) {
        if (index >= descriptor.positions.size()) {
            throw std::invalid_argument("mesh triangle index is outside the vertex buffer");
        }
    }
}

void validate_uv_sets(const MeshDescriptor& descriptor) {
    if (descriptor.uv_sets.empty()) {
        throw std::invalid_argument("mesh must provide at least one UV set");
    }
    std::set<std::string_view> names;
    bool found_default = false;
    for (const UvSetView& uv_set : descriptor.uv_sets) {
        if (uv_set.name.empty() || !names.insert(uv_set.name).second) {
            throw std::invalid_argument("mesh UV set names must be non-empty and unique");
        }
        if (uv_set.values.size() != descriptor.positions.size()) {
            throw std::invalid_argument("mesh UV values must match the position count");
        }
        if (!std::all_of(uv_set.values.begin(), uv_set.values.end(),
                         [](Vec2f value) { return finite(value); })) {
            throw std::invalid_argument("mesh UV values must be finite");
        }
        found_default |= uv_set.name == descriptor.default_uv_set;
    }
    if (!found_default) {
        throw std::invalid_argument("mesh default UV set is not present");
    }
}

void validate_partitions(const MeshDescriptor& descriptor) {
    const std::size_t triangle_count = descriptor.triangle_indices.size() / 3;
    if (descriptor.partitions.empty()) {
        throw std::invalid_argument("mesh must provide at least one texture-set partition");
    }
    if (descriptor.face_partition_indices.size() != triangle_count) {
        throw std::invalid_argument("every mesh face must have exactly one partition assignment");
    }
    if (descriptor.face_material_ids.size() != triangle_count) {
        throw std::invalid_argument("every mesh face must have exactly one material identifier");
    }
    std::set<std::pair<PartitionKind, std::string_view>> identities;
    for (const MeshPartition& partition : descriptor.partitions) {
        if (partition.stable_key.empty() || partition.display_name.empty()) {
            throw std::invalid_argument("mesh partition keys and names must not be empty");
        }
        if (!identities.emplace(partition.kind, partition.stable_key).second) {
            throw std::invalid_argument("mesh partition stable identities must be unique");
        }
    }
    for (std::uint32_t partition_index : descriptor.face_partition_indices) {
        if (partition_index >= descriptor.partitions.size()) {
            throw std::invalid_argument("mesh face references a missing partition");
        }
    }
}

}  // namespace

MeshView::MeshView(MeshDescriptor descriptor) : descriptor_(descriptor) {
    validate_vertex_attributes(descriptor_);
    validate_indices(descriptor_);
    validate_uv_sets(descriptor_);
    validate_partitions(descriptor_);
}

MeshAttributeDescription MeshView::attributes() const noexcept {
    return {
        descriptor_.positions.size(),
        triangle_count(),
        descriptor_.uv_sets.size(),
        !descriptor_.vertex_colors.empty(),
    };
}

std::size_t MeshView::triangle_count() const noexcept {
    return descriptor_.triangle_indices.size() / 3;
}

const UvSetView& MeshView::uv_set(std::string_view name) const {
    for (const UvSetView& candidate : descriptor_.uv_sets) {
        if (candidate.name == name) {
            return candidate;
        }
    }
    throw std::out_of_range("mesh UV set is not present: " + std::string(name));
}

const MeshPartition& MeshView::partition_for_face(std::size_t face_index) const {
    if (face_index >= triangle_count()) {
        throw std::out_of_range("mesh face index is outside the triangle buffer");
    }
    return descriptor_.partitions[descriptor_.face_partition_indices[face_index]];
}

MeshBinding::MeshBinding(MeshDescriptor descriptor)
    : view_(descriptor), revision_(issue_mesh_revision()) {}

void MeshBinding::replace(MeshDescriptor descriptor) {
    MeshView replacement(descriptor);
    const MeshRevision replacement_revision = issue_mesh_revision();
    view_ = replacement;
    revision_ = replacement_revision;
}

std::string texture_set_stable_id(PartitionKind kind, std::string_view partition_key,
                                  std::string_view uv_set) {
    if (partition_key.empty() || uv_set.empty()) {
        throw std::invalid_argument("texture-set partition key and UV set must not be empty");
    }
    return std::string(partition_kind_name(kind)) + "/" + length_prefixed(partition_key) + "/uv/" +
           length_prefixed(uv_set);
}

}  // namespace ctex::mesh
