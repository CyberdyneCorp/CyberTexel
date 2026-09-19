#include <algorithm>
#include <atomic>
#include <cmath>
#include <ctex/mesh/mesh.hpp>
#include <limits>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

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

Vec3f subtract(Vec3f left, Vec3f right) {
    return {left.x - right.x, left.y - right.y, left.z - right.z};
}

Vec3f multiply(Vec3f value, float scale) {
    return {value.x * scale, value.y * scale, value.z * scale};
}

float dot(Vec3f left, Vec3f right) {
    return left.x * right.x + left.y * right.y + left.z * right.z;
}

Vec3f cross(Vec3f left, Vec3f right) {
    return {left.y * right.z - left.z * right.y, left.z * right.x - left.x * right.z,
            left.x * right.y - left.y * right.x};
}

Vec3f normalize(Vec3f value, std::string_view subject) {
    const float length_squared = dot(value, value);
    if (!std::isfinite(length_squared) || length_squared <= 1.0e-20F) {
        throw std::invalid_argument(std::string(subject) + " must have non-zero finite length");
    }
    return multiply(value, 1.0F / std::sqrt(length_squared));
}

Vec3f fallback_tangent(Vec3f normal) {
    const Vec3f axis =
        std::abs(normal.x) < 0.9F ? Vec3f{1.0F, 0.0F, 0.0F} : Vec3f{0.0F, 1.0F, 0.0F};
    return normalize(cross(axis, normal), "generated tangent");
}

void validate_tangent_frame_descriptor(const TangentFrameDescriptor& frame,
                                       const MeshDescriptor& descriptor) {
    static_cast<void>(tangent_basis_algorithm_name(frame.algorithm));
    if (frame.algorithm_version == 0 || frame.uv_set.empty()) {
        throw std::invalid_argument("tangent frame requires an algorithm version and UV set");
    }
    if (frame.normal_orientation != NormalOrientation::vertex_normals &&
        frame.normal_orientation != NormalOrientation::inverted_vertex_normals) {
        throw std::invalid_argument("tangent frame normal orientation is invalid");
    }
    if (frame.coordinate_handedness != CoordinateSystemHandedness::right_handed &&
        frame.coordinate_handedness != CoordinateSystemHandedness::left_handed) {
        throw std::invalid_argument("tangent frame coordinate handedness is invalid");
    }
    if (frame.uv_v_axis != UvVAxis::upward && frame.uv_v_axis != UvVAxis::downward) {
        throw std::invalid_argument("tangent frame UV convention is invalid");
    }
    if (frame.handedness_encoding != TangentHandednessEncoding::tangent_w_sign) {
        throw std::invalid_argument("tangent frame handedness encoding is invalid");
    }
    const bool has_uv_set = std::ranges::any_of(
        descriptor.uv_sets,
        [&](const UvSetView& candidate) { return candidate.name == frame.uv_set; });
    if (!has_uv_set) {
        throw std::invalid_argument("tangent frame names a missing UV set: " + frame.uv_set);
    }
}

void validate_supplied_tangents(const MeshDescriptor& descriptor) {
    if (descriptor.corner_tangents.size() != descriptor.triangle_indices.size()) {
        throw std::invalid_argument("supplied tangents must provide one value per triangle corner");
    }
    for (std::size_t corner = 0; corner < descriptor.corner_tangents.size(); ++corner) {
        const Vec4f tangent = descriptor.corner_tangents[corner];
        if (!finite(tangent) || std::abs(std::abs(tangent.w) - 1.0F) > 1.0e-5F) {
            throw std::invalid_argument("supplied tangent must be finite with handedness +1 or -1");
        }
        const Vec3f tangent_xyz{tangent.x, tangent.y, tangent.z};
        const float tangent_length = std::sqrt(dot(tangent_xyz, tangent_xyz));
        if (std::abs(tangent_length - 1.0F) > 1.0e-4F) {
            throw std::invalid_argument("supplied tangent direction must have unit length");
        }
        const Vec3f direction = normalize(tangent_xyz, "supplied tangent");
        const Vec3f normal =
            normalize(descriptor.normals[descriptor.triangle_indices[corner]], "mesh normal");
        if (std::abs(dot(direction, normal)) > 1.0e-4F) {
            throw std::invalid_argument("supplied tangent must be orthogonal to its vertex normal");
        }
    }
}

const UvSetView& find_uv_set(const MeshDescriptor& descriptor, std::string_view name) {
    const auto found = std::ranges::find(descriptor.uv_sets, name, &UvSetView::name);
    if (found == descriptor.uv_sets.end()) {
        throw std::invalid_argument("tangent generation UV set is not present");
    }
    return *found;
}

Vec4f generated_corner_tangent(Vec3f raw_tangent, Vec3f raw_bitangent, Vec3f source_normal) {
    const Vec3f normal = normalize(source_normal, "mesh normal");
    const Vec3f projected = subtract(raw_tangent, multiply(normal, dot(normal, raw_tangent)));
    const Vec3f tangent = dot(projected, projected) <= 1.0e-20F
                              ? fallback_tangent(normal)
                              : normalize(projected, "generated tangent");
    const float handedness = dot(cross(normal, tangent), raw_bitangent) < 0.0F ? -1.0F : 1.0F;
    return {tangent.x, tangent.y, tangent.z, handedness};
}

