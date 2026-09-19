#include <ctex/capi.h>
#include <stddef.h>
#include <stdint.h>

static int expect(int condition) { return condition ? 1 : 0; }

static ctex_paint_dilation_tile_descriptor tile_descriptor(int32_t u, const double* pixels) {
    static const uint8_t coverage[5] = {0, 1, 1, 1, 0};
    const ctex_paint_dilation_tile_descriptor tile = {
        .size = CTEX_PAINT_DILATION_TILE_DESCRIPTOR_CURRENT_SIZE,
        .u = u,
        .v = 0,
        .width = 5,
        .height = 1,
        .component_count = 1,
        .pixels = pixels,
        .pixel_count = 5,
        .coverage = coverage,
        .coverage_count = 5,
    };
    return tile;
}

static int stroke_tiles_are_deferred_until_idempotent_finish(void) {
    const double old_first[5] = {-1.0, 1.0, 2.0, 3.0, -1.0};
    const double latest_first[5] = {-1.0, 10.0, 11.0, 12.0, -1.0};
    const double second[5] = {-1.0, 4.0, 5.0, 6.0, -1.0};
    ctex_paint_dilation_session* session = NULL;
    ctex_paint_dilation_session_info info = {
        .size = CTEX_PAINT_DILATION_SESSION_INFO_CURRENT_SIZE,
    };
    ctex_paint_dilation_tile_info tiles[2] = {{0}};
    double pixels[10] = {0.0};
    size_t tile_count = 0;
    size_t pixel_count = 0;
    int passed = expect(ctex_paint_dilation_session_create(2, &session) == CTEX_RESULT_SUCCESS) &&
                 expect(session != NULL);
    ctex_paint_dilation_tile_descriptor tile = tile_descriptor(1, second);
    if (passed) {
        passed =
            expect(ctex_paint_dilation_session_stage_tile(session, &tile) == CTEX_RESULT_SUCCESS);
    }
    tile = tile_descriptor(0, old_first);
    if (passed) {
        passed =
            expect(ctex_paint_dilation_session_stage_tile(session, &tile) == CTEX_RESULT_SUCCESS);
    }
    tile = tile_descriptor(0, latest_first);
    if (passed) {
        passed =
            expect(ctex_paint_dilation_session_stage_tile(session, &tile) == CTEX_RESULT_SUCCESS);
    }
    if (passed) {
        passed =
            expect(ctex_paint_dilation_session_get_preview(session, &info, NULL, 0, &tile_count,
                                                           NULL, 0,
                                                           &pixel_count) == CTEX_RESULT_SUCCESS) &&
            expect(info.state == CTEX_PAINT_DILATION_PROVISIONAL && tile_count == 2 &&
                   pixel_count == 10 && info.dilation_pass_count == 0 && info.resolved_radius == 2);
    }
    if (passed) {
        passed = expect(ctex_paint_dilation_session_get_preview(
                            session, &info, tiles, 2, &tile_count, pixels, 10, &pixel_count) ==
                        CTEX_RESULT_SUCCESS) &&
                 expect(tiles[0].u == 0 && tiles[0].pixel_offset == 0 && tiles[1].u == 1 &&
                        tiles[1].pixel_offset == 5 && pixels[0] == -1.0 && pixels[1] == 10.0 &&
                        pixels[5] == -1.0 && pixels[6] == 4.0);
    }
    if (passed) {
        passed =
            expect(ctex_paint_dilation_session_finish(session, &info, NULL, 0, &tile_count, NULL, 0,
                                                      &pixel_count) == CTEX_RESULT_SUCCESS) &&
            expect(info.state == CTEX_PAINT_DILATION_FINAL && info.dilation_pass_count == 2);
    }
    if (passed) {
        passed =
            expect(ctex_paint_dilation_session_finish(session, &info, tiles, 2, &tile_count, pixels,
                                                      10, &pixel_count) == CTEX_RESULT_SUCCESS) &&
            expect(info.dilation_pass_count == 2 && tiles[0].dilated_texel_count == 2 &&
                   tiles[1].dilated_texel_count == 2 && pixels[0] == 9.0 && pixels[4] == 13.0 &&
                   pixels[5] == 3.0 && pixels[9] == 7.0);
    }
    if (passed) {
        passed = expect(ctex_paint_dilation_session_stage_tile(session, &tile) ==
                        CTEX_RESULT_INVALID_ARGUMENT) &&
                 expect(ctex_get_last_diagnostic_code() == CTEX_DIAGNOSTIC_INVALID_PAINT_DILATION);
    }
    ctex_paint_dilation_session_destroy(session);
    return passed;
}

static int undersized_finish_does_not_finalize(void) {
    const double source[5] = {-1.0, 1.0, 2.0, 3.0, -1.0};
    ctex_paint_dilation_session* session = NULL;
    ctex_paint_dilation_session_info info = {
        .size = CTEX_PAINT_DILATION_SESSION_INFO_CURRENT_SIZE,
        .state = 9,
    };
    ctex_paint_dilation_tile_info tile_info = {.u = 9};
    double pixels[5] = {9.0, 9.0, 9.0, 9.0, 9.0};
    size_t tile_count = 0;
    size_t pixel_count = 0;
    int passed = expect(ctex_paint_dilation_session_create(0, &session) == CTEX_RESULT_SUCCESS);
    const ctex_paint_dilation_tile_descriptor tile = tile_descriptor(0, source);
    if (passed) {
        passed =
            expect(ctex_paint_dilation_session_stage_tile(session, &tile) == CTEX_RESULT_SUCCESS);
    }
    if (passed) {
        passed = expect(ctex_paint_dilation_session_finish(session, &info, &tile_info, 1,
                                                           &tile_count, pixels, 4, &pixel_count) ==
                        CTEX_RESULT_BUFFER_TOO_SMALL) &&
                 expect(info.state == 9 && tile_info.u == 9 && pixels[0] == 9.0);
    }
    if (passed) {
        passed = expect(ctex_paint_dilation_session_get_preview(
                            session, &info, &tile_info, 1, &tile_count, pixels, 5, &pixel_count) ==
                        CTEX_RESULT_SUCCESS) &&
                 expect(info.state == CTEX_PAINT_DILATION_PROVISIONAL);
    }
    if (passed) {
        passed = expect(ctex_paint_dilation_session_finish(session, &info, &tile_info, 1,
                                                           &tile_count, pixels, 5,
                                                           &pixel_count) == CTEX_RESULT_SUCCESS) &&
                 expect(info.state == CTEX_PAINT_DILATION_FINAL && info.dilation_pass_count == 0 &&
                        pixels[0] == -1.0 && pixels[4] == -1.0);
    }
    ctex_paint_dilation_session_destroy(session);
    return passed;
}

int main(void) {
    return stroke_tiles_are_deferred_until_idempotent_finish() &&
                   undersized_finish_does_not_finalize()
               ? 0
               : 1;
}
