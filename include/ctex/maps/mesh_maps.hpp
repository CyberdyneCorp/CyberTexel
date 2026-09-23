#ifndef CTEX_MAPS_MESH_MAPS_HPP
#define CTEX_MAPS_MESH_MAPS_HPP

#include <array>
#include <cstddef>
#include <cstdint>
#include <ctex/doc/document.hpp>
#include <ctex/image/tiled_image.hpp>
#include <ctex/mesh/mesh.hpp>
#include <map>
#include <memory>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace ctex::maps {

enum class MeshMapKind : std::uint8_t {
    tangent_space_normal,
    object_space_normal,
    world_space_direction,
    ambient_occlusion,
    curvature,
    thickness,
    position,
    height,
    bent_normal,
    material_id,
    object_id,
    uv_density,
    vertex_colour,
};

inline constexpr std::array all_mesh_map_kinds{
    MeshMapKind::tangent_space_normal,
    MeshMapKind::object_space_normal,
    MeshMapKind::world_space_direction,
    MeshMapKind::ambient_occlusion,
    MeshMapKind::curvature,
    MeshMapKind::thickness,
    MeshMapKind::position,
    MeshMapKind::height,
    MeshMapKind::bent_normal,
    MeshMapKind::material_id,
    MeshMapKind::object_id,
    MeshMapKind::uv_density,
    MeshMapKind::vertex_colour,
};

[[nodiscard]] std::string_view mesh_map_name(MeshMapKind kind);

enum class NormalMapConvention : std::uint8_t { open_gl, direct_x };

[[nodiscard]] std::string_view normal_map_convention_name(NormalMapConvention convention);
[[nodiscard]] bool mesh_map_uses_normal_convention(MeshMapKind kind);

struct MeshMapDescriptor {
    MeshMapKind kind{};
    std::string texture_set_id;
    std::string uv_set;
    mesh::MeshRevision mesh_revision{};
    std::optional<NormalMapConvention> normal_convention{};
    std::optional<mesh::TangentFrameDescriptor> tangent_frame{};
    std::shared_ptr<const image::TiledImage> pixels;
};

struct MeshMapStaleness {
    MeshMapKind kind{};
    mesh::MeshRevision produced_mesh_revision{};
    mesh::MeshRevision current_mesh_revision{};

    friend bool operator==(const MeshMapStaleness&, const MeshMapStaleness&) = default;
};

struct MapResolutionMismatch {
    MeshMapKind kind{};
    std::string texture_set_id;
    std::uint32_t map_width{};
    std::uint32_t map_height{};
    std::uint32_t texture_set_width{};
    std::uint32_t texture_set_height{};

    friend bool operator==(const MapResolutionMismatch&, const MapResolutionMismatch&) = default;
};

struct MeshMapBindResult {
    bool replaced_existing{};
    std::optional<MapResolutionMismatch> resolution_mismatch;
    std::optional<MeshMapStaleness> staleness;
};

struct MeshMapSample {
    std::uint8_t component_count{};
    std::array<double, 4> values{};

    friend bool operator==(const MeshMapSample&, const MeshMapSample&) = default;
};

struct MeshMapReadResult {
    MeshMapSample sample;
    std::optional<MeshMapStaleness> staleness;

    friend bool operator==(const MeshMapReadResult&, const MeshMapReadResult&) = default;
};

struct MeshMapMemoryEntry {
    MeshMapKind kind{};
    std::uint32_t width{};
    std::uint32_t height{};
    std::size_t resident_pixel_bytes{};
    friend bool operator==(const MeshMapMemoryEntry&, const MeshMapMemoryEntry&) = default;
};

struct MeshMapMemoryReport {
    std::string texture_set_id;
    std::vector<MeshMapMemoryEntry> maps;
    std::size_t resident_pixel_bytes{};
    friend bool operator==(const MeshMapMemoryReport&, const MeshMapMemoryReport&) = default;
};

struct MeshMapReleaseResult {
    std::vector<MeshMapKind> released_maps;
    std::size_t resident_pixel_bytes_released{};
    friend bool operator==(const MeshMapReleaseResult&, const MeshMapReleaseResult&) = default;
};

struct MeshMapBindingSnapshot {
    std::string texture_set_id;
    std::string uv_set;
    std::vector<MeshMapDescriptor> maps;
};

struct MeshMapRequirementReport {
    std::string consumer;
    std::string texture_set_id;
    std::vector<MeshMapKind> missing_maps;
    std::vector<MeshMapStaleness> stale_maps;
    std::string message;

