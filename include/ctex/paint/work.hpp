#ifndef CTEX_PAINT_WORK_HPP
#define CTEX_PAINT_WORK_HPP

#include <cstddef>
#include <cstdint>
#include <ctex/doc/channels.hpp>
#include <ctex/paint/seam_dilation.hpp>
#include <functional>
#include <span>
#include <vector>

namespace ctex::paint {

struct StampTexelFootprint {
    std::uint64_t stamp_ordinal{};
    std::uint32_t minimum_x{};
    std::uint32_t minimum_y{};
    std::uint32_t maximum_x{};
    std::uint32_t maximum_y{};
};

struct PaintWorkRequest {
    std::uint32_t canvas_width{};
    std::uint32_t canvas_height{};
    std::uint32_t tile_size{image::default_tile_size};
    std::uint32_t dilation_radius{default_seam_dilation_radius};
    std::span<const StampTexelFootprint> stamp_footprints;
};

struct PaintWorkReport {
    std::uint64_t canvas_tile_count{};
    std::size_t footprint_count{};
    std::size_t candidate_tile_visits{};
    std::vector<image::TileCoordinate> processed_tiles;
    std::uint32_t dilation_radius{};
    ToolParameterReport parameter_report;
};

using PaintTileProcessor = std::function<void(image::TileCoordinate)>;

[[nodiscard]] PaintWorkReport plan_paint_work(const PaintWorkRequest& request);
[[nodiscard]] PaintWorkReport process_paint_work(const PaintWorkRequest& request,
                                                 const PaintTileProcessor& processor);

}  // namespace ctex::paint

#endif
