#ifndef CTEX_MESH_MESH_HPP
#define CTEX_MESH_MESH_HPP

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>

namespace ctex::mesh {

struct Vec2f {
    float x;
    float y;
    friend constexpr bool operator==(Vec2f, Vec2f) noexcept = default;
};

struct Vec3f {
    float x;
    float y;
    float z;
    friend constexpr bool operator==(Vec3f, Vec3f) noexcept = default;
};

struct Vec4f {
    float x;
    float y;
    float z;
    float w;
    friend constexpr bool operator==(Vec4f, Vec4f) noexcept = default;
};

struct UvSetView {
    std::string_view name;
    std::span<const Vec2f> values;
};

enum class PartitionKind { material, object, submesh, explicit_faces };

struct MeshPartition {
    PartitionKind kind;
    std::string_view stable_key;
    std::string_view display_name;
};

struct MeshDescriptor {
    std::span<const Vec3f> positions;
    std::span<const Vec3f> normals;
    std::span<const Vec4f> vertex_colors;
    std::span<const std::uint32_t> triangle_indices;
    std::span<const UvSetView> uv_sets;
    std::string_view default_uv_set;
    std::span<const MeshPartition> partitions;
    std::span<const std::uint32_t> face_partition_indices;
    std::span<const std::uint32_t> face_material_ids;
};

struct MeshAttributeDescription {
    std::size_t vertex_count;
    std::size_t triangle_count;
    std::size_t uv_set_count;
    bool has_vertex_colors;
};

class MeshView {
public:
    explicit MeshView(MeshDescriptor descriptor);

    [[nodiscard]] const MeshDescriptor& descriptor() const noexcept { return descriptor_; }
    [[nodiscard]] MeshAttributeDescription attributes() const noexcept;
    [[nodiscard]] std::size_t triangle_count() const noexcept;
    [[nodiscard]] const UvSetView& uv_set(std::string_view name) const;
    [[nodiscard]] const MeshPartition& partition_for_face(std::size_t face_index) const;

private:
    MeshDescriptor descriptor_;
};

using MeshRevision = std::uint64_t;

class MeshBinding {
public:
    explicit MeshBinding(MeshDescriptor descriptor);

    void replace(MeshDescriptor descriptor);
    [[nodiscard]] const MeshView& view() const noexcept { return view_; }
    [[nodiscard]] MeshRevision revision() const noexcept { return revision_; }

private:
    MeshView view_;
    MeshRevision revision_;
};

[[nodiscard]] std::string texture_set_stable_id(PartitionKind kind, std::string_view partition_key,
                                                std::string_view uv_set);

}  // namespace ctex::mesh

#endif