    [[nodiscard]] bool satisfied() const noexcept { return missing_maps.empty(); }
    friend bool operator==(const MeshMapRequirementReport&,
                           const MeshMapRequirementReport&) = default;
};

class MissingMeshMapsError : public std::out_of_range {
public:
    explicit MissingMeshMapsError(MeshMapRequirementReport report);

    [[nodiscard]] const MeshMapRequirementReport& report() const noexcept { return report_; }

private:
    MeshMapRequirementReport report_;
};

class MeshMapSet {
public:
    MeshMapSet(const doc::TextureSet& texture_set, mesh::MeshRevision mesh_revision,
               std::optional<mesh::TangentFrameDescriptor> tangent_frame = std::nullopt);
    MeshMapSet(const doc::TextureSet& texture_set, const mesh::MeshBinding& mesh);

    [[nodiscard]] const std::string& texture_set_id() const noexcept { return texture_set_id_; }
    [[nodiscard]] const std::string& uv_set() const noexcept { return uv_set_; }
    [[nodiscard]] std::uint32_t texture_set_width() const noexcept { return texture_set_width_; }
    [[nodiscard]] std::uint32_t texture_set_height() const noexcept { return texture_set_height_; }
    [[nodiscard]] mesh::MeshRevision mesh_revision() const noexcept { return mesh_revision_; }
    [[nodiscard]] const std::optional<mesh::TangentFrameDescriptor>& tangent_frame()
        const noexcept {
        return tangent_frame_;
    }

    [[nodiscard]] MeshMapBindResult bind(MeshMapDescriptor descriptor);
    [[nodiscard]] bool contains(MeshMapKind kind) const noexcept;
    [[nodiscard]] const MeshMapDescriptor& map(MeshMapKind kind) const;
    [[nodiscard]] std::vector<MeshMapKind> bound_maps() const;
    [[nodiscard]] std::size_t size() const noexcept { return maps_.size(); }
    [[nodiscard]] MeshMapRequirementReport check_required_maps(
        std::string_view consumer, std::span<const MeshMapKind> required) const;
    [[nodiscard]] MeshMapRequirementReport require_maps(
        std::string_view consumer, std::span<const MeshMapKind> required) const;
    [[nodiscard]] std::vector<MeshMapStaleness> synchronize_mesh_revision(
        mesh::MeshRevision revision);
    [[nodiscard]] std::vector<MeshMapStaleness> synchronize_mesh_revision(
        const mesh::MeshBinding& mesh);
    [[nodiscard]] std::vector<MeshMapStaleness> stale_maps() const;
    [[nodiscard]] MeshMapReadResult sample(MeshMapKind kind, double u, double v) const;

    /// A sampler bound to one map, validated once.
    ///
    /// `sample` resolves the map, revalidates its tangent binding and recomputes
    /// its staleness on every call. That is right for a one-off read and wrong
    /// for a dense sweep, where all three are loop invariants. A consumer that
    /// samples the same map many times binds it once and reads through this.
    ///
    /// The sampler borrows its map: it is valid only while the set still holds
    /// that map unchanged.
    class BoundSampler {
    public:
        [[nodiscard]] MeshMapSample at(double u, double v) const;
        [[nodiscard]] const std::optional<MeshMapStaleness>& staleness() const noexcept {
            return staleness_;
        }

    private:
        friend class MeshMapSet;
        BoundSampler(const MeshMapDescriptor& descriptor, std::optional<MeshMapStaleness> staleness)
            : descriptor_(&descriptor), staleness_(std::move(staleness)) {}

        const MeshMapDescriptor* descriptor_;
        std::optional<MeshMapStaleness> staleness_;
    };

    /// Resolve and validate a map once for repeated sampling.
    [[nodiscard]] BoundSampler bind_sampler(MeshMapKind kind) const;
    [[nodiscard]] MeshMapMemoryReport memory_report() const;
    [[nodiscard]] MeshMapReleaseResult release_map(MeshMapKind kind);
    [[nodiscard]] MeshMapReleaseResult release_all_maps();
    [[nodiscard]] MeshMapBindingSnapshot snapshot_bindings() const;
    void restore_bindings(MeshMapBindingSnapshot snapshot);

private:
    std::string texture_set_id_;
    std::string uv_set_;
    std::uint32_t texture_set_width_{};
    std::uint32_t texture_set_height_{};
    mesh::MeshRevision mesh_revision_{};
    std::optional<mesh::TangentFrameDescriptor> tangent_frame_;
    doc::TextureSetMemoryAccount memory_account_;
    std::map<MeshMapKind, MeshMapDescriptor> maps_;
};

}  // namespace ctex::maps

#endif
