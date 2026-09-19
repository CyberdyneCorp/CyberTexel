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
        .display_name = "Preview",
        .partition_kind = CTEX_PARTITION_SOURCE_MATERIAL,
        .partition_key = "preview",
        .uv_set = "uv0",
        .width = 3,
        .height = 1,
        .default_bit_depth = 16,
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
           ctex_texture_set_set_channel_enabled(*document, texture_set_id, "pbr.height", 1, 16) ==
               CTEX_RESULT_SUCCESS;
}

static int preview_is_isolated_and_commits_exactly(void) {
    ctex_document* document = NULL;
    char texture_set_id[128] = {0};
    ctex_paint_preview_session* session = NULL;
    ctex_paint_preview_session* verification = NULL;
    ctex_paint_preview_info info = {.size = CTEX_PAINT_PREVIEW_INFO_CURRENT_SIZE};
    uint16_t pixels[3] = {9, 9, 9};
    uint16_t value = 32768;
    const uint8_t coverage[3] = {0, 1, 0};
    size_t required_size = 0;
    size_t changed_count = 0;
    ctex_paint_tile_coordinate changed = {9, 9};
    int passed =
        expect(create_fixture(&document, texture_set_id, sizeof(texture_set_id)),
               "preview fixture creation failed") &&
        expect(ctex_paint_preview_session_create(document, texture_set_id, "pbr.height",
                                                 &session) == CTEX_RESULT_SUCCESS,
               "preview session creation failed") &&
        expect(ctex_paint_preview_session_write_pixel(session, 1, 0, &value, sizeof(value)) ==
                   CTEX_RESULT_SUCCESS,
               "preview pixel write failed") &&
        expect(ctex_paint_preview_session_get_pixels(session, pixels, sizeof(pixels),
                                                     &required_size) == CTEX_RESULT_SUCCESS,
               "provisional readback failed") &&
        expect(required_size == sizeof(pixels) && pixels[0] == 0 && pixels[1] == value &&
                   pixels[2] == 0,
               "provisional bytes are wrong") &&
        expect(ctex_paint_preview_session_get_changed_tiles(session, &changed, 1, &changed_count) ==
                   CTEX_RESULT_SUCCESS,
               "changed-tile query failed") &&
        expect(changed_count == 1 && changed.x == 0 && changed.y == 0,
               "changed-tile report is wrong") &&
        expect(ctex_paint_preview_session_finalize(session, coverage, 3, 1, &info) ==
                   CTEX_RESULT_SUCCESS,
               "preview finalization failed") &&
        expect(info.state == CTEX_PAINT_PREVIEW_FINAL && info.dilated_texel_count == 2 &&
                   info.pixel_byte_count == sizeof(pixels),
               "final preview metadata is wrong") &&
        expect(ctex_paint_preview_session_commit(session, &info) == CTEX_RESULT_SUCCESS,
               "preview commit failed") &&
        expect(info.state == CTEX_PAINT_PREVIEW_COMMITTED && info.maximum_component_error == 0.0 &&
                   info.committed_epoch != 0,
               "commit report is wrong") &&
        expect(ctex_paint_preview_session_get_pixels(session, pixels, sizeof(pixels),
                                                     &required_size) == CTEX_RESULT_SUCCESS,
               "committed preview readback failed") &&
        expect(pixels[0] == value && pixels[1] == value && pixels[2] == value,
               "dilated committed preview bytes are wrong") &&
        expect(ctex_paint_preview_session_create(document, texture_set_id, "pbr.height",
                                                 &verification) == CTEX_RESULT_SUCCESS,
               "committed channel could not seed a new preview") &&
        expect(ctex_paint_preview_session_get_pixels(verification, pixels, sizeof(pixels),
                                                     &required_size) == CTEX_RESULT_SUCCESS,
               "committed channel readback failed") &&
        expect(pixels[0] == value && pixels[1] == value && pixels[2] == value,
               "commit did not publish the final preview bytes");
    ctex_paint_preview_session_destroy(verification);
    ctex_paint_preview_session_destroy(session);
    ctex_document_destroy(document);
    return passed;
}

static int stale_commit_and_cancel_are_refused(void) {
    ctex_document* document = NULL;
    char texture_set_id[128] = {0};
    ctex_paint_preview_session* first = NULL;
    ctex_paint_preview_session* stale = NULL;
    ctex_paint_preview_session* cancelled = NULL;
    ctex_paint_preview_info info = {.size = CTEX_PAINT_PREVIEW_INFO_CURRENT_SIZE};
    const uint8_t coverage[3] = {1, 1, 1};
    uint16_t first_value = 100;
    uint16_t stale_value = 200;
    int passed =
        create_fixture(&document, texture_set_id, sizeof(texture_set_id)) &&
        ctex_paint_preview_session_create(document, texture_set_id, "pbr.height", &first) ==
            CTEX_RESULT_SUCCESS &&
        ctex_paint_preview_session_create(document, texture_set_id, "pbr.height", &stale) ==
            CTEX_RESULT_SUCCESS &&
        ctex_paint_preview_session_write_pixel(first, 0, 0, &first_value, sizeof(first_value)) ==
            CTEX_RESULT_SUCCESS &&
        ctex_paint_preview_session_write_pixel(stale, 0, 0, &stale_value, sizeof(stale_value)) ==
            CTEX_RESULT_SUCCESS &&
        ctex_paint_preview_session_finalize(first, coverage, 3, 0, &info) == CTEX_RESULT_SUCCESS &&
        ctex_paint_preview_session_finalize(stale, coverage, 3, 0, &info) == CTEX_RESULT_SUCCESS &&
        ctex_paint_preview_session_commit(first, &info) == CTEX_RESULT_SUCCESS;
    if (passed) {
        passed =
            expect(ctex_paint_preview_session_commit(stale, &info) == CTEX_RESULT_INVALID_ARGUMENT,
                   "stale preview commit was accepted") &&
            expect(ctex_get_last_diagnostic_code() == CTEX_DIAGNOSTIC_INVALID_PAINT_PREVIEW,
                   "stale preview diagnostic is wrong");
    }
    if (passed) {
        passed =
            ctex_paint_preview_session_create(document, texture_set_id, "pbr.height", &cancelled) ==
                CTEX_RESULT_SUCCESS &&
            ctex_paint_preview_session_cancel(cancelled) == CTEX_RESULT_SUCCESS &&
            ctex_paint_preview_session_write_pixel(
                cancelled, 0, 0, &first_value, sizeof(first_value)) == CTEX_RESULT_INVALID_ARGUMENT;
    }
    ctex_paint_preview_session_destroy(cancelled);
    ctex_paint_preview_session_destroy(stale);
    ctex_paint_preview_session_destroy(first);
    ctex_document_destroy(document);
    return passed;
}

int main(void) {
    return preview_is_isolated_and_commits_exactly() && stale_commit_and_cancel_are_refused() ? 0
                                                                                              : 1;
}