std::vector<Vec4f> generate_tangents(const MeshDescriptor& descriptor,
                                     const TangentFrameDescriptor& frame) {
    const UvSetView& uv_set = find_uv_set(descriptor, frame.uv_set);
    std::vector<Vec4f> result(descriptor.triangle_indices.size());
    for (std::size_t triangle = 0; triangle < descriptor.triangle_indices.size(); triangle += 3) {
        const std::uint32_t i0 = descriptor.triangle_indices[triangle];
        const std::uint32_t i1 = descriptor.triangle_indices[triangle + 1];
        const std::uint32_t i2 = descriptor.triangle_indices[triangle + 2];
        const Vec3f edge1 = subtract(descriptor.positions[i1], descriptor.positions[i0]);
        const Vec3f edge2 = subtract(descriptor.positions[i2], descriptor.positions[i0]);
        const Vec2f delta1{uv_set.values[i1].x - uv_set.values[i0].x,
                           uv_set.values[i1].y - uv_set.values[i0].y};
        const Vec2f delta2{uv_set.values[i2].x - uv_set.values[i0].x,
                           uv_set.values[i2].y - uv_set.values[i0].y};
        const float determinant = delta1.x * delta2.y - delta1.y * delta2.x;
        const bool degenerate = std::abs(determinant) <= 1.0e-20F;
        const float inverse = degenerate ? 1.0F : 1.0F / determinant;
        const Vec3f tangent =
            degenerate
                ? edge1
                : multiply(subtract(multiply(edge1, delta2.y), multiply(edge2, delta1.y)), inverse);
        const Vec3f bitangent =
            degenerate
                ? edge2
                : multiply(subtract(multiply(edge2, delta1.x), multiply(edge1, delta2.x)), inverse);
        for (std::size_t offset = 0; offset < 3; ++offset) {
            const std::uint32_t vertex = descriptor.triangle_indices[triangle + offset];
            result[triangle + offset] =
                generated_corner_tangent(tangent, bitangent, descriptor.normals[vertex]);
        }
    }
    return result;
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

std::string_view tangent_basis_algorithm_name(TangentBasisAlgorithm algorithm) {
    switch (algorithm) {
        case TangentBasisAlgorithm::ctex_uv_derivative:
            return "CyberTexel UV derivative";
        case TangentBasisAlgorithm::lengyel_orthonormalized:
            return "Lengyel orthonormalized UV derivative";
        case TangentBasisAlgorithm::mikktspace:
            return "MikkTSpace";
    }
    throw std::invalid_argument("tangent basis algorithm is invalid");
}

bool tangent_frames_compatible(const TangentFrameDescriptor& left,
                               const TangentFrameDescriptor& right) noexcept {
    return left == right;
}

Vec3f tangent_space_to_object(Vec3f tangent_space_normal, Vec3f surface_normal, Vec4f tangent) {
    const Vec3f normal = normalize(surface_normal, "surface normal");
    const Vec3f tangent_direction = normalize({tangent.x, tangent.y, tangent.z}, "tangent");
    if (!finite(tangent) || std::abs(std::abs(tangent.w) - 1.0F) > 1.0e-5F ||
        std::abs(dot(normal, tangent_direction)) > 1.0e-4F) {
        throw std::invalid_argument("tangent frame is not orthonormal with signed handedness");
    }
    const Vec3f bitangent = multiply(cross(normal, tangent_direction), tangent.w);
    return normalize({tangent_direction.x * tangent_space_normal.x +
                          bitangent.x * tangent_space_normal.y + normal.x * tangent_space_normal.z,
                      tangent_direction.y * tangent_space_normal.x +
                          bitangent.y * tangent_space_normal.y + normal.y * tangent_space_normal.z,
                      tangent_direction.z * tangent_space_normal.x +
                          bitangent.z * tangent_space_normal.y + normal.z * tangent_space_normal.z},
                     "transformed tangent-space normal");
}

MeshView::MeshView(MeshDescriptor descriptor) : descriptor_(descriptor) {
    validate_vertex_attributes(descriptor_);
    validate_indices(descriptor_);
    validate_uv_sets(descriptor_);
    validate_partitions(descriptor_);
    if (descriptor_.corner_tangents.empty()) {
        if (descriptor_.tangent_frame) {
            throw std::invalid_argument("a tangent-frame declaration requires supplied tangents");
        }
        tangent_frame_descriptor_.uv_set = std::string(descriptor_.default_uv_set);
        tangent_frame_source_ = TangentFrameSource::generated;
        corner_tangents_ = generate_tangents(descriptor_, tangent_frame_descriptor_);
    } else {
        if (!descriptor_.tangent_frame) {
            throw std::invalid_argument("supplied tangents require a tangent-frame declaration");
        }
        validate_tangent_frame_descriptor(*descriptor_.tangent_frame, descriptor_);
        validate_supplied_tangents(descriptor_);
        tangent_frame_descriptor_ = *descriptor_.tangent_frame;
        tangent_frame_source_ = TangentFrameSource::supplied;
        corner_tangents_.assign(descriptor_.corner_tangents.begin(),
                                descriptor_.corner_tangents.end());
    }
}

MeshAttributeDescription MeshView::attributes() const noexcept {
    return {
        descriptor_.positions.size(),       triangle_count(),      descriptor_.uv_sets.size(),
        !descriptor_.vertex_colors.empty(), tangent_frame_source_, corner_tangents_.size(),
    };
}

TangentFrameView MeshView::tangent_frames() const noexcept {
    return {.descriptor = tangent_frame_descriptor_,
            .source = tangent_frame_source_,
            .corner_tangents = corner_tangents_};
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
