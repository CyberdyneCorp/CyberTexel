#ifndef CTEX_MESH_MESH_HPP
#define CTEX_MESH_MESH_HPP

#include <cstddef>
#include <cstdint>
#include <memory_resource>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace ctex::mesh {

inline constexpr std::size_t maximum_vertex_count = 100'000'000;
inline constexpr std::size_t maximum_triangle_count = 100'000'000;

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

enum class TangentBasisAlgorithm : std::uint8_t {
    ctex_uv_derivative,
    lengyel_orthonormalized,
    mikktspace,
};
enum class NormalOrientation : std::uint8_t { vertex_normals, inverted_vertex_normals };
enum class CoordinateSystemHandedness : std::uint8_t { right_handed, left_handed };
enum class UvVAxis : std::uint8_t { upward, downward };
enum class TangentHandednessEncoding : std::uint8_t { tangent_w_sign };
enum class TangentFrameSource : std::uint8_t { supplied, generated };

inline constexpr std::uint32_t ctex_uv_derivative_version = 1;

struct TangentFrameDescriptor {
    TangentBasisAlgorithm algorithm{TangentBasisAlgorithm::ctex_uv_derivative};
    std::uint32_t algorithm_version{ctex_uv_derivative_version};
    NormalOrientation normal_orientation{NormalOrientation::vertex_normals};
    CoordinateSystemHandedness coordinate_handedness{CoordinateSystemHandedness::right_handed};
    UvVAxis uv_v_axis{UvVAxis::upward};
    TangentHandednessEncoding handedness_encoding{TangentHandednessEncoding::tangent_w_sign};
    std::pmr::string uv_set;

    friend bool operator==(const TangentFrameDescriptor&, const TangentFrameDescriptor&) = default;
};

struct TangentFrameView {
    TangentFrameDescriptor descriptor;
    TangentFrameSource source{};
    std::span<const Vec4f> corner_tangents;
};

[[nodiscard]] std::string_view tangent_basis_algorithm_name(TangentBasisAlgorithm algorithm);
[[nodiscard]] bool tangent_frames_compatible(const TangentFrameDescriptor& left,
                                             const TangentFrameDescriptor& right) noexcept;
[[nodiscard]] Vec3f tangent_space_to_object(Vec3f tangent_space_normal, Vec3f surface_normal,
                                            Vec4f tangent);

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
    std::span<const Vec4f> corner_tangents{};
    std::optional<TangentFrameDescriptor> tangent_frame{};
};

struct MeshAttributeDescription {
    std::size_t vertex_count;
    std::size_t triangle_count;
    std::size_t uv_set_count;
    bool has_vertex_colors;
    TangentFrameSource tangent_source;
    std::size_t corner_tangent_count;
};

class MeshView {
public:
    explicit MeshView(MeshDescriptor descriptor, std::pmr::memory_resource* memory_resource =
                                                     std::pmr::get_default_resource());

    [[nodiscard]] const MeshDescriptor& descriptor() const noexcept { return descriptor_; }
    [[nodiscard]] MeshAttributeDescription attributes() const noexcept;
    [[nodiscard]] std::size_t triangle_count() const noexcept;
    [[nodiscard]] const UvSetView& uv_set(std::string_view name) const;
    [[nodiscard]] const MeshPartition& partition_for_face(std::size_t face_index) const;
    [[nodiscard]] TangentFrameView tangent_frames() const noexcept;

private:
    MeshDescriptor descriptor_;
    TangentFrameDescriptor tangent_frame_descriptor_;
    TangentFrameSource tangent_frame_source_{};
    std::pmr::vector<Vec4f> corner_tangents_;
};

using MeshRevision = std::uint64_t;

class MeshBinding {
public:
    explicit MeshBinding(MeshDescriptor descriptor, std::pmr::memory_resource* memory_resource =
                                                        std::pmr::get_default_resource());

    void replace(MeshDescriptor descriptor);
    [[nodiscard]] const MeshView& view() const noexcept { return view_; }
    [[nodiscard]] MeshRevision revision() const noexcept { return revision_; }

private:
    std::pmr::memory_resource* memory_resource_;
    MeshView view_;
    MeshRevision revision_;
};

[[nodiscard]] std::string texture_set_stable_id(PartitionKind kind, std::string_view partition_key,
                                                std::string_view uv_set);

}  // namespace ctex::mesh

#endif
