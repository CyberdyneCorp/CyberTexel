#include <ctex/capi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct fixture {
    ctex_document* document;
    ctex_layer_snapshot* snapshot;
    char set_id[128];
} fixture;

static int expect(int condition, const char* message) {
    if (!condition) {
        fprintf(stderr, "%s: %s\n", message, ctex_get_last_diagnostic());
    }
    return condition;
}

static ctex_layer_entry_descriptor entry(const char* id, uint32_t kind) {
    const ctex_layer_entry_descriptor result = {
        .size = CTEX_LAYER_ENTRY_DESCRIPTOR_CURRENT_SIZE,
        .identifier = id,
        .display_name = id,
        .kind = kind,
        .enabled = 1,
        .opacity = 1.0,
        .blend_mode = "normal",
    };
    return result;
}

static int begin_fixture(fixture* value, const ctex_layer_entry_descriptor* entries,
                         size_t entry_count, const ctex_layer_composite_raster_descriptor* content,
                         size_t content_count) {
    const ctex_texture_set_descriptor descriptor = {
        .size = CTEX_TEXTURE_SET_DESCRIPTOR_CURRENT_SIZE,
        .display_name = "Transaction",
        .partition_kind = CTEX_PARTITION_SOURCE_MATERIAL,
        .partition_key = "transaction",
        .uv_set = "uv0",
        .width = 128,
        .height = 64,
        .default_bit_depth = 8,
    };
    size_t required_size = 0;
    size_t count = 0;
    memset(value, 0, sizeof(*value));
    return ctex_document_create(&value->document) == CTEX_RESULT_SUCCESS &&
           ctex_document_create_texture_set(value->document, &descriptor) == CTEX_RESULT_SUCCESS &&
           ctex_document_get_texture_set_ids(value->document, value->set_id, sizeof(value->set_id),
                                             &required_size, &count) == CTEX_RESULT_SUCCESS &&
           count == 1 &&
           ctex_texture_set_set_channel_enabled(value->document, value->set_id, "pbr.base_color", 1,
                                                0) == CTEX_RESULT_SUCCESS &&
           ctex_texture_set_configure_tile_history(value->document, value->set_id, 1U << 20) ==
               CTEX_RESULT_SUCCESS &&
           ctex_texture_set_layer_append(value->document, value->set_id, entries, entry_count) ==
               CTEX_RESULT_SUCCESS &&
           ctex_layer_snapshot_create(128, 64, content, content_count, NULL, 0, &value->snapshot) ==
               CTEX_RESULT_SUCCESS;
}

static void end_fixture(fixture* value) {
    ctex_layer_snapshot_destroy(value->snapshot);
    ctex_document_destroy(value->document);
}

static int inspect_contains(const fixture* value, const char* text) {
    char json[4096] = {0};
    size_t required_size = 0;
    return ctex_texture_set_layer_inspect(value->document, value->set_id, json, sizeof(json),
                                          &required_size) == CTEX_RESULT_SUCCESS &&
           strstr(json, text) != NULL;
}

static ctex_layer_operation_descriptor create_operation(
    const ctex_layer_entry_descriptor* created) {
    const ctex_layer_operation_descriptor result = {
        .size = CTEX_LAYER_OPERATION_DESCRIPTOR_CURRENT_SIZE,
        .kind = CTEX_LAYER_OPERATION_CREATE,
        .entry = created,
        .maximum_output_bytes = 1U << 20,
        .appearance_tolerance = 1.0e-6F,
    };
    return result;
}

