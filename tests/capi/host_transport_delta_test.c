#include <ctex/capi.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static int expect(int condition, const char* message) {
    if (!condition) {
        fprintf(stderr, "%s\n", message);
    }
    return condition;
}

static int create_fixture(ctex_document** document, char* texture_set_id, size_t capacity) {
    const ctex_texture_set_descriptor descriptor = {
        .size = CTEX_TEXTURE_SET_DESCRIPTOR_CURRENT_SIZE,
        .display_name = "Transport",
        .partition_kind = CTEX_PARTITION_SOURCE_MATERIAL,
        .partition_key = "transport",
        .uv_set = "uv0",
        .width = 70,
        .height = 70,
        .default_bit_depth = 8,
    };
    size_t required_size = 0;
    size_t count = 0;
    return ctex_document_create(document) == CTEX_RESULT_SUCCESS &&
           ctex_document_create_texture_set(*document, &descriptor) == CTEX_RESULT_SUCCESS &&
           ctex_document_get_texture_set_ids(*document, NULL, 0, &required_size, &count) ==
               CTEX_RESULT_SUCCESS &&
           required_size <= capacity && count == 1 &&
           ctex_document_get_texture_set_ids(*document, texture_set_id, capacity, &required_size,
                                             &count) == CTEX_RESULT_SUCCESS &&
           ctex_texture_set_set_channel_enabled(*document, texture_set_id, "pbr.base_color", 1,
                                                8) == CTEX_RESULT_SUCCESS;
}

static int commit_two_changed_tiles(ctex_document* document, const char* texture_set_id) {
    ctex_paint_preview_session* preview = NULL;
    ctex_paint_preview_info info = {.size = CTEX_PAINT_PREVIEW_INFO_CURRENT_SIZE};
    const uint8_t first[3] = {1, 2, 3};
    const uint8_t second[3] = {5, 8, 13};
    uint8_t coverage[70 * 70] = {0};
    coverage[(2 * 70) + 1] = 1;
    coverage[(65 * 70) + 65] = 1;
    const int passed =
        ctex_paint_preview_session_create(document, texture_set_id, "pbr.base_color", &preview) ==
            CTEX_RESULT_SUCCESS &&
        ctex_paint_preview_session_write_pixel(preview, 1, 2, first, sizeof(first)) ==
            CTEX_RESULT_SUCCESS &&
        ctex_paint_preview_session_write_pixel(preview, 65, 65, second, sizeof(second)) ==
            CTEX_RESULT_SUCCESS &&
        ctex_paint_preview_session_finalize(preview, coverage, sizeof(coverage), 0, &info) ==
            CTEX_RESULT_SUCCESS &&
        ctex_paint_preview_session_commit(preview, &info) == CTEX_RESULT_SUCCESS;
    ctex_paint_preview_session_destroy(preview);
    return passed;
}

static int revisions_and_delta_cost_are_exposed(void) {
    ctex_document* document = NULL;
    char texture_set_id[128] = {0};
    ctex_transport_delta_info initial = {.size = CTEX_TRANSPORT_DELTA_INFO_CURRENT_SIZE};
    ctex_transport_delta_info delta = {.size = CTEX_TRANSPORT_DELTA_INFO_CURRENT_SIZE};
    ctex_transport_tile_version tiles[2] = {{0}};
    ctex_transport_tile_version untouched = {.x = 99, .y = 99};
    int passed = expect(create_fixture(&document, texture_set_id, sizeof(texture_set_id)),
                        "transport fixture creation failed");
    passed = expect(ctex_texture_set_query_channel_delta(document, texture_set_id, "pbr.base_color",
                                                         (ctex_transport_revision_cursor){0, 0},
                                                         NULL, 0, &initial) == CTEX_RESULT_SUCCESS,
                    "initial transport cursor query failed") &&
             expect(initial.disposition == CTEX_TRANSPORT_FULL_RESYNCHRONIZATION_REQUIRED &&
                        initial.current_cursor.epoch != 0 && initial.current_cursor.revision == 0 &&
                        initial.changed_tile_count == 0 && initial.indexed_tiles_visited == 0,
                    "initial cursor did not request a bounded full resynchronization") &&
             passed;
    passed = expect(commit_two_changed_tiles(document, texture_set_id),
                    "transport fixture paint commit failed") &&
             passed;
    passed = expect(ctex_texture_set_query_channel_delta(document, texture_set_id, "pbr.base_color",
                                                         initial.current_cursor, NULL, 0,
                                                         &delta) == CTEX_RESULT_SUCCESS,
                    "delta sizing query failed") &&
             expect(delta.disposition == CTEX_TRANSPORT_DELTA_COMPLETE &&
                        delta.changed_tile_count == 2 && delta.indexed_tiles_visited == 2 &&
                        delta.current_cursor.revision == 2,
                    "delta did not coalesce changes or report indexed work") &&
             passed;
    passed = expect(ctex_texture_set_query_channel_delta(document, texture_set_id, "pbr.base_color",
                                                         initial.current_cursor, &untouched, 1,
                                                         &delta) == CTEX_RESULT_BUFFER_TOO_SMALL,
                    "delta accepted a short tile-version buffer") &&
             expect(untouched.x == 99 && untouched.y == 99,
                    "delta partially wrote a short tile-version buffer") &&
             passed;
    passed =
        expect(ctex_texture_set_query_channel_delta(document, texture_set_id, "pbr.base_color",
                                                    initial.current_cursor, tiles, 2,
                                                    &delta) == CTEX_RESULT_SUCCESS,
               "delta tile-version query failed") &&
        expect(tiles[0].x == 0 && tiles[0].y == 0 && tiles[1].x == 1 && tiles[1].y == 1 &&
                   tiles[0].revision != 0 && tiles[1].revision != 0 && tiles[0].generation == 1 &&
                   tiles[1].generation == 1 && tiles[0].residency == CTEX_TRANSPORT_TILE_CPU,
               "delta lost tile order, revision, generation, or residency") &&
        passed;

    ctex_transport_revision_cursor reset = {0, 0};
    passed = expect(ctex_texture_set_reset_channel_revision_history(
                        document, texture_set_id, "pbr.base_color", &reset) == CTEX_RESULT_SUCCESS,
                    "revision history reset failed") &&
             expect(reset.epoch == delta.current_cursor.epoch + 1 && reset.revision == 0,
                    "revision reset did not advance its epoch") &&
             passed;
    ctex_transport_delta_info stale = {.size = CTEX_TRANSPORT_DELTA_INFO_CURRENT_SIZE};
    passed = expect(ctex_texture_set_query_channel_delta(document, texture_set_id, "pbr.base_color",
                                                         delta.current_cursor, NULL, 0,
                                                         &stale) == CTEX_RESULT_SUCCESS,
                    "stale cursor query failed") &&
             expect(stale.disposition == CTEX_TRANSPORT_FULL_RESYNCHRONIZATION_REQUIRED &&
                        stale.current_cursor.epoch == reset.epoch &&
                        stale.changed_tile_count == 0 && stale.indexed_tiles_visited == 0,
                    "stale cursor returned a partial delta") &&
             passed;
    ctex_document_destroy(document);
    return passed;
}

int main(void) { return revisions_and_delta_cost_are_exposed() ? 0 : 1; }
