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

static int preview_uses_the_same_snapshot_transport(void) {
    ctex_document* document = NULL;
    ctex_paint_preview_session* preview = NULL;
    ctex_transport_snapshot_pool* pool = NULL;
    ctex_transport_snapshot* first = NULL;
    ctex_transport_snapshot* second = NULL;
    char texture_set_id[128] = {0};
    ctex_paint_preview_info preview_info = {.size = CTEX_PAINT_PREVIEW_INFO_CURRENT_SIZE};
    ctex_transport_snapshot_query_info first_info = {
        .size = CTEX_TRANSPORT_SNAPSHOT_QUERY_INFO_CURRENT_SIZE};
    ctex_transport_snapshot_query_info second_info = {
        .size = CTEX_TRANSPORT_SNAPSHOT_QUERY_INFO_CURRENT_SIZE};
    const uint8_t revision_one[3] = {21, 34, 55};
    const uint8_t revision_two[3] = {89, 144, 233};
    int passed =
        expect(create_fixture(&document, texture_set_id, sizeof(texture_set_id)),
               "preview transport fixture creation failed") &&
        expect(ctex_paint_preview_session_create(document, texture_set_id, "pbr.base_color",
                                                 &preview) == CTEX_RESULT_SUCCESS,
               "preview transport session creation failed") &&
        expect(ctex_paint_preview_session_get_info(preview, &preview_info) == CTEX_RESULT_SUCCESS,
               "preview transport initial cursor query failed") &&
        expect(ctex_paint_preview_session_write_pixel(preview, 1, 2, revision_one, 3) ==
                   CTEX_RESULT_SUCCESS,
               "preview transport first edit failed") &&
        expect(ctex_transport_snapshot_pool_create(2 * 64 * 64 * 3, &pool) == CTEX_RESULT_SUCCESS,
               "preview transport pool creation failed");

    const ctex_transport_revision_cursor initial = {
        .epoch = preview_info.preview_epoch,
        .revision = preview_info.preview_revision,
    };
    passed = expect(ctex_paint_preview_session_query_snapshot(pool, preview, initial, &first,
                                                              &first_info) == CTEX_RESULT_SUCCESS &&
                        first_info.changed_tile_count == 1,
                    "first preview snapshot query failed") &&
             expect(ctex_paint_preview_session_write_pixel(preview, 1, 2, revision_two, 3) ==
                        CTEX_RESULT_SUCCESS,
                    "preview transport second edit failed") &&
             expect(ctex_paint_preview_session_query_snapshot(
                        pool, preview, first_info.current_cursor, &second, &second_info) ==
                            CTEX_RESULT_SUCCESS &&
                        second_info.changed_tile_count == 1,
                    "following preview snapshot omitted the later edit") &&
             passed;

    ctex_transport_tile_version first_version = {0};
    ctex_transport_tile_version second_version = {0};
    size_t version_count = 0;
    passed = expect(ctex_transport_snapshot_get_tile_versions(
                        first, &first_version, 1, &version_count) == CTEX_RESULT_SUCCESS &&
                        version_count == 1,
                    "first preview snapshot version query failed") &&
             expect(ctex_transport_snapshot_get_tile_versions(
                        second, &second_version, 1, &version_count) == CTEX_RESULT_SUCCESS &&
                        version_count == 1,
                    "second preview snapshot version query failed") &&
             passed;

    ctex_transport_tile_memory_layout first_layout = {
        .size = CTEX_TRANSPORT_TILE_MEMORY_LAYOUT_CURRENT_SIZE};
    ctex_transport_tile_memory_layout second_layout = {
        .size = CTEX_TRANSPORT_TILE_MEMORY_LAYOUT_CURRENT_SIZE};
    passed = expect(ctex_transport_snapshot_get_tile_memory_layout(
                        first, first_version, NULL, &first_layout) == CTEX_RESULT_SUCCESS,
                    "first preview snapshot layout query failed") &&
             expect(ctex_transport_snapshot_get_tile_memory_layout(
                        second, second_version, NULL, &second_layout) == CTEX_RESULT_SUCCESS,
                    "second preview snapshot layout query failed") &&
             passed;

    ctex_paint_preview_session_destroy(preview);
    preview = NULL;
    uint8_t first_pixels[64 * 64 * 3] = {0};
    uint8_t second_pixels[64 * 64 * 3] = {0};
    const ctex_transport_tile_readback_destination first_destination = {
        .size = CTEX_TRANSPORT_TILE_READBACK_DESTINATION_CURRENT_SIZE,
        .version = first_version,
        .layout = first_layout,
        .output = first_pixels,
        .output_size = sizeof(first_pixels),
    };
    const ctex_transport_tile_readback_destination second_destination = {
        .size = CTEX_TRANSPORT_TILE_READBACK_DESTINATION_CURRENT_SIZE,
        .version = second_version,
        .layout = second_layout,
        .output = second_pixels,
        .output_size = sizeof(second_pixels),
    };
    const size_t pixel_offset = ((2 * 64) + 1) * 3;
    passed = expect(ctex_transport_snapshot_read_tiles(first, NULL, &first_destination, 1) ==
                        CTEX_RESULT_SUCCESS,
                    "first preview snapshot readback failed after session destruction") &&
             expect(ctex_transport_snapshot_read_tiles(second, NULL, &second_destination, 1) ==
                        CTEX_RESULT_SUCCESS,
                    "second preview snapshot readback failed after session destruction") &&
             expect(memcmp(first_pixels + pixel_offset, revision_one, 3) == 0 &&
                        memcmp(second_pixels + pixel_offset, revision_two, 3) == 0,
                    "preview snapshots were torn or shared one mutable version") &&
             passed;

    ctex_transport_snapshot_destroy(second);
    ctex_transport_snapshot_destroy(first);
    ctex_transport_snapshot_pool_destroy(pool);
    ctex_paint_preview_session_destroy(preview);
    ctex_document_destroy(document);
    return passed;
}

