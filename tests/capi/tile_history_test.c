#include <ctex/capi.h>
#include <stdint.h>
#include <stdio.h>

#define CANVAS_SIZE 64U
#define PIXEL_COUNT ((size_t)CANVAS_SIZE * CANVAS_SIZE)
#define TILE_BYTES (PIXEL_COUNT * 3U)

static int expect(int condition, const char* message) {
    if (!condition) {
        fprintf(stderr, "%s\n", message);
    }
    return condition;
}

static int create_fixture(ctex_document** document, char* set_id, size_t set_id_size) {
    const ctex_texture_set_descriptor descriptor = {
        .size = CTEX_TEXTURE_SET_DESCRIPTOR_CURRENT_SIZE,
        .display_name = "History",
        .partition_kind = CTEX_PARTITION_SOURCE_MATERIAL,
        .partition_key = "history",
        .uv_set = "uv0",
        .width = CANVAS_SIZE,
        .height = CANVAS_SIZE,
        .default_bit_depth = 8,
    };
    size_t required_size = 0;
    size_t count = 0;
    return ctex_document_create(document) == CTEX_RESULT_SUCCESS &&
           ctex_document_create_texture_set(*document, &descriptor) == CTEX_RESULT_SUCCESS &&
           ctex_document_get_texture_set_ids(*document, set_id, set_id_size, &required_size,
                                             &count) == CTEX_RESULT_SUCCESS &&
           count == 1 &&
           ctex_texture_set_set_channel_enabled(*document, set_id, "pbr.base_color", 1, 0) ==
               CTEX_RESULT_SUCCESS;
}

static int commit_pixel(ctex_document* document, const char* set_id, const uint8_t value[3]) {
    ctex_paint_preview_session* preview = NULL;
    ctex_paint_preview_info info = {.size = CTEX_PAINT_PREVIEW_INFO_CURRENT_SIZE};
    static const uint8_t coverage[PIXEL_COUNT] = {1};
    const int passed =
        ctex_paint_preview_session_create(document, set_id, "pbr.base_color", &preview) ==
            CTEX_RESULT_SUCCESS &&
        ctex_paint_preview_session_write_pixel(preview, 0, 0, value, 3) == CTEX_RESULT_SUCCESS &&
        ctex_paint_preview_session_finalize(preview, coverage, PIXEL_COUNT, 0, &info) ==
            CTEX_RESULT_SUCCESS &&
        ctex_paint_preview_session_commit(preview, &info) == CTEX_RESULT_SUCCESS;
    ctex_paint_preview_session_destroy(preview);
    return passed;
}

static int first_pixel_is(ctex_document* document, const char* set_id, const uint8_t value[3]) {
    ctex_paint_preview_session* preview = NULL;
    static uint8_t pixels[TILE_BYTES];
    size_t required_size = 0;
    const int passed =
        ctex_paint_preview_session_create(document, set_id, "pbr.base_color", &preview) ==
            CTEX_RESULT_SUCCESS &&
        ctex_paint_preview_session_get_pixels(preview, pixels, sizeof(pixels), &required_size) ==
            CTEX_RESULT_SUCCESS &&
        required_size == sizeof(pixels) && pixels[0] == value[0] && pixels[1] == value[1] &&
        pixels[2] == value[2];
    ctex_paint_preview_session_destroy(preview);
    return passed;
}

static int begin_capture(ctex_document* document, const char* set_id, const char* step,
                         ctex_tile_history_capture** capture) {
    const ctex_tile_history_target_descriptor target = {
        .size = CTEX_TILE_HISTORY_TARGET_DESCRIPTOR_CURRENT_SIZE,
        .semantic_id = "pbr.base_color",
        .tile_x = 0,
        .tile_y = 0,
    };
    return ctex_texture_set_begin_tile_history(document, set_id, step, &target, 1, capture) ==
           CTEX_RESULT_SUCCESS;
}

