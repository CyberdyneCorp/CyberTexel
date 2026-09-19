#include <algorithm>
#include <ctex/paint/work.hpp>
#include <limits>
#include <set>
#include <stdexcept>

namespace ctex::paint {
namespace {

std::uint32_t divide_rounding_up(std::uint32_t value, std::uint32_t divisor) {
    return 1 + ((value - 1) / divisor);
}

void validate_request(const PaintWorkRequest& request) {
    if (request.canvas_width == 0 || request.canvas_height == 0) {
        throw std::invalid_argument("paint work canvas dimensions must be non-zero");
    }
    if (request.tile_size == 0) {
        throw std::invalid_argument("paint work tile size must be non-zero");
    }
    for (const StampTexelFootprint& footprint : request.stamp_footprints) {
        if (footprint.minimum_x >= footprint.maximum_x ||
            footprint.minimum_y >= footprint.maximum_y ||
            footprint.maximum_x > request.canvas_width ||
            footprint.maximum_y > request.canvas_height) {
            throw std::invalid_argument(
                "stamp texel footprints must be non-empty and inside the canvas");
        }
    }
}

std::uint64_t tile_key(std::uint32_t x, std::uint32_t y) {
    return (static_cast<std::uint64_t>(y) << 32U) | x;
}

image::TileCoordinate tile_coordinate(std::uint64_t key) {
    return {.x = static_cast<std::uint32_t>(key), .y = static_cast<std::uint32_t>(key >> 32U)};
}

struct ExpandedFootprint {
    std::uint32_t minimum_x;
    std::uint32_t minimum_y;
    std::uint32_t maximum_x;
    std::uint32_t maximum_y;
};

ExpandedFootprint expand(const StampTexelFootprint& footprint, const PaintWorkRequest& request) {
    const auto expanded_maximum = [](std::uint32_t value, std::uint32_t radius,
                                     std::uint32_t limit) {
        return static_cast<std::uint32_t>(
            std::min<std::uint64_t>(limit, static_cast<std::uint64_t>(value) + radius));
    };
    return {
        .minimum_x = footprint.minimum_x > request.dilation_radius
                         ? footprint.minimum_x - request.dilation_radius
                         : 0,
        .minimum_y = footprint.minimum_y > request.dilation_radius
                         ? footprint.minimum_y - request.dilation_radius
                         : 0,
        .maximum_x =
            expanded_maximum(footprint.maximum_x, request.dilation_radius, request.canvas_width),
        .maximum_y =
            expanded_maximum(footprint.maximum_y, request.dilation_radius, request.canvas_height),
    };
}

void add_tiles(const ExpandedFootprint& footprint, std::uint32_t tile_size,
               std::set<std::uint64_t>& tiles, std::size_t& candidate_visits) {
    const std::uint32_t minimum_tile_x = footprint.minimum_x / tile_size;
    const std::uint32_t minimum_tile_y = footprint.minimum_y / tile_size;
    const std::uint32_t maximum_tile_x = (footprint.maximum_x - 1) / tile_size;
    const std::uint32_t maximum_tile_y = (footprint.maximum_y - 1) / tile_size;
    const std::uint64_t columns = static_cast<std::uint64_t>(maximum_tile_x) - minimum_tile_x + 1;
    const std::uint64_t rows = static_cast<std::uint64_t>(maximum_tile_y) - minimum_tile_y + 1;
    const std::uint64_t visits = columns * rows;
    if (visits > std::numeric_limits<std::size_t>::max() - candidate_visits) {
        throw std::length_error("paint work candidate tile count exceeds address space");
    }
    candidate_visits += static_cast<std::size_t>(visits);
    for (std::uint32_t y = minimum_tile_y;; ++y) {
        for (std::uint32_t x = minimum_tile_x;; ++x) {
            tiles.insert(tile_key(x, y));
            if (x == maximum_tile_x) {
                break;
            }
        }
        if (y == maximum_tile_y) {
            break;
        }
    }
}

}  // namespace

PaintWorkReport plan_paint_work(const PaintWorkRequest& request) {
    validate_request(request);
    const std::uint32_t tile_columns = divide_rounding_up(request.canvas_width, request.tile_size);
    const std::uint32_t tile_rows = divide_rounding_up(request.canvas_height, request.tile_size);
    PaintWorkReport report{
        .canvas_tile_count = static_cast<std::uint64_t>(tile_columns) * tile_rows,
        .footprint_count = request.stamp_footprints.size(),
        .candidate_tile_visits = 0,
        .processed_tiles = {}};
    std::set<std::uint64_t> tiles;
    for (const StampTexelFootprint& footprint : request.stamp_footprints) {
        add_tiles(expand(footprint, request), request.tile_size, tiles,
                  report.candidate_tile_visits);
    }
    report.processed_tiles.reserve(tiles.size());
    std::transform(tiles.begin(), tiles.end(), std::back_inserter(report.processed_tiles),
                   tile_coordinate);
    return report;
}

PaintWorkReport process_paint_work(const PaintWorkRequest& request,
                                   const PaintTileProcessor& processor) {
    if (!processor) {
        throw std::invalid_argument("paint tile processor must be callable");
    }
    PaintWorkReport report = plan_paint_work(request);
    for (const image::TileCoordinate tile : report.processed_tiles) {
        processor(tile);
    }
    return report;
}

}  // namespace ctex::paint