static int asynchronous_readback_retains_snapshot_and_publishes_atomically(void) {
    ctex_document* document = NULL;
    ctex_transport_snapshot_pool* pool = NULL;
    ctex_transport_snapshot* snapshot = NULL;
    ctex_transport_readback* cpu = NULL;
    ctex_transport_readback* host = NULL;
    ctex_transport_readback* cancelled = NULL;
    ctex_transport_readback* failed = NULL;
    ctex_transport_readback* mismatched = NULL;
    char texture_set_id[128] = {0};
    const uint8_t pixel[3] = {7, 11, 13};
    ctex_transport_delta_info initial = {.size = CTEX_TRANSPORT_DELTA_INFO_CURRENT_SIZE};
    ctex_transport_snapshot_query_info query = {
        .size = CTEX_TRANSPORT_SNAPSHOT_QUERY_INFO_CURRENT_SIZE};
    ctex_transport_snapshot_memory_report memory = {
        .size = CTEX_TRANSPORT_SNAPSHOT_MEMORY_REPORT_CURRENT_SIZE};
    int passed =
        expect(create_fixture(&document, texture_set_id, sizeof(texture_set_id)),
               "async readback fixture creation failed") &&
        expect(ctex_texture_set_query_channel_delta(document, texture_set_id, "pbr.base_color",
                                                    (ctex_transport_revision_cursor){0, 0}, NULL, 0,
                                                    &initial) == CTEX_RESULT_SUCCESS,
               "async readback initial cursor query failed") &&
        expect(commit_pixel(document, texture_set_id, 1, 2, pixel),
               "async readback fixture edit failed") &&
        expect(ctex_transport_snapshot_pool_create(64 * 64 * 3, &pool) == CTEX_RESULT_SUCCESS,
               "async readback pool creation failed") &&
        expect(ctex_texture_set_query_channel_snapshot(pool, document, texture_set_id,
                                                       "pbr.base_color", initial.current_cursor,
                                                       &snapshot, &query) == CTEX_RESULT_SUCCESS,
               "async readback snapshot query failed");

    ctex_transport_tile_version version = {0};
    size_t version_count = 0;
    ctex_transport_tile_memory_layout layout = {.size =
                                                    CTEX_TRANSPORT_TILE_MEMORY_LAYOUT_CURRENT_SIZE};
    passed = expect(ctex_transport_snapshot_get_tile_versions(
                        snapshot, &version, 1, &version_count) == CTEX_RESULT_SUCCESS &&
                        version_count == 1,
                    "async readback tile version query failed") &&
             expect(ctex_transport_snapshot_get_tile_memory_layout(snapshot, version, NULL,
                                                                   &layout) == CTEX_RESULT_SUCCESS,
                    "async readback layout query failed") &&
             passed;

    uint8_t cpu_output[64 * 64 * 3];
    memset(cpu_output, 0xa5, sizeof(cpu_output));
    const ctex_transport_tile_readback_destination cpu_destination = {
        .size = CTEX_TRANSPORT_TILE_READBACK_DESTINATION_CURRENT_SIZE,
        .version = version,
        .layout = layout,
        .output = cpu_output,
        .output_size = sizeof(cpu_output),
    };
    ctex_transport_readback_info info = {.size = CTEX_TRANSPORT_READBACK_INFO_CURRENT_SIZE};
    passed =
        expect(ctex_transport_snapshot_begin_readback(snapshot, NULL, &cpu_destination, 1, &cpu) ==
                   CTEX_RESULT_SUCCESS,
               "CPU asynchronous readback did not start") &&
        expect(ctex_transport_readback_get_info(cpu, &info, NULL, 0) == CTEX_RESULT_SUCCESS &&
                   info.status == CTEX_TRANSPORT_READBACK_COMPLETE && info.output_readable == 1 &&
                   info.tile_count == 1 && memcmp(cpu_output + (((2 * 64) + 1) * 3), pixel, 3) == 0,
               "CPU asynchronous readback did not complete through the state model") &&
        passed;

    ctex_transport_tile_version host_version = version;
    host_version.residency = CTEX_TRANSPORT_TILE_HOST_DEVICE;
    uint8_t host_output[64 * 64 * 3];
    uint8_t cancelled_output[64 * 64 * 3];
    uint8_t failed_output[64 * 64 * 3];
    uint8_t mismatched_output[64 * 64 * 3];
    uint8_t completion_bytes[64 * 64 * 3];
    memset(host_output, 0xb6, sizeof(host_output));
    memset(cancelled_output, 0xc7, sizeof(cancelled_output));
    memset(failed_output, 0xd8, sizeof(failed_output));
    memset(mismatched_output, 0xe9, sizeof(mismatched_output));
    memset(completion_bytes, 0x2a, sizeof(completion_bytes));
    const ctex_transport_tile_readback_destination host_destination = {
        .size = CTEX_TRANSPORT_TILE_READBACK_DESTINATION_CURRENT_SIZE,
        .version = host_version,
        .layout = layout,
        .output = host_output,
        .output_size = sizeof(host_output),
    };
    ctex_transport_tile_readback_destination cancelled_destination = host_destination;
    cancelled_destination.output = cancelled_output;
    ctex_transport_tile_readback_destination failed_destination = host_destination;
    failed_destination.output = failed_output;
    ctex_transport_tile_readback_destination mismatched_destination = host_destination;
    mismatched_destination.output = mismatched_output;
    passed =
        expect(ctex_transport_snapshot_begin_host_readback(snapshot, NULL, &host_destination, 1,
                                                           &host) == CTEX_RESULT_SUCCESS,
               "host asynchronous readback did not start") &&
        expect(ctex_transport_snapshot_begin_host_readback(snapshot, NULL, &cancelled_destination,
                                                           1, &cancelled) == CTEX_RESULT_SUCCESS,
               "cancellable asynchronous readback did not start") &&
        expect(ctex_transport_snapshot_begin_host_readback(snapshot, NULL, &failed_destination, 1,
                                                           &failed) == CTEX_RESULT_SUCCESS,
               "fallible asynchronous readback did not start") &&
        expect(ctex_transport_snapshot_begin_host_readback(snapshot, NULL, &mismatched_destination,
                                                           1, &mismatched) == CTEX_RESULT_SUCCESS,
               "mismatch asynchronous readback did not start") &&
        passed;
    info.size = CTEX_TRANSPORT_READBACK_INFO_CURRENT_SIZE;
    passed = expect(ctex_transport_readback_get_info(host, &info, NULL, 0) == CTEX_RESULT_SUCCESS &&
                        info.status == CTEX_TRANSPORT_READBACK_PENDING &&
                        info.output_readable == 0 && host_output[0] == 0xb6,
                    "pending host readback exposed its output") &&
             passed;

    ctex_transport_snapshot_destroy(snapshot);
    snapshot = NULL;
    passed = expect(ctex_transport_snapshot_pool_get_memory_report(pool, &memory) ==
                            CTEX_RESULT_SUCCESS &&
                        memory.active_snapshots == 1 && memory.pinned_bytes == 64 * 64 * 3,
                    "active readbacks did not retain their pinned snapshot") &&
             passed;

    const ctex_transport_host_tile_completion completion = {
        .size = CTEX_TRANSPORT_HOST_TILE_COMPLETION_CURRENT_SIZE,
        .version = host_version,
        .layout = layout,
        .bytes = completion_bytes,
        .byte_size = sizeof(completion_bytes),
    };
    ctex_transport_host_tile_completion wrong_completion = completion;
    ++wrong_completion.version.generation;
    passed =
        expect(ctex_transport_readback_complete_host(host, &completion, 1) == CTEX_RESULT_SUCCESS &&
                   memcmp(host_output, completion_bytes, sizeof(host_output)) == 0,
               "host completion did not atomically publish its payload") &&
        expect(ctex_transport_readback_cancel(cancelled) == CTEX_RESULT_SUCCESS &&
                   ctex_transport_readback_complete_host(cancelled, &completion, 1) ==
                       CTEX_RESULT_INVALID_ARGUMENT &&
                   cancelled_output[0] == 0xc7,
               "cancelled readback accepted a late completion") &&
        expect(ctex_transport_readback_fail(failed, "device lost") == CTEX_RESULT_SUCCESS &&
                   failed_output[0] == 0xd8,
               "failed readback changed its output") &&
        expect(ctex_transport_readback_complete_host(mismatched, &wrong_completion, 1) ==
                       CTEX_RESULT_INVALID_ARGUMENT &&
                   mismatched_output[0] == 0xe9,
               "mismatched readback completion changed its output") &&
        passed;
    char detail[32] = {0};
    info.size = CTEX_TRANSPORT_READBACK_INFO_CURRENT_SIZE;
    passed = expect(ctex_transport_readback_get_info(failed, &info, detail, sizeof(detail)) ==
                            CTEX_RESULT_SUCCESS &&
                        info.status == CTEX_TRANSPORT_READBACK_FAILED &&
                        info.output_readable == 0 && strcmp(detail, "device lost") == 0,
                    "failed readback did not expose its terminal state") &&
             passed;

    ctex_transport_readback_destroy(failed);
    ctex_transport_readback_destroy(mismatched);
    ctex_transport_readback_destroy(cancelled);
    ctex_transport_readback_destroy(host);
    ctex_transport_readback_destroy(cpu);
    failed = NULL;
    mismatched = NULL;
    cancelled = NULL;
    host = NULL;
    cpu = NULL;
    passed = expect(ctex_transport_snapshot_pool_get_memory_report(pool, &memory) ==
                            CTEX_RESULT_SUCCESS &&
                        memory.active_snapshots == 0 && memory.pinned_bytes == 0,
                    "terminal readback destruction did not release the snapshot") &&
             passed;

    ctex_transport_readback_destroy(failed);
    ctex_transport_readback_destroy(mismatched);
    ctex_transport_readback_destroy(cancelled);
    ctex_transport_readback_destroy(host);
    ctex_transport_readback_destroy(cpu);
    ctex_transport_snapshot_destroy(snapshot);
    ctex_transport_snapshot_pool_destroy(pool);
    ctex_document_destroy(document);
    return passed;
}

int main(void) {
    return snapshot_readback_is_consistent_and_budgeted() &&
                   preview_uses_the_same_snapshot_transport() &&
                   asynchronous_readback_retains_snapshot_and_publishes_atomically()
               ? 0
               : 1;
}
