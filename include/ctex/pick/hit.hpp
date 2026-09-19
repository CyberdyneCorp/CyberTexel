#ifndef CTEX_PICK_HIT_HPP
#define CTEX_PICK_HIT_HPP

#include <cstdint>
#include <ctex/mesh/mesh.hpp>
#include <ctex/pick/ray.hpp>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace ctex::pick {

class SpatialIndex;
class UvSpatialIndex;

struct TextureSetBindingView {
    std::uint32_t partition_index;
    std::string_view uv_set;
};

struct UdimTile {
    std::int32_t u;
    std::int32_t v;
    std::int64_t number;
};

struct HitRecord {
    mesh::Vec3f position;
    mesh::Vec3f interpolated_normal;
    mesh::Vec3f geometric_normal;
    mesh::Vec2f uv;
    std::string texture_set_id;
    UdimTile udim_tile;
    std::uint32_t triangle_index;
    mesh::Vec3f barycentric;
    std::uint32_t material_id;
    float distance;
};

struct UvHitRecord {
    mesh::Vec3f position;
    mesh::Vec3f interpolated_normal;
    mesh::Vec3f geometric_normal;
    mesh::Vec2f uv;
    std::string texture_set_id;
    UdimTile udim_tile;
    std::uint32_t triangle_index;
    mesh::Vec3f barycentric;
    std::uint32_t material_id;
};

enum class OcclusionPolicy { nearest, all_hits };
enum class BackfacePolicy { accept, reject };

struct RayPickOptions {
    float maximum_distance;
    OcclusionPolicy occlusion{OcclusionPolicy::nearest};
    BackfacePolicy backfaces{BackfacePolicy::accept};
};

[[nodiscard]] std::vector<HitRecord> pick_ray(SpatialIndex& index, const mesh::MeshBinding& mesh,
                                              Ray ray, RayPickOptions options,
                                              std::span<const TextureSetBindingView> texture_sets);

[[nodiscard]] std::optional<HitRecord> pick_nearest(
    SpatialIndex& index, const mesh::MeshBinding& mesh, Ray ray, float maximum_distance,
    std::span<const TextureSetBindingView> texture_sets,
    BackfacePolicy backfaces = BackfacePolicy::accept);

[[nodiscard]] std::optional<UvHitRecord> pick_uv(UvSpatialIndex& index,
                                                 const mesh::MeshBinding& mesh,
                                                 mesh::Vec2f coordinate,
                                                 TextureSetBindingView texture_set);

[[nodiscard]] std::optional<HitRecord> snap_to_surface(
    SpatialIndex& index, const mesh::MeshBinding& mesh, mesh::Vec3f point, float maximum_distance,
    std::span<const TextureSetBindingView> texture_sets);

}  // namespace ctex::pick

#endif
