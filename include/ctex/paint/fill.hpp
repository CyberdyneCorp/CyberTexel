#ifndef CTEX_PAINT_FILL_HPP
#define CTEX_PAINT_FILL_HPP

#include <cstddef>
#include <cstdint>
#include <ctex/paint/brush.hpp>
#include <ctex/paint/surface_cache.hpp>
#include <optional>
#include <span>
#include <string_view>
#include <vector>

namespace ctex::paint {

enum class FillScope : std::uint8_t {
    whole_set,
    triangle,
    connected_by_angle,
    uv_island,
    uv_tile,
    selection,
};

struct FillTriangleTopology {
    std::uint32_t triangle_identity{};
    Vec3d geometric_normal;
    std::vector<std::uint32_t> adjacent_triangles;
};

inline constexpr double default_fill_angle_degrees = 45.0;

struct FillScopeRequest {
    FillScope scope{FillScope::whole_set};
    std::optional<std::size_t> picked_texel;
    double maximum_angle_degrees{default_fill_angle_degrees};
    std::span<const FillTriangleTopology> triangle_topology;
    std::optional<PaintMaskView> selection;
};

struct FillScopeResult {
    std::uint32_t width{};
    std::uint32_t height{};
    FillScope scope{FillScope::whole_set};
    std::vector<double> values;
    std::vector<std::uint32_t> selected_triangle_ids;
    std::size_t selected_texel_count{};
};

[[nodiscard]] FillScopeResult resolve_fill_scope(const CachedSurfaceMaps& surface,
                                                 const FillScopeRequest& request);

struct FillSettings {
    FillScopeRequest scope;
    std::string_view blend_mode{"normal"};
    PaintMaskInputs masks;
    std::optional<PaintMaskView> rejection_acceptance;
};

struct FillResult {
    FillScopeResult resolved_scope;
    std::vector<double> strength;
    std::vector<PaintToolChannelRaster> channels;
    std::vector<std::string> applied_channel_ids;
};

[[nodiscard]] FillResult apply_fill(const CachedSurfaceMaps& surface,
                                    std::span<const PaintToolChannelRaster> enabled_layer_snapshot,
                                    std::span<const PaintToolChannelRaster> material,
                                    const FillSettings& settings = {});

}  // namespace ctex::paint

#endif