static int tile_history_is_budgeted_symmetric_and_invalidates_redo(void) {
    ctex_document* document = NULL;
    ctex_tile_history_capture* capture = NULL;
    char set_id[128] = {0};
    const uint8_t channel_default[3] = {128, 128, 128};
    const uint8_t first[3] = {10, 20, 30};
    const uint8_t second[3] = {40, 50, 60};
    ctex_tile_history_commit_info commit = {.size = CTEX_TILE_HISTORY_COMMIT_INFO_CURRENT_SIZE};
    ctex_tile_history_restore_info restore = {.size = CTEX_TILE_HISTORY_RESTORE_INFO_CURRENT_SIZE};
    ctex_tile_history_budget_report budget = {.size = CTEX_TILE_HISTORY_BUDGET_REPORT_CURRENT_SIZE};

    int passed =
        expect(create_fixture(&document, set_id, sizeof(set_id)),
               "tile history fixture creation failed") &&
        expect(ctex_texture_set_configure_tile_history(document, set_id, TILE_BYTES - 1) ==
                   CTEX_RESULT_SUCCESS,
               "small history budget configuration failed") &&
        expect(!begin_capture(document, set_id, "too-large", &capture) &&
                   ctex_get_last_result() == CTEX_RESULT_OVER_BUDGET && capture == NULL,
               "over-budget capture was admitted") &&
        expect(ctex_texture_set_configure_tile_history(document, set_id, TILE_BYTES) ==
                   CTEX_RESULT_SUCCESS,
               "history budget configuration failed") &&
        expect(begin_capture(document, set_id, "first", &capture) &&
                   commit_pixel(document, set_id, first) &&
                   ctex_tile_history_capture_commit(capture, &commit) == CTEX_RESULT_SUCCESS &&
                   commit.committed == 1 && commit.tile_count == 1 &&
                   commit.retained_bytes == TILE_BYTES,
               "tile history capture did not retain one changed tile");
    ctex_tile_history_capture_destroy(capture);
    capture = NULL;

    const ctex_result undo_result = ctex_texture_set_undo_tiles(document, set_id, &restore);
    const int default_restored = first_pixel_is(document, set_id, channel_default);
    passed =
        passed &&
        expect(undo_result == CTEX_RESULT_SUCCESS && restore.tile_count == 1 &&
                   restore.exchanged_storage_count == 1 && restore.copied_pixel_bytes == 0 &&
                   default_restored,
               "undo did not exchange the exact tile storage") &&
        expect(ctex_texture_set_get_tile_history_budget(document, set_id, TILE_BYTES, &budget) ==
                       CTEX_RESULT_SUCCESS &&
                   budget.retained_bytes == TILE_BYTES && budget.undo_steps == 0 &&
                   budget.redo_steps == 1,
               "undo budget report is wrong") &&
        expect(ctex_texture_set_redo_tiles(document, set_id, &restore) == CTEX_RESULT_SUCCESS &&
                   restore.exchanged_storage_count == 1 && first_pixel_is(document, set_id, first),
               "redo did not restore the exact changed tile") &&
        expect(ctex_texture_set_undo_tiles(document, set_id, &restore) == CTEX_RESULT_SUCCESS,
               "second undo failed") &&
        expect(begin_capture(document, set_id, "replacement", &capture) &&
                   commit_pixel(document, set_id, second) &&
                   ctex_tile_history_capture_commit(capture, &commit) == CTEX_RESULT_SUCCESS,
               "replacement history step failed");
    ctex_tile_history_capture_destroy(capture);
    capture = NULL;

    passed = passed &&
             expect(ctex_texture_set_get_tile_history_budget(document, set_id, 0, &budget) ==
                            CTEX_RESULT_SUCCESS &&
                        budget.undo_steps == 1 && budget.redo_steps == 0,
                    "new edit did not release redo history") &&
             expect(ctex_texture_set_redo_tiles(document, set_id, &restore) == CTEX_RESULT_NO_REDO,
                    "empty redo did not return its distinct result") &&
             expect(first_pixel_is(document, set_id, second),
                    "failed redo changed the replacement pixels");

    ctex_tile_history_capture_destroy(capture);
    ctex_document_destroy(document);
    return passed;
}

int main(void) { return tile_history_is_budgeted_symmetric_and_invalidates_redo() ? 0 : 1; }
