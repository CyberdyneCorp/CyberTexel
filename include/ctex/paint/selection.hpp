#ifndef CTEX_PAINT_SELECTION_HPP
#define CTEX_PAINT_SELECTION_HPP

#include <cstddef>
#include <cstdint>
#include <ctex/paint/fill.hpp>
#include <ctex/pick/region.hpp>
#include <ctex/pick/spatial_index.hpp>
#include <span>
#include <vector>

namespace ctex::paint {

enum class SelectionKind : std::uint8_t {
    screen_rectangle,
    screen_lasso,
    polygon_triangle,
    polygon_uv_island,
    polygon_connected_by_angle,
};

struct SelectionResult {
    std::uint32_t width{};
    std::uint32_t height{};
    SelectionKind kind{SelectionKind::screen_rectangle};
    std::vector<double> values;
    std::vector<std::uint32_t> selected_triangle_ids;
    std::size_t selected_texel_count{};
    std::size_t visited_nodes{};
    std::size_t tested_leaf_triangles{};

    [[nodiscard]] PaintMaskView as_paint_restriction() const& noexcept { return {values}; }
    PaintMaskView as_paint_restriction() const&& = delete;
};

enum class PolygonSelectionMode : std::uint8_t { triangle, uv_island, connected_by_angle };

struct PolygonSelectionRequest {
    PolygonSelectionMode mode{PolygonSelectionMode::triangle};
    std::size_t picked_texel{};
    double maximum_angle_degrees{default_fill_angle_degrees};
    std::span<const FillTriangleTopology> triangle_topology;
};

[[nodiscard]] SelectionResult select_screen_rectangle(pick::SpatialIndex& index,
                                                      const mesh::MeshBinding& mesh,
                                                      const CachedSurfaceMaps& surface,
                                                      pick::ScreenRectangle rectangle,
                                                      const pick::ScreenRegionView& view);

[[nodiscard]] SelectionResult select_screen_lasso(pick::SpatialIndex& index,
                                                  const mesh::MeshBinding& mesh,
                                                  const CachedSurfaceMaps& surface,
                                                  std::span<const pick::ScreenPosition> points,
                                                  const pick::ScreenRegionView& view);

[[nodiscard]] SelectionResult select_polygon(const CachedSurfaceMaps& surface,
                                             const PolygonSelectionRequest& request);

struct StoredSelectionMask {
    std::uint32_t width{};
    std::uint32_t height{};
    std::vector<double> values;

    [[nodiscard]] PaintMaskView view() const& noexcept { return {values}; }
    PaintMaskView view() const&& = delete;
};

[[nodiscard]] StoredSelectionMask store_selection_mask(const SelectionResult& selection);

}  // namespace ctex::paint

#endif
