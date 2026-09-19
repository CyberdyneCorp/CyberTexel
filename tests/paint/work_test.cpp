#include <array>
#include <cstdint>
#include <ctex/paint/work.hpp>
#include <iostream>
#include <limits>
#include <string_view>
#include <vector>

namespace {

using namespace ctex;

bool expect(bool condition, std::string_view message) {
    if (!condition) {
        std::cerr << message << '\n';
    }
    return condition;
}

bool short_stroke_on_16k_canvas_visits_only_its_reachable_tiles() {
    const std::array footprints{paint::StampTexelFootprint{
        .stamp_ordinal = 0,
        .minimum_x = 8190,
        .minimum_y = 8190,
        .maximum_x = 8200,
        .maximum_y = 8200,
    }};
    std::vector<image::TileCoordinate> visited;
    const paint::PaintWorkReport report =
        paint::process_paint_work({.canvas_width = 16384,
                                   .canvas_height = 16384,
                                   .tile_size = 64,
                                   .dilation_radius = 2,
                                   .stamp_footprints = footprints},
                                  [&](image::TileCoordinate tile) { visited.push_back(tile); });
    const std::vector expected{image::TileCoordinate{127, 127}, image::TileCoordinate{128, 127},
                               image::TileCoordinate{127, 128}, image::TileCoordinate{128, 128}};
    return expect(report.canvas_tile_count == 65536 && report.footprint_count == 1,
                  "16K work report has the wrong canvas or footprint count") &&
           expect(report.processed_tiles == expected && visited == expected &&
                      report.candidate_tile_visits == expected.size(),
                  "short stroke visited tiles outside its footprint and dilation reach");
}

bool dilation_expansion_and_canvas_clipping_are_exact() {
    const std::array edge_footprint{paint::StampTexelFootprint{
        .stamp_ordinal = 3, .minimum_x = 0, .minimum_y = 0, .maximum_x = 1, .maximum_y = 1}};
    const auto no_dilation = paint::plan_paint_work({.canvas_width = 130,
                                                     .canvas_height = 130,
                                                     .tile_size = 64,
                                                     .dilation_radius = 0,
                                                     .stamp_footprints = edge_footprint});
    const auto large_dilation = paint::plan_paint_work({.canvas_width = 130,
                                                        .canvas_height = 130,
                                                        .tile_size = 64,
                                                        .dilation_radius = 128,
                                                        .stamp_footprints = edge_footprint});
    const std::vector expected_large{
        image::TileCoordinate{0, 0}, image::TileCoordinate{1, 0}, image::TileCoordinate{2, 0},
        image::TileCoordinate{0, 1}, image::TileCoordinate{1, 1}, image::TileCoordinate{2, 1},
        image::TileCoordinate{0, 2}, image::TileCoordinate{1, 2}, image::TileCoordinate{2, 2}};
    return expect(no_dilation.processed_tiles == std::vector<image::TileCoordinate>{{0, 0}},
                  "zero dilation expanded the stamp footprint") &&
           expect(large_dilation.processed_tiles == expected_large,
                  "dilation did not expand and clip work at the canvas edge");
}

bool overlapping_frame_footprints_are_deduplicated_and_sorted() {
    const std::array footprints{
        paint::StampTexelFootprint{.stamp_ordinal = 2,
                                   .minimum_x = 128,
                                   .minimum_y = 0,
                                   .maximum_x = 192,
                                   .maximum_y = 64},
        paint::StampTexelFootprint{
            .stamp_ordinal = 0, .minimum_x = 0, .minimum_y = 64, .maximum_x = 64, .maximum_y = 128},
        paint::StampTexelFootprint{.stamp_ordinal = 1,
                                   .minimum_x = 128,
                                   .minimum_y = 0,
                                   .maximum_x = 192,
                                   .maximum_y = 64},
    };
    const auto report = paint::plan_paint_work({.canvas_width = 256,
                                                .canvas_height = 256,
                                                .tile_size = 64,
                                                .dilation_radius = 0,
                                                .stamp_footprints = footprints});
    const std::vector expected{image::TileCoordinate{2, 0}, image::TileCoordinate{0, 1}};
    return expect(report.processed_tiles == expected && report.footprint_count == 3 &&
                      report.candidate_tile_visits == 3,
                  "batched footprints were not deduplicated in row-major order");
}

bool huge_canvas_metadata_does_not_cause_a_canvas_scan() {
    const std::array footprint{paint::StampTexelFootprint{
        .stamp_ordinal = 0, .minimum_x = 1, .minimum_y = 1, .maximum_x = 2, .maximum_y = 2}};
    const auto report = paint::plan_paint_work({
        .canvas_width = std::numeric_limits<std::uint32_t>::max(),
        .canvas_height = std::numeric_limits<std::uint32_t>::max(),
        .tile_size = 64,
        .dilation_radius = 2,
        .stamp_footprints = footprint,
    });
    const std::uint64_t tile_axis = 67108864;
    return expect(report.canvas_tile_count == tile_axis * tile_axis &&
                      report.processed_tiles == std::vector<image::TileCoordinate>{{0, 0}} &&
                      report.candidate_tile_visits == 1,
                  "work planning scaled with huge canvas metadata instead of the footprint");
}

bool dilation_radius_is_bounded_and_reported_by_work_planning() {
    const std::array footprints{paint::StampTexelFootprint{
        .stamp_ordinal = 0, .minimum_x = 4, .minimum_y = 4, .maximum_x = 5, .maximum_y = 5}};
    const std::uint32_t supplied = paint::maximum_seam_dilation_radius + 1;
    const paint::PaintWorkReport report = paint::plan_paint_work({.canvas_width = 8,
                                                                  .canvas_height = 8,
                                                                  .tile_size = 4,
                                                                  .dilation_radius = supplied,
                                                                  .stamp_footprints = footprints});
    return expect(report.dilation_radius == paint::maximum_seam_dilation_radius &&
                      report.parameter_report.clamp_for("seam_dilation.radius") ==
                          paint::ToolParameterClamp{"seam_dilation.radius", supplied,
                                                    paint::maximum_seam_dilation_radius} &&
                      report.processed_tiles.size() == 4,
                  "paint work planning bypassed shared dilation-radius resolution");
}

bool invalid_requests_are_refused_before_processing() {
    const std::array invalid{paint::StampTexelFootprint{
        .stamp_ordinal = 0, .minimum_x = 4, .minimum_y = 0, .maximum_x = 4, .maximum_y = 1}};
    std::size_t calls = 0;
    bool footprint_refused = false;
    try {
        static_cast<void>(paint::process_paint_work({.canvas_width = 8,
                                                     .canvas_height = 8,
                                                     .tile_size = 4,
                                                     .dilation_radius = 0,
                                                     .stamp_footprints = invalid},
                                                    [&](image::TileCoordinate) { ++calls; }));
    } catch (const std::invalid_argument&) {
        footprint_refused = true;
    }
    bool processor_refused = false;
    try {
        static_cast<void>(
            paint::process_paint_work({.canvas_width = 8,
                                       .canvas_height = 8,
                                       .tile_size = 4,
                                       .dilation_radius = paint::default_seam_dilation_radius,
                                       .stamp_footprints = {}},
                                      {}));
    } catch (const std::invalid_argument&) {
        processor_refused = true;
    }
    return expect(footprint_refused && processor_refused && calls == 0,
                  "invalid bounded-work input invoked the tile processor");
}

}  // namespace

int main() {
    return short_stroke_on_16k_canvas_visits_only_its_reachable_tiles() &&
                   dilation_expansion_and_canvas_clipping_are_exact() &&
                   overlapping_frame_footprints_are_deduplicated_and_sorted() &&
                   huge_canvas_metadata_does_not_cause_a_canvas_scan() &&
                   dilation_radius_is_bounded_and_reported_by_work_planning() &&
                   invalid_requests_are_refused_before_processing()
               ? 0
               : 1;
}
