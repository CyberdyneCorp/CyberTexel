#include <ctex/capi.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

static int expect(int condition, const char* message) {
    if (!condition) {
        fprintf(stderr, "%s\n", message);
    }
    return condition;
}

static int create_fixture(ctex_document** document, char* set_id, size_t set_id_size) {
    const ctex_texture_set_descriptor descriptor = {
        .size = CTEX_TEXTURE_SET_DESCRIPTOR_CURRENT_SIZE,
        .display_name = "Composite",
        .partition_kind = CTEX_PARTITION_SOURCE_MATERIAL,
        .partition_key = "composite",
        .uv_set = "uv0",
        .width = 1,
        .height = 1,
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

static int cpu_composite_is_grouped_and_bit_deterministic(void) {
    ctex_document* document = NULL;
    char set_id[128] = {0};
    if (!expect(create_fixture(&document, set_id, sizeof(set_id)),
                "composite fixture creation failed")) {
        ctex_document_destroy(document);
        return 0;
    }
    const ctex_layer_channel_descriptor base_channel = {
        .size = CTEX_LAYER_CHANNEL_DESCRIPTOR_CURRENT_SIZE,
        .semantic_id = "pbr.base_color",
        .enabled = 1,
        .opacity = 1.0,
    };
    ctex_layer_entry_descriptor entries[3] = {
        entry("group", CTEX_LAYER_ENTRY_GROUP),
        entry("low", CTEX_LAYER_ENTRY_PAINT),
        entry("high", CTEX_LAYER_ENTRY_PAINT),
    };
    entries[0].opacity = 0.5;
    entries[0].blend_mode = "multiply";
    entries[1].parent_identifier = "group";
    entries[1].channels = &base_channel;
    entries[1].channel_count = 1;
    entries[2].parent_identifier = "group";
    entries[2].channels = &base_channel;
    entries[2].channel_count = 1;

    const ctex_vec4f low = {0.2F, 0.2F, 0.2F, 0.0F};
    const ctex_vec4f high = {0.8F, 0.8F, 0.8F, 0.0F};
    const ctex_layer_composite_raster_descriptor content[2] = {
        {.size = CTEX_LAYER_COMPOSITE_RASTER_DESCRIPTOR_CURRENT_SIZE,
         .entry_identifier = "low",
         .semantic_id = "pbr.base_color",
         .width = 1,
         .height = 1,
         .pixels = &low,
         .pixel_count = 1},
        {.size = CTEX_LAYER_COMPOSITE_RASTER_DESCRIPTOR_CURRENT_SIZE,
         .entry_identifier = "high",
         .semantic_id = "pbr.base_color",
         .width = 1,
         .height = 1,
         .pixels = &high,
         .pixel_count = 1},
    };
    ctex_layer_composite_info info = {.size = CTEX_LAYER_COMPOSITE_INFO_CURRENT_SIZE};
    ctex_layer_composite_channel_info channel = {0};
    char semantic[32] = {0};
    ctex_vec4f isolated = {0};
    ctex_vec4f repeated = {0};
    ctex_vec4f passed = {0};

    int ok =
        expect(ctex_texture_set_layer_append(document, set_id, entries, 3) == CTEX_RESULT_SUCCESS,
               "composite stack append failed") &&
        expect(ctex_texture_set_layer_composite_cpu(document, set_id, 1, 1, content, 2, NULL, 0,
                                                    &info, NULL, 0, NULL, 0, NULL,
                                                    0) == CTEX_RESULT_SUCCESS &&
                   info.channel_count == 1 && info.required_pixel_count == 1,
               "composite sizing query failed") &&
        expect(ctex_texture_set_layer_composite_cpu(document, set_id, 1, 1, content, 2, NULL, 0,
                                                    &info, &channel, 1, semantic, sizeof(semantic),
                                                    &isolated, 1) == CTEX_RESULT_SUCCESS &&
                   strcmp(semantic, "pbr.base_color") == 0 && channel.component_count == 3 &&
                   fabsf(isolated.x - 0.45F) < 1.0e-6F,
               "isolated group was not composited once") &&
        expect(ctex_texture_set_layer_composite_cpu(document, set_id, 1, 1, content, 2, NULL, 0,
                                                    &info, &channel, 1, semantic, sizeof(semantic),
                                                    &repeated, 1) == CTEX_RESULT_SUCCESS &&
                   memcmp(&isolated, &repeated, sizeof(isolated)) == 0,
               "repeated CPU composite was not bit-identical") &&
        expect(ctex_texture_set_layer_set_state(document, set_id, "group", "group", 1, 0.5,
                                                "pass_through") == CTEX_RESULT_SUCCESS,
               "pass-through group state failed") &&
        expect(ctex_texture_set_layer_composite_cpu(document, set_id, 1, 1, content, 2, NULL, 0,
                                                    &info, &channel, 1, semantic, sizeof(semantic),
                                                    &passed, 1) == CTEX_RESULT_SUCCESS &&
                   fabsf(passed.x - 0.575F) < 1.0e-6F &&
                   memcmp(&isolated, &passed, sizeof(isolated)) != 0,
               "pass-through group collapsed into isolated composition");

    ctex_vec4f sentinel = {-1.0F, -1.0F, -1.0F, -1.0F};
    ok = ok && expect(ctex_texture_set_layer_composite_cpu(
                          document, set_id, 1, 1, content, 1, NULL, 0, &info, &channel, 1, semantic,
                          sizeof(semantic), &sentinel, 1) == CTEX_RESULT_INVALID_ARGUMENT &&
                          sentinel.x == -1.0F,
                      "missing content was accepted or partially published");
    ctex_document_destroy(document);
    return ok;
}

static int owned_snapshot_round_trips_and_composites(void) {
    ctex_document* document = NULL;
    ctex_layer_snapshot* snapshot = NULL;
    char set_id[128] = {0};
    if (!expect(create_fixture(&document, set_id, sizeof(set_id)),
                "snapshot fixture creation failed")) {
        ctex_document_destroy(document);
        return 0;
    }
    const ctex_layer_channel_descriptor base_channel = {
        .size = CTEX_LAYER_CHANNEL_DESCRIPTOR_CURRENT_SIZE,
        .semantic_id = "pbr.base_color",
        .enabled = 1,
        .opacity = 1.0,
    };
    ctex_layer_entry_descriptor entries[2] = {
        entry("paint", CTEX_LAYER_ENTRY_PAINT),
        entry("mask", CTEX_LAYER_ENTRY_MASK),
    };
    entries[0].channels = &base_channel;
    entries[0].channel_count = 1;
    entries[1].target_identifier = "paint";
    const ctex_vec4f source_pixel = {0.8F, 0.4F, 0.2F, 1.0F};
    const float source_coverage = 0.75F;
    const double source_mask = 0.5;
    const ctex_layer_composite_raster_descriptor content = {
        .size = CTEX_LAYER_COMPOSITE_RASTER_DESCRIPTOR_CURRENT_SIZE,
        .entry_identifier = "paint",
        .semantic_id = "pbr.base_color",
        .width = 1,
        .height = 1,
        .pixels = &source_pixel,
        .pixel_count = 1,
        .coverage = &source_coverage,
        .coverage_count = 1,
    };
    const ctex_layer_composite_mask_descriptor mask = {
        .size = CTEX_LAYER_COMPOSITE_MASK_DESCRIPTOR_CURRENT_SIZE,
        .mask_identifier = "mask",
        .width = 1,
        .height = 1,
        .values = &source_mask,
        .value_count = 1,
    };
    ctex_layer_snapshot_info snapshot_info = {.size = CTEX_LAYER_SNAPSHOT_INFO_CURRENT_SIZE};
    ctex_layer_snapshot_content_info content_info = {0};
    ctex_layer_snapshot_mask_info mask_info = {0};
    char strings[64] = {0};
    ctex_vec4f pixel = {0};
    float coverage = 0.0F;
    double mask_value = 0.0;
    int ok =
        expect(ctex_texture_set_layer_append(document, set_id, entries, 2) == CTEX_RESULT_SUCCESS,
               "snapshot stack append failed") &&
        expect(ctex_layer_snapshot_create(1, 1, &content, 1, &mask, 1, &snapshot) ==
                       CTEX_RESULT_SUCCESS &&
                   snapshot != NULL,
               "snapshot creation failed") &&
        expect(ctex_layer_snapshot_read(snapshot, &snapshot_info, NULL, 0, NULL, 0, NULL, 0, NULL,
                                        0, NULL, 0, NULL, 0) == CTEX_RESULT_SUCCESS &&
                   snapshot_info.width == 1 && snapshot_info.height == 1 &&
                   snapshot_info.content_count == 1 && snapshot_info.mask_count == 1 &&
                   snapshot_info.required_pixel_count == 1 &&
                   snapshot_info.required_coverage_count == 1 &&
                   snapshot_info.required_mask_value_count == 1,
               "snapshot sizing query failed") &&
        expect(ctex_layer_snapshot_read(snapshot, &snapshot_info, &content_info, 1, &mask_info, 1,
                                        strings, sizeof(strings), &pixel, 1, &coverage, 1,
                                        &mask_value, 1) == CTEX_RESULT_SUCCESS &&
                   strcmp(strings + content_info.entry_identifier_offset, "paint") == 0 &&
                   strcmp(strings + content_info.semantic_id_offset, "pbr.base_color") == 0 &&
                   strcmp(strings + mask_info.mask_identifier_offset, "mask") == 0 &&
                   memcmp(&pixel, &source_pixel, sizeof(pixel)) == 0 &&
                   coverage == source_coverage && mask_value == source_mask,
               "snapshot did not round-trip its owned arrays");

    ctex_layer_snapshot_content_info sentinel = {.width = 77};
    snapshot_info.width = 99;
    ok = ok && expect(ctex_layer_snapshot_read(snapshot, &snapshot_info, &sentinel, 1, NULL, 0,
                                               strings, 1, NULL, 0, NULL, 0, NULL,
                                               0) == CTEX_RESULT_BUFFER_TOO_SMALL &&
                          sentinel.width == 77 && snapshot_info.width == 99,
                      "short snapshot output partially published data");

    ctex_layer_composite_info composite_info = {.size = CTEX_LAYER_COMPOSITE_INFO_CURRENT_SIZE};
    ctex_layer_composite_channel_info channel = {0};
    char semantic[32] = {0};
    ctex_vec4f composite_pixel = {0};
    ok = ok && expect(ctex_texture_set_layer_composite_snapshot_cpu(
                          document, set_id, snapshot, &composite_info, &channel, 1, semantic,
                          sizeof(semantic), &composite_pixel, 1) == CTEX_RESULT_SUCCESS &&
                          composite_info.channel_count == 1 &&
                          strcmp(semantic, "pbr.base_color") == 0 && channel.pixel_count == 1,
                      "owned snapshot did not composite through the texture set");

    ctex_layer_snapshot_destroy(snapshot);
    ctex_document_destroy(document);
    return ok;
}

int main(void) {
    return cpu_composite_is_grouped_and_bit_deterministic() &&
                   owned_snapshot_round_trips_and_composites()
               ? 0
               : 1;
}
