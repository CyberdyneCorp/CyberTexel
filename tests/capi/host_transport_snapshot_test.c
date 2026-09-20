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
        .display_name = "Snapshot",
        .partition_kind = CTEX_PARTITION_SOURCE_MATERIAL,
        .partition_key = "snapshot",
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

static int commit_pixel(ctex_document* document, const char* texture_set_id, uint32_t x, uint32_t y,
                        const uint8_t value[3]) {
    ctex_paint_preview_session* preview = NULL;
    ctex_paint_preview_info info = {.size = CTEX_PAINT_PREVIEW_INFO_CURRENT_SIZE};
    uint8_t coverage[70 * 70] = {0};
    coverage[(y * 70) + x] = 1;
    const int passed =
        ctex_paint_preview_session_create(document, texture_set_id, "pbr.base_color", &preview) ==
            CTEX_RESULT_SUCCESS &&
        ctex_paint_preview_session_write_pixel(preview, x, y, value, 3) == CTEX_RESULT_SUCCESS &&
        ctex_paint_preview_session_finalize(preview, coverage, sizeof(coverage), 0, &info) ==
            CTEX_RESULT_SUCCESS &&
        ctex_paint_preview_session_commit(preview, &info) == CTEX_RESULT_SUCCESS;
    ctex_paint_preview_session_destroy(preview);
    return passed;
}

