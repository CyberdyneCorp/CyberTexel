#include <ctex/capi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct fixture {
    ctex_document* document;
    ctex_layer_snapshot* snapshot;
    char set_id[128];
} fixture;

typedef struct byte_blob {
    unsigned char* data;
    size_t size;
} byte_blob;

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

static ctex_layer_composite_raster_descriptor raster(const char* id, const ctex_vec4f* pixel,
                                                     const float* coverage) {
    const ctex_layer_composite_raster_descriptor result = {
        .size = CTEX_LAYER_COMPOSITE_RASTER_DESCRIPTOR_CURRENT_SIZE,
        .entry_identifier = id,
        .semantic_id = "pbr.base_color",
        .width = 1,
        .height = 1,
        .pixels = pixel,
        .pixel_count = 1,
        .coverage = coverage,
        .coverage_count = coverage == NULL ? 0 : 1,
    };
    return result;
}

static int begin_fixture(fixture* value, const ctex_layer_entry_descriptor* entries,
                         size_t entry_count, const ctex_layer_composite_raster_descriptor* content,
                         size_t content_count, const ctex_layer_composite_mask_descriptor* masks,
                         size_t mask_count) {
    const ctex_texture_set_descriptor descriptor = {
        .size = CTEX_TEXTURE_SET_DESCRIPTOR_CURRENT_SIZE,
        .display_name = "Operations",
        .partition_kind = CTEX_PARTITION_SOURCE_MATERIAL,
        .partition_key = "operations",
        .uv_set = "uv0",
        .width = 1,
        .height = 1,
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
           ctex_texture_set_layer_append(value->document, value->set_id, entries, entry_count) ==
               CTEX_RESULT_SUCCESS &&
           ctex_layer_snapshot_create(1, 1, content, content_count, masks, mask_count,
                                      &value->snapshot) == CTEX_RESULT_SUCCESS;
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

static ctex_layer_operation_descriptor operation(uint32_t kind, const char* identifier) {
    const ctex_layer_operation_descriptor result = {
        .size = CTEX_LAYER_OPERATION_DESCRIPTOR_CURRENT_SIZE,
        .kind = kind,
        .identifier = identifier,
        .source_deletion_policy = CTEX_LAYER_SOURCE_DELETION_REFUSE,
        .target_kind = CTEX_LAYER_ENTRY_PAINT,
        .maximum_output_bytes = 1U << 20,
        .appearance_tolerance = 1.0e-6F,
    };
    return result;
}

static int apply(fixture* value, const ctex_layer_operation_descriptor* descriptor) {
    ctex_layer_operation_info info = {.size = CTEX_LAYER_OPERATION_INFO_CURRENT_SIZE};
    char affected[1024] = {0};
    return ctex_texture_set_apply_layer_operation(value->document, value->set_id, value->snapshot,
                                                  descriptor, &info, NULL,
                                                  0) == CTEX_RESULT_SUCCESS &&
           info.affected_count != 0 && info.required_affected_id_size <= sizeof(affected) &&
           ctex_texture_set_apply_layer_operation(value->document, value->set_id, value->snapshot,
                                                  descriptor, &info, affected,
                                                  sizeof(affected)) == CTEX_RESULT_SUCCESS;
}

static int create_duplicate_delete_are_atomic(void) {
    fixture value;
    if (!begin_fixture(&value, NULL, 0, NULL, 0, NULL, 0)) {
        end_fixture(&value);
        return expect(0, "empty operation fixture failed");
    }
    const ctex_vec4f source = {0.2F, 0.2F, 0.2F, 0.0F};
    const ctex_layer_composite_raster_descriptor content = raster("original", &source, NULL);
    ctex_layer_entry_descriptor original = entry("original", CTEX_LAYER_ENTRY_PAINT);
    const ctex_layer_channel_descriptor channel = {
        .size = CTEX_LAYER_CHANNEL_DESCRIPTOR_CURRENT_SIZE,
        .semantic_id = "pbr.base_color",
        .enabled = 1,
        .opacity = 1.0,
    };
    original.channels = &channel;
    original.channel_count = 1;
    ctex_layer_operation_descriptor create = operation(CTEX_LAYER_OPERATION_CREATE, NULL);
    create.entry = &original;
    create.replacement_content = &content;
    create.replacement_content_count = 1;
    ctex_layer_operation_info info = {.size = CTEX_LAYER_OPERATION_INFO_CURRENT_SIZE};
    int ok = expect(ctex_texture_set_apply_layer_operation(value.document, value.set_id,
                                                           value.snapshot, &create, &info, NULL,
                                                           0) == CTEX_RESULT_SUCCESS &&
                        !inspect_contains(&value, "original"),
                    "create sizing call mutated the stack") &&
             expect(apply(&value, &create) && inspect_contains(&value, "original"),
                    "create did not publish stack and raster state");

    ctex_layer_operation_descriptor duplicate =
        operation(CTEX_LAYER_OPERATION_DUPLICATE, "original");
    duplicate.duplicate_identifier = "duplicate";
    char short_output[1] = {42};
    ok = ok &&
         expect(ctex_texture_set_apply_layer_operation(
                    value.document, value.set_id, value.snapshot, &duplicate, &info, short_output,
                    sizeof(short_output)) == CTEX_RESULT_BUFFER_TOO_SMALL &&
                    short_output[0] == 42 && !inspect_contains(&value, "duplicate"),
                "short affected-ID buffer partially committed duplicate") &&
         expect(apply(&value, &duplicate) && inspect_contains(&value, "duplicate"),
                "duplicate operation failed");
    ctex_layer_operation_descriptor remove = operation(CTEX_LAYER_OPERATION_DELETE, "duplicate");
    ok = ok && expect(apply(&value, &remove) && !inspect_contains(&value, "duplicate"),
                      "delete operation failed");
    end_fixture(&value);
    return ok;
}

static int layout_clear_and_invert_work(void) {
    const ctex_vec4f first_pixel = {0.2F, 0.2F, 0.2F, 0.0F};
    const ctex_vec4f second_pixel = {0.8F, 0.8F, 0.8F, 0.0F};
    const ctex_layer_channel_descriptor channel = {
        .size = CTEX_LAYER_CHANNEL_DESCRIPTOR_CURRENT_SIZE,
        .semantic_id = "pbr.base_color",
        .enabled = 1,
        .opacity = 1.0,
    };
    ctex_layer_entry_descriptor entries[3] = {
        entry("group", CTEX_LAYER_ENTRY_GROUP),
        entry("first", CTEX_LAYER_ENTRY_PAINT),
        entry("second", CTEX_LAYER_ENTRY_PAINT),
    };
    entries[1].parent_identifier = "group";
    entries[1].channels = &channel;
    entries[1].channel_count = 1;
    entries[2].channels = &channel;
    entries[2].channel_count = 1;
    const ctex_layer_composite_raster_descriptor content[2] = {
        raster("first", &first_pixel, NULL),
        raster("second", &second_pixel, NULL),
    };
    fixture value;
    if (!begin_fixture(&value, entries, 3, content, 2, NULL, 0)) {
        end_fixture(&value);
        return expect(0, "layout operation fixture failed");
    }
    ctex_layer_operation_descriptor reparent = operation(CTEX_LAYER_OPERATION_REPARENT, "second");
    reparent.parent_identifier = "group";
    ctex_layer_operation_descriptor reorder = operation(CTEX_LAYER_OPERATION_REORDER, "second");
    reorder.before_identifier = "first";
    ctex_layer_operation_descriptor invert = operation(CTEX_LAYER_OPERATION_INVERT, "second");
    ctex_layer_operation_descriptor clear = operation(CTEX_LAYER_OPERATION_CLEAR, "first");
    const int ok = expect(apply(&value, &reparent), "reparent operation failed") &&
                   expect(apply(&value, &reorder), "reorder operation failed") &&
                   expect(apply(&value, &invert), "invert operation failed") &&
                   expect(apply(&value, &clear), "clear operation failed");
    end_fixture(&value);
    return ok;
}

static int merge_down_preserves_appearance(void) {
    const ctex_vec4f low = {0.2F, 0.2F, 0.2F, 0.0F};
    const ctex_vec4f high = {0.8F, 0.8F, 0.8F, 0.0F};
    const ctex_layer_channel_descriptor channel = {
        .size = CTEX_LAYER_CHANNEL_DESCRIPTOR_CURRENT_SIZE,
        .semantic_id = "pbr.base_color",
        .enabled = 1,
        .opacity = 1.0,
    };
    ctex_layer_entry_descriptor entries[2] = {entry("bottom", CTEX_LAYER_ENTRY_PAINT),
                                              entry("top", CTEX_LAYER_ENTRY_PAINT)};
    entries[0].channels = entries[1].channels = &channel;
    entries[0].channel_count = entries[1].channel_count = 1;
    const ctex_layer_composite_raster_descriptor content[2] = {raster("bottom", &low, NULL),
                                                               raster("top", &high, NULL)};
    const ctex_layer_composite_raster_descriptor replacement = raster("bottom", &high, NULL);
    fixture value;
    if (!begin_fixture(&value, entries, 2, content, 2, NULL, 0)) {
        end_fixture(&value);
        return expect(0, "merge-down fixture failed");
    }
    ctex_layer_operation_descriptor merge = operation(CTEX_LAYER_OPERATION_MERGE_DOWN, "top");
    merge.replacement_content = &replacement;
    merge.replacement_content_count = 1;
    const int ok = expect(apply(&value, &merge) && !inspect_contains(&value, "\"id\":\"top\""),
                          "merge-down operation failed");
    end_fixture(&value);
    return ok;
}

static int merge_group_and_flatten_work(void) {
    const ctex_vec4f child_pixel = {0.7F, 0.7F, 0.7F, 0.0F};
    const ctex_layer_channel_descriptor channel = {
        .size = CTEX_LAYER_CHANNEL_DESCRIPTOR_CURRENT_SIZE,
        .semantic_id = "pbr.base_color",
        .enabled = 1,
        .opacity = 1.0,
    };
    ctex_layer_entry_descriptor grouped[2] = {entry("group", CTEX_LAYER_ENTRY_GROUP),
                                              entry("child", CTEX_LAYER_ENTRY_PAINT)};
    grouped[1].parent_identifier = "group";
    grouped[1].channels = &channel;
    grouped[1].channel_count = 1;
    const ctex_layer_composite_raster_descriptor child = raster("child", &child_pixel, NULL);
    const ctex_layer_composite_raster_descriptor merged = raster("group", &child_pixel, NULL);
    fixture value;
    if (!begin_fixture(&value, grouped, 2, &child, 1, NULL, 0)) {
        end_fixture(&value);
        return expect(0, "merge-group fixture failed");
    }
    ctex_layer_operation_descriptor merge = operation(CTEX_LAYER_OPERATION_MERGE_GROUP, "group");
    merge.replacement_content = &merged;
    merge.replacement_content_count = 1;
    int ok = expect(apply(&value, &merge) && !inspect_contains(&value, "\"id\":\"child\""),
                    "merge-group operation failed");
    end_fixture(&value);

    const ctex_vec4f low = {0.1F, 0.1F, 0.1F, 0.0F};
    const ctex_vec4f high = {0.9F, 0.9F, 0.9F, 0.0F};
    ctex_layer_entry_descriptor layers[2] = {entry("low", CTEX_LAYER_ENTRY_PAINT),
                                             entry("high", CTEX_LAYER_ENTRY_PAINT)};
    layers[0].channels = layers[1].channels = &channel;
    layers[0].channel_count = layers[1].channel_count = 1;
    const ctex_layer_composite_raster_descriptor content[2] = {raster("low", &low, NULL),
                                                               raster("high", &high, NULL)};
    if (!begin_fixture(&value, layers, 2, content, 2, NULL, 0)) {
        end_fixture(&value);
        return expect(0, "flatten fixture failed");
    }
    ctex_layer_entry_descriptor flat_entry = entry("flat", CTEX_LAYER_ENTRY_PAINT);
    flat_entry.channels = &channel;
    flat_entry.channel_count = 1;
    const ctex_layer_composite_raster_descriptor flat = raster("flat", &high, NULL);
    ctex_layer_operation_descriptor flatten = operation(CTEX_LAYER_OPERATION_FLATTEN, NULL);
    flatten.entry = &flat_entry;
    flatten.replacement_content = &flat;
    flatten.replacement_content_count = 1;
    ok = ok && expect(apply(&value, &flatten) && inspect_contains(&value, "\"id\":\"flat\"") &&
                          !inspect_contains(&value, "\"id\":\"high\""),
                      "flatten operation failed");
    end_fixture(&value);
    return ok;
}

static int create_default_graph(byte_blob* graph) {
    ctex_material_graph_info info = {.size = CTEX_MATERIAL_GRAPH_INFO_CURRENT_SIZE};
    if (ctex_material_graph_create_default(&info, NULL, 0, NULL, 0) != CTEX_RESULT_SUCCESS) {
        return 0;
    }
    graph->data = (unsigned char*)malloc(info.canonical_size);
    if (graph->data == NULL ||
        ctex_material_graph_create_default(&info, graph->data, info.canonical_size, NULL, 0) !=
            CTEX_RESULT_SUCCESS) {
        free(graph->data);
        graph->data = NULL;
        return 0;
    }
    graph->size = info.canonical_size;
    return 1;
}

static int convert_and_apply_mask_work(void) {
    const ctex_vec4f pixel = {0.6F, 0.6F, 0.6F, 0.0F};
    const ctex_layer_channel_descriptor channel = {
        .size = CTEX_LAYER_CHANNEL_DESCRIPTOR_CURRENT_SIZE,
        .semantic_id = "pbr.base_color",
        .enabled = 1,
        .opacity = 1.0,
    };
    ctex_layer_entry_descriptor paint = entry("paint", CTEX_LAYER_ENTRY_PAINT);
    paint.channels = &channel;
    paint.channel_count = 1;
    const ctex_layer_composite_raster_descriptor content = raster("paint", &pixel, NULL);
    fixture value;
    byte_blob graph = {0};
    if (!begin_fixture(&value, &paint, 1, &content, 1, NULL, 0) || !create_default_graph(&graph)) {
        free(graph.data);
        end_fixture(&value);
        return expect(0, "convert fixture failed");
    }
    ctex_layer_operation_descriptor convert = operation(CTEX_LAYER_OPERATION_CONVERT, "paint");
    convert.target_kind = CTEX_LAYER_ENTRY_FILL;
    convert.graph_serialized = graph.data;
    convert.graph_serialized_size = graph.size;
    int ok = expect(apply(&value, &convert) && inspect_contains(&value, "\"kind\":\"fill\""),
                    "convert operation failed");
    free(graph.data);
    end_fixture(&value);

    const float half_coverage = 0.5F;
    const double half_mask = 0.5;
    ctex_layer_entry_descriptor masked[2] = {entry("target", CTEX_LAYER_ENTRY_PAINT),
                                             entry("mask", CTEX_LAYER_ENTRY_MASK)};
    masked[0].channels = &channel;
    masked[0].channel_count = 1;
    masked[1].target_identifier = "target";
    const ctex_layer_composite_raster_descriptor target = raster("target", &pixel, NULL);
    const ctex_layer_composite_mask_descriptor mask = {
        .size = CTEX_LAYER_COMPOSITE_MASK_DESCRIPTOR_CURRENT_SIZE,
        .mask_identifier = "mask",
        .width = 1,
        .height = 1,
        .values = &half_mask,
        .value_count = 1,
    };
    if (!begin_fixture(&value, masked, 2, &target, 1, &mask, 1)) {
        end_fixture(&value);
        return expect(0, "apply-mask fixture failed");
    }
    const ctex_layer_composite_raster_descriptor applied = raster("target", &pixel, &half_coverage);
    ctex_layer_operation_descriptor apply_mask = operation(CTEX_LAYER_OPERATION_APPLY_MASK, "mask");
    apply_mask.replacement_content = &applied;
    apply_mask.replacement_content_count = 1;
    ok = ok && expect(apply(&value, &apply_mask) && !inspect_contains(&value, "\"id\":\"mask\""),
                      "apply-mask operation failed");
    end_fixture(&value);
    return ok;
}

int main(void) {
    return create_duplicate_delete_are_atomic() && layout_clear_and_invert_work() &&
                   merge_down_preserves_appearance() && merge_group_and_flatten_work() &&
                   convert_and_apply_mask_work()
               ? 0
               : 1;
}