static int mixed_commit_cancel_and_history(void) {
    fixture value;
    if (!begin_fixture(&value, NULL, 0, NULL, 0)) {
        end_fixture(&value);
        return expect(0, "mixed transaction fixture failed");
    }
    const ctex_tile_history_target_descriptor target = {
        .size = CTEX_TILE_HISTORY_TARGET_DESCRIPTOR_CURRENT_SIZE,
        .semantic_id = "pbr.base_color",
        .tile_x = 0,
        .tile_y = 0,
    };
    ctex_texture_set_transaction* transaction = NULL;
    ctex_layer_entry_descriptor group = entry("group", CTEX_LAYER_ENTRY_GROUP);
    const ctex_layer_operation_descriptor create = create_operation(&group);
    int ok = expect(
        ctex_texture_set_begin_transaction(value.document, value.set_id, "gesture", &target, 1,
                                           value.snapshot, &transaction) == CTEX_RESULT_SUCCESS,
        "transaction begin failed");
    for (unsigned component = 1; ok && component <= 40; ++component) {
        const unsigned char pixel[3] = {(unsigned char)component, (unsigned char)component,
                                        (unsigned char)component};
        ok = expect(
            ctex_texture_set_transaction_write_pixel(transaction, "pbr.base_color", 0, 0, pixel,
                                                     sizeof(pixel)) == CTEX_RESULT_SUCCESS,
            "transaction pixel write failed");
    }
    ok = ok &&
         expect(ctex_texture_set_transaction_apply_layer_operation(transaction, &create) ==
                    CTEX_RESULT_SUCCESS,
                "transaction layer operation failed") &&
         expect(ctex_texture_set_transaction_set_layer_state(
                    transaction, "group", "Renamed group", 1, 0.5, "normal") == CTEX_RESULT_SUCCESS,
                "transaction layer state failed") &&
         expect(!inspect_contains(&value, "group"), "transaction published before commit");
    ctex_tile_history_commit_info commit = {.size = CTEX_TILE_HISTORY_COMMIT_INFO_CURRENT_SIZE};
    ok = ok &&
         expect(ctex_texture_set_transaction_commit(transaction, &commit) == CTEX_RESULT_SUCCESS &&
                    commit.committed == 1 && commit.tile_count == 1 &&
                    commit.layer_stack_changed == 1 && inspect_contains(&value, "Renamed group"),
                "mixed transaction did not commit as one step");
    ctex_document_memory_info memory = {.size = CTEX_DOCUMENT_MEMORY_INFO_CURRENT_SIZE};
    ok = ok && expect(ctex_document_get_memory_report(value.document, &memory, NULL, 0, NULL, 0) ==
                              CTEX_RESULT_SUCCESS &&
                          memory.texture_set_count == 1 && memory.history_retained_bytes > 0 &&
                          memory.total_resident_bytes >= memory.history_retained_bytes &&
                          memory.estimated_save_bytes > 0,
                      "document memory sizing report omitted history or save estimate");
    ctex_document_texture_set_memory_info set_memory = {0};
    char set_ids[128] = {0};
    ok = ok &&
         expect(ctex_document_get_memory_report(value.document, &memory, &set_memory, 1, set_ids,
                                                sizeof(set_ids)) == CTEX_RESULT_SUCCESS &&
                    strcmp(set_ids + set_memory.texture_set_id_offset, value.set_id) == 0 &&
                    set_memory.history_retained_bytes == memory.history_retained_bytes &&
                    set_memory.estimated_save_bytes > 0,
                "document memory detail report did not match its aggregate");
    ctex_document_texture_set_memory_info sentinel = {.texture_set_id_offset = 77};
    memory.texture_set_count = 99;
    ok = ok && expect(ctex_document_get_memory_report(value.document, &memory, &sentinel, 1,
                                                      set_ids, 1) == CTEX_RESULT_BUFFER_TOO_SMALL &&
                          sentinel.texture_set_id_offset == 77 && memory.texture_set_count == 99,
                      "short document memory output partially published data");
    ctex_tile_history_restore_info restored = {.size = CTEX_TILE_HISTORY_RESTORE_INFO_CURRENT_SIZE};
    ok = ok &&
         expect(ctex_texture_set_undo_tiles(value.document, value.set_id, &restored) ==
                        CTEX_RESULT_SUCCESS &&
                    restored.tile_count == 1 && restored.layer_stack_exchanged == 1 &&
                    !inspect_contains(&value, "group"),
                "mixed transaction did not undo as one step") &&
         expect(ctex_texture_set_redo_tiles(value.document, value.set_id, &restored) ==
                        CTEX_RESULT_SUCCESS &&
                    inspect_contains(&value, "Renamed group"),
                "mixed transaction did not redo as one step");
    ctex_texture_set_transaction_destroy(transaction);

    transaction = NULL;
    ctex_layer_entry_descriptor discarded = entry("discarded", CTEX_LAYER_ENTRY_GROUP);
    const ctex_layer_operation_descriptor discard = create_operation(&discarded);
    ok = ok && expect(ctex_texture_set_begin_transaction(value.document, value.set_id, "cancel",
                                                         NULL, 0, value.snapshot,
                                                         &transaction) == CTEX_RESULT_SUCCESS &&
                          ctex_texture_set_transaction_apply_layer_operation(
                              transaction, &discard) == CTEX_RESULT_SUCCESS,
                      "cancel transaction setup failed");
    ctex_texture_set_transaction_cancel(transaction);
    ok = ok &&
         expect(!inspect_contains(&value, "discarded"), "cancelled transaction changed live state");
    ctex_texture_set_transaction_destroy(transaction);
    end_fixture(&value);
    return ok;
}