static int snapshot_readback_is_consistent_and_budgeted(void) {
    ctex_document* document = NULL;
    ctex_transport_snapshot_pool* pool = NULL;
    ctex_transport_snapshot* first = NULL;
    ctex_transport_snapshot* second = NULL;
    char texture_set_id[128] = {0};
    ctex_transport_delta_info initial = {.size = CTEX_TRANSPORT_DELTA_INFO_CURRENT_SIZE};
    ctex_transport_snapshot_query_info first_info = {
        .size = CTEX_TRANSPORT_SNAPSHOT_QUERY_INFO_CURRENT_SIZE};
    ctex_transport_snapshot_query_info second_info = {
        .size = CTEX_TRANSPORT_SNAPSHOT_QUERY_INFO_CURRENT_SIZE};
    ctex_transport_snapshot_memory_report memory = {
        .size = CTEX_TRANSPORT_SNAPSHOT_MEMORY_REPORT_CURRENT_SIZE};
    const uint8_t revision_one[3] = {1, 2, 3};
    const uint8_t revision_two[3] = {5, 8, 13};
    int passed = expect(create_fixture(&document, texture_set_id, sizeof(texture_set_id)),
                        "snapshot fixture creation failed");
    passed = expect(ctex_texture_set_query_channel_delta(document, texture_set_id, "pbr.base_color",
                                                         (ctex_transport_revision_cursor){0, 0},
                                                         NULL, 0, &initial) == CTEX_RESULT_SUCCESS,
                    "snapshot initial cursor query failed") &&
             expect(commit_pixel(document, texture_set_id, 1, 2, revision_one),
                    "snapshot first edit failed") &&
             expect(ctex_transport_snapshot_pool_create(64 * 64 * 3, &pool) == CTEX_RESULT_SUCCESS,
                    "snapshot pool creation failed") &&
             passed;
    passed = expect(ctex_texture_set_query_channel_snapshot(
                        pool, document, texture_set_id, "pbr.base_color", initial.current_cursor,
                        &first, &first_info) == CTEX_RESULT_SUCCESS,
                    "first snapshot query failed") &&
             expect(first != NULL && first_info.changed_tile_count == 1 &&
                        first_info.indexed_tiles_visited == 1 &&
                        first_info.retained_bytes == 64 * 64 * 3,
                    "first snapshot metadata is wrong") &&
             passed;

    size_t version_count = 0;
    ctex_transport_tile_version version = {0};
    passed =
        expect(ctex_transport_snapshot_get_tile_versions(first, NULL, 0, &version_count) ==
                       CTEX_RESULT_SUCCESS &&
                   version_count == 1,
               "snapshot tile-version sizing failed") &&
        expect(ctex_transport_snapshot_get_tile_versions(first, &version, 1, &version_count) ==
                       CTEX_RESULT_SUCCESS &&
                   version.x == 0 && version.y == 0 && version.residency == CTEX_TRANSPORT_TILE_CPU,
               "snapshot tile version is wrong") &&
        passed;

    const ctex_transport_pixel_format uint16_rgb = {
        .component_type = CTEX_TRANSPORT_COMPONENT_UINT16_UNORM,
        .channel_count = 3,
    };
    ctex_transport_format_selection selection = {.size =
                                                     CTEX_TRANSPORT_FORMAT_SELECTION_CURRENT_SIZE};
    passed =
        expect(ctex_transport_snapshot_negotiate_format(
                   first, &uint16_rgb, 1, CTEX_TRANSPORT_EXACT_FORMAT_ONLY, &selection) ==
                   CTEX_RESULT_UNSUPPORTED_OPERATION,
               "exact-only format negotiation silently converted") &&
        expect(ctex_transport_snapshot_negotiate_format(first, &uint16_rgb, 1,
                                                        CTEX_TRANSPORT_ALLOW_FORMAT_CONVERSION,
                                                        &selection) == CTEX_RESULT_SUCCESS,
               "declared format conversion was not negotiated") &&
        expect(
            selection.source_format.component_type == CTEX_TRANSPORT_COMPONENT_UINT8_UNORM &&
                selection.output_format.component_type == CTEX_TRANSPORT_COMPONENT_UINT16_UNORM &&
                selection.conversion == CTEX_TRANSPORT_CONVERSION_UINT8_TO_UINT16,
            "format negotiation returned the wrong selection") &&
        passed;

    ctex_transport_tile_memory_layout layout = {.size =
                                                    CTEX_TRANSPORT_TILE_MEMORY_LAYOUT_CURRENT_SIZE};
    passed = expect(ctex_transport_snapshot_get_tile_memory_layout(first, version, &selection,
                                                                   &layout) == CTEX_RESULT_SUCCESS,
                    "converted tile layout query failed") &&
             expect(layout.width == 64 && layout.height == 64 && layout.row_pitch_bytes == 64 * 6 &&
                        layout.pixel_stride_bytes == 6 &&
                        layout.channel_order == CTEX_TRANSPORT_CHANNEL_ORDER_RGB &&
                        layout.component_type == CTEX_TRANSPORT_COMPONENT_UINT16_UNORM &&
                        layout.byte_size == 64 * 64 * 6,
                    "converted tile layout is not directly uploadable") &&
             passed;

    passed = expect(commit_pixel(document, texture_set_id, 1, 2, revision_two),
                    "snapshot second edit failed") &&
             passed;
    uint16_t pixels[64 * 64 * 3] = {0};
    const ctex_transport_tile_readback_destination destination = {
        .size = CTEX_TRANSPORT_TILE_READBACK_DESTINATION_CURRENT_SIZE,
        .version = version,
        .layout = layout,
        .output = pixels,
        .output_size = sizeof(pixels),
    };
    passed = expect(ctex_transport_snapshot_read_tiles(first, &selection, &destination, 1) ==
                        CTEX_RESULT_SUCCESS,
                    "pinned snapshot readback failed after a later edit") &&
             expect(pixels[((2 * 64) + 1) * 3] == 257 && pixels[(((2 * 64) + 1) * 3) + 1] == 514 &&
                        pixels[(((2 * 64) + 1) * 3) + 2] == 771,
                    "pinned snapshot was torn by a later edit") &&
             passed;

    passed = expect(ctex_transport_snapshot_pool_get_memory_report(pool, &memory) ==
                            CTEX_RESULT_SUCCESS &&
                        memory.budget_bytes == 64 * 64 * 3 && memory.pinned_bytes == 64 * 64 * 3 &&
                        memory.active_snapshots == 1,
                    "snapshot memory report omitted pinned bytes") &&
             expect(ctex_texture_set_query_channel_snapshot(
                        pool, document, texture_set_id, "pbr.base_color", first_info.current_cursor,
                        &second, &second_info) == CTEX_RESULT_OVER_BUDGET &&
                        second == NULL && second_info.additional_pinned_bytes == 64 * 64 * 3,
                    "snapshot pool exceeded its pinned-byte budget") &&
             passed;

    ctex_transport_snapshot_destroy(first);
    first = NULL;
    passed = expect(ctex_transport_snapshot_pool_get_memory_report(pool, &memory) ==
                            CTEX_RESULT_SUCCESS &&
                        memory.pinned_bytes == 0 && memory.active_snapshots == 0,
                    "snapshot release did not reclaim its budget") &&
             expect(ctex_texture_set_query_channel_snapshot(
                        pool, document, texture_set_id, "pbr.base_color", first_info.current_cursor,
                        &second, &second_info) == CTEX_RESULT_SUCCESS &&
                        second != NULL && second_info.changed_tile_count == 1,
                    "released snapshot capacity was not reusable") &&
             passed;

    ctex_transport_snapshot_destroy(second);
    ctex_transport_snapshot_destroy(first);
    ctex_transport_snapshot_pool_destroy(pool);
    ctex_document_destroy(document);
    return passed;
}

int main(void) { return snapshot_readback_is_consistent_and_budgeted() ? 0 : 1; }
