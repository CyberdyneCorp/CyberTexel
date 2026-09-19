#include <ctex/capi.h>
#include <stddef.h>
#include <stdint.h>

static int expect(int condition) { return condition ? 1 : 0; }

static int short_stroke_reports_only_reachable_tiles(void) {
    const ctex_paint_stamp_footprint footprint = {
        .stamp_ordinal = 0,
        .minimum_x = 8190,
        .minimum_y = 8190,
        .maximum_x = 8200,
        .maximum_y = 8200,
    };
    ctex_paint_work_descriptor work;
    ctex_paint_work_info info = {.size = CTEX_PAINT_WORK_INFO_CURRENT_SIZE};
    ctex_paint_tile_coordinate tiles[4] = {{9, 9}, {9, 9}, {9, 9}, {9, 9}};
    size_t count = 0;
    int passed = expect(ctex_paint_work_init(&work) == CTEX_RESULT_SUCCESS) &&
                 expect(work.tile_size == 64 && work.dilation_radius == 2);
    work.canvas_width = 16384;
    work.canvas_height = 16384;
    work.stamp_footprints = &footprint;
    work.stamp_footprint_count = 1;
    if (passed) {
        passed =
            expect(ctex_paint_plan_work(&work, &info, NULL, 0, &count) == CTEX_RESULT_SUCCESS) &&
            expect(count == 4 && info.canvas_tile_count == 65536 && info.footprint_count == 1 &&
                   info.candidate_tile_visits == 4 && info.processed_tile_count == 4 &&
                   info.resolved_dilation_radius == 2 && info.dilation_radius_clamped == 0);
    }
    info.candidate_tile_visits = 99;
    if (passed) {
        passed = expect(ctex_paint_plan_work(&work, &info, tiles, 3, &count) ==
                        CTEX_RESULT_BUFFER_TOO_SMALL) &&
                 expect(count == 4 && info.candidate_tile_visits == 99 && tiles[0].x == 9 &&
                        tiles[0].y == 9);
    }
    if (passed) {
        passed =
            expect(ctex_paint_plan_work(&work, &info, tiles, 4, &count) == CTEX_RESULT_SUCCESS) &&
            expect(tiles[0].x == 127 && tiles[0].y == 127 && tiles[1].x == 128 &&
                   tiles[1].y == 127 && tiles[2].x == 127 && tiles[2].y == 128 &&
                   tiles[3].x == 128 && tiles[3].y == 128);
    }
    return passed;
}

static int dilation_clamping_is_reported(void) {
    const ctex_paint_stamp_footprint footprint = {
        .stamp_ordinal = 3,
        .minimum_x = 4,
        .minimum_y = 4,
        .maximum_x = 5,
        .maximum_y = 5,
    };
    ctex_paint_work_descriptor work;
    ctex_paint_work_info info = {.size = CTEX_PAINT_WORK_INFO_CURRENT_SIZE};
    size_t count = 0;
    int passed = expect(ctex_paint_work_init(&work) == CTEX_RESULT_SUCCESS);
    work.canvas_width = 8;
    work.canvas_height = 8;
    work.tile_size = 4;
    work.dilation_radius = UINT32_MAX;
    work.stamp_footprints = &footprint;
    work.stamp_footprint_count = 1;
    if (passed) {
        passed =
            expect(ctex_paint_plan_work(&work, &info, NULL, 0, &count) == CTEX_RESULT_SUCCESS) &&
            expect(count == 4 && info.processed_tile_count == 4 &&
                   info.resolved_dilation_radius < UINT32_MAX && info.dilation_radius_clamped == 1);
    }
    return passed;
}

static int invalid_work_is_transactional(void) {
    const ctex_paint_stamp_footprint invalid = {
        .stamp_ordinal = 0,
        .minimum_x = 4,
        .minimum_y = 0,
        .maximum_x = 4,
        .maximum_y = 1,
    };
    ctex_paint_work_descriptor work;
    ctex_paint_work_info info = {
        .size = CTEX_PAINT_WORK_INFO_CURRENT_SIZE,
        .candidate_tile_visits = 9,
    };
    ctex_paint_tile_coordinate tile = {9, 9};
    size_t count = 9;
    int passed = expect(ctex_paint_work_init(&work) == CTEX_RESULT_SUCCESS);
    work.canvas_width = 8;
    work.canvas_height = 8;
    work.tile_size = 4;
    work.stamp_footprints = &invalid;
    work.stamp_footprint_count = 1;
    if (passed) {
        passed =
            expect(ctex_paint_plan_work(&work, &info, &tile, 1, &count) ==
                   CTEX_RESULT_INVALID_ARGUMENT) &&
            expect(ctex_get_last_diagnostic_code() == CTEX_DIAGNOSTIC_INVALID_PAINT_WORK) &&
            expect(count == 9 && info.candidate_tile_visits == 9 && tile.x == 9 && tile.y == 9);
    }
    return passed;
}

int main(void) {
    return short_stroke_reports_only_reachable_tiles() && dilation_clamping_is_reported() &&
                   invalid_work_is_transactional()
               ? 0
               : 1;
}