static int create_default_graph(unsigned char** output, size_t* output_size) {
    ctex_material_graph_info info = {.size = CTEX_MATERIAL_GRAPH_INFO_CURRENT_SIZE};
    if (ctex_material_graph_create_default(&info, NULL, 0, NULL, 0) != CTEX_RESULT_SUCCESS) {
        return 0;
    }
    *output = (unsigned char*)malloc(info.canonical_size);
    if (*output == NULL || ctex_material_graph_create_default(&info, *output, info.canonical_size,
                                                              NULL, 0) != CTEX_RESULT_SUCCESS) {
        free(*output);
        *output = NULL;
        return 0;
    }
    *output_size = info.canonical_size;
    return 1;
}

static int non_pixel_edits_retain_zero_bytes(void) {
    const ctex_layer_channel_descriptor channel = {
        .size = CTEX_LAYER_CHANNEL_DESCRIPTOR_CURRENT_SIZE,
        .semantic_id = "pbr.base_color",
        .enabled = 1,
        .opacity = 1.0,
    };
    ctex_layer_entry_descriptor entries[2] = {entry("group", CTEX_LAYER_ENTRY_GROUP),
                                              entry("paint", CTEX_LAYER_ENTRY_PAINT)};
    entries[1].channels = &channel;
    entries[1].channel_count = 1;
    ctex_vec4f pixels[128 * 64] = {0};
    const ctex_layer_composite_raster_descriptor content = {
        .size = CTEX_LAYER_COMPOSITE_RASTER_DESCRIPTOR_CURRENT_SIZE,
        .entry_identifier = "paint",
        .semantic_id = "pbr.base_color",
        .width = 128,
        .height = 64,
        .pixels = pixels,
        .pixel_count = 128 * 64,
    };
    fixture value;
    if (!begin_fixture(&value, entries, 2, &content, 1)) {
        end_fixture(&value);
        return expect(0, "metadata transaction fixture failed");
    }
    unsigned char* graph = NULL;
    size_t graph_size = 0;
    if (!create_default_graph(&graph, &graph_size)) {
        end_fixture(&value);
        return expect(0, "metadata transaction graph creation failed");
    }
    ctex_texture_set_transaction* transaction = NULL;
    ctex_layer_channel_descriptor changed_channel = channel;
    changed_channel.opacity = 0.75;
    ctex_layer_entry_descriptor fill = entry("fill", CTEX_LAYER_ENTRY_FILL);
    fill.channels = &channel;
    fill.channel_count = 1;
    ctex_layer_composite_raster_descriptor fill_content = content;
    fill_content.entry_identifier = "fill";
    ctex_layer_operation_descriptor create_fill = create_operation(&fill);
    create_fill.replacement_content = &fill_content;
    create_fill.replacement_content_count = 1;
    create_fill.graph_serialized = graph;
    create_fill.graph_serialized_size = graph_size;
    int ok = expect(ctex_texture_set_begin_transaction(value.document, value.set_id, "metadata",
                                                       NULL, 0, value.snapshot,
                                                       &transaction) == CTEX_RESULT_SUCCESS,
                    "metadata transaction begin failed") &&
             expect(ctex_texture_set_transaction_set_layer_layout(transaction, "paint", "group",
                                                                  NULL) == CTEX_RESULT_SUCCESS,
                    "transaction layer layout failed") &&
             expect(ctex_texture_set_transaction_set_layer_channel(
                        transaction, "paint", &changed_channel) == CTEX_RESULT_SUCCESS,
                    "transaction layer channel failed") &&
             expect(ctex_texture_set_transaction_set_layer_state(transaction, "paint",
                                                                 "Renamed paint", 1, 0.5,
                                                                 "multiply") == CTEX_RESULT_SUCCESS,
                    "transaction metadata state failed") &&
             expect(ctex_texture_set_transaction_apply_layer_operation(transaction, &create_fill) ==
                        CTEX_RESULT_SUCCESS,
                    "transaction fill creation failed") &&
             expect(ctex_texture_set_transaction_set_fill_graph(transaction, "fill", graph,
                                                                graph_size) == CTEX_RESULT_SUCCESS,
                    "transaction fill graph edit failed");
    ctex_tile_history_commit_info commit = {.size = CTEX_TILE_HISTORY_COMMIT_INFO_CURRENT_SIZE};
    ok = ok &&
         expect(ctex_texture_set_transaction_commit(transaction, &commit) == CTEX_RESULT_SUCCESS &&
                    commit.committed == 1 && commit.tile_count == 0 && commit.retained_bytes == 0 &&
                    commit.layer_stack_changed == 1 && inspect_contains(&value, "Renamed paint"),
                "metadata transaction consumed pixel history bytes");
    free(graph);
    ctex_texture_set_transaction_destroy(transaction);
    ctex_tile_history_restore_info restored = {.size = CTEX_TILE_HISTORY_RESTORE_INFO_CURRENT_SIZE};
    ok = ok && expect(ctex_texture_set_undo_tiles(value.document, value.set_id, &restored) ==
                              CTEX_RESULT_SUCCESS &&
                          restored.tile_count == 0 && restored.layer_stack_exchanged == 1 &&
                          inspect_contains(&value, "\"display_name\":\"paint\""),
                      "zero-byte metadata transaction was not undoable");
    end_fixture(&value);
    return ok;
}

static int bulk_write_round_trip(void) {
    fixture value;
    if (!begin_fixture(&value, NULL, 0, NULL, 0)) {
        end_fixture(&value);
        return expect(0, "bulk transaction fixture failed");
    }
    ctex_tile_history_target_descriptor targets[2] = {
        {.size = CTEX_TILE_HISTORY_TARGET_DESCRIPTOR_CURRENT_SIZE,
         .semantic_id = "pbr.base_color", .tile_x = 0, .tile_y = 0},
        {.size = CTEX_TILE_HISTORY_TARGET_DESCRIPTOR_CURRENT_SIZE,
         .semantic_id = "pbr.base_color", .tile_x = 1, .tile_y = 0},
    };
    unsigned char source[128 * 64 * 3];
    unsigned char actual[sizeof(source)];
    for (size_t index = 0; index < sizeof(source); ++index) {
        source[index] = (unsigned char)(index % 251);
    }
    ctex_channel_region_descriptor region = {
        .size = CTEX_CHANNEL_REGION_DESCRIPTOR_CURRENT_SIZE,
        .width = 128, .height = 64, .row_pitch_bytes = 128 * 3,
    };
    ctex_texture_set_transaction* transaction = NULL;
    int ok = expect(ctex_texture_set_begin_transaction(
                        value.document, value.set_id, "bulk", targets, 2, value.snapshot,
                        &transaction) == CTEX_RESULT_SUCCESS, "bulk transaction begin failed");
    if (ok) {
        ctex_channel_region_descriptor invalid = region;
        invalid.size = 1;
        ok = expect(ctex_texture_set_transaction_write_region(
                        transaction, "pbr.base_color", &invalid, source,
                        sizeof(source)) == CTEX_RESULT_INVALID_ARGUMENT &&
                        ctex_get_last_diagnostic_code() == CTEX_DIAGNOSTIC_INVALID_DESCRIPTOR_SIZE,
                    "bulk write accepted an invalid descriptor size");
        ok = ok && expect(ctex_texture_set_transaction_write_region(
                              transaction, "pbr.base_color", &region, source,
                              sizeof(source) - 1) == CTEX_RESULT_INVALID_ARGUMENT,
                          "bulk write accepted a short buffer");
        ok = ok && expect(ctex_texture_set_transaction_write_region(
                              transaction, "pbr.base_color", &region, source,
                              sizeof(source)) == CTEX_RESULT_SUCCESS,
                          "bulk write failed");
        ctex_tile_history_commit_info commit = {.size = CTEX_TILE_HISTORY_COMMIT_INFO_CURRENT_SIZE};
        ok = ok && expect(ctex_texture_set_transaction_commit(transaction, &commit) ==
                              CTEX_RESULT_SUCCESS && commit.tile_count == 2,
                          "bulk write did not commit both tiles");
    }
    ctex_texture_set_transaction_destroy(transaction);
    ctex_paint_preview_session* preview = NULL;
    size_t required = 0;
    ok = ok && expect(ctex_paint_preview_session_create(
                          value.document, value.set_id, "pbr.base_color", &preview) ==
                          CTEX_RESULT_SUCCESS &&
                          ctex_paint_preview_session_get_pixels(preview, actual, sizeof(actual),
                                                                &required) == CTEX_RESULT_SUCCESS &&
                          required == sizeof(source) && memcmp(source, actual, sizeof(source)) == 0,
                      "bulk write did not round-trip through channel read");
    ctex_paint_preview_session_destroy(preview);
    ctex_tile_history_restore_info restored = {.size = CTEX_TILE_HISTORY_RESTORE_INFO_CURRENT_SIZE};
    ok = ok && expect(ctex_texture_set_undo_tiles(value.document, value.set_id, &restored) ==
                          CTEX_RESULT_SUCCESS && restored.copied_pixel_bytes == 0 &&
                          restored.exchanged_storage_count == 2,
                      "bulk undo copied pixels");
    transaction = NULL;
    ok = ok && expect(ctex_texture_set_begin_transaction(
                          value.document, value.set_id, "bounded", targets, 1, value.snapshot,
                          &transaction) == CTEX_RESULT_SUCCESS,
                      "bounded bulk transaction begin failed");
    region.x = 63;
    region.width = 2;
    region.height = 1;
    region.row_pitch_bytes = 6;
    ok = ok && expect(ctex_texture_set_transaction_write_region(
                          transaction, "pbr.base_color", &region, source, 6) ==
                          CTEX_RESULT_INVALID_ARGUMENT &&
                          ctex_get_last_diagnostic_code() == CTEX_DIAGNOSTIC_INVALID_TILE_HISTORY,
                      "bulk write accepted an undeclared tile");
    ctex_texture_set_transaction_destroy(transaction);
    end_fixture(&value);
    return ok;
}

int main(void) {
    return mixed_commit_cancel_and_history() && non_pixel_edits_retain_zero_bytes() &&
                   bulk_write_round_trip()
               ? 0 : 1;
}
