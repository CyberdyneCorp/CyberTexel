#include <ctex/capi.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int expect(int condition, const char* message) {
    if (!condition) fprintf(stderr, "%s: %s\n", message, ctex_get_last_diagnostic());
    return condition;
}

static void* make_record(const char* identifier, uint32_t replay_class, size_t* out_size) {
    static const double defaults[] = {0.5, 0.5, 0.5};
    static const char* checkpoint[] = {"checkpoint/base-color"};
    ctex_operation_channel_descriptor channel = {
        .size = CTEX_OPERATION_CHANNEL_DESCRIPTOR_CURRENT_SIZE,
        .semantic_id = "pbr.base_color",
        .component_count = 3,
        .scalar_representation = CTEX_SCALAR_REPRESENTATION_UNSIGNED_NORMALIZED,
        .bit_depth = 8,
        .color_space = CTEX_COLOR_SPACE_SRGB_REC709,
        .default_value = defaults,
        .default_value_count = 3,
    };
    ctex_operation_record_descriptor descriptor = {
        .size = CTEX_OPERATION_RECORD_DESCRIPTOR_CURRENT_SIZE,
        .identifier = identifier,
        .algorithm_identifier = "cybertexel.paint.brush",
        .algorithm_version = 3,
        .preset_identifier = "brushes/basic",
        .preset_version = 1,
        .replay_class = replay_class,
        .input_document_revision = 1,
        .seed = 7,
        .mesh_content_identity = "sha256:mesh",
        .coordinate_frame = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1},
        .payload_kind = CTEX_OPERATION_PAYLOAD_RESOLVED_STAMPS,
        .payload_version = 1,
        .channels = &channel,
        .channel_count = 1,
        .checkpoint_image_identifiers = checkpoint,
        .checkpoint_image_count = 1,
        .payload = "stamps",
        .payload_size = 6,
    };
    ctex_operation_record_info info = {.size = CTEX_OPERATION_RECORD_INFO_CURRENT_SIZE};
    if (ctex_operation_record_create(&descriptor, &info, NULL, 0, NULL, 0) != CTEX_RESULT_SUCCESS) {
        return NULL;
    }
    void* result = malloc(info.canonical_size);
    if (result == NULL ||
        ctex_operation_record_create(&descriptor, &info, result, info.canonical_size, NULL, 0) !=
            CTEX_RESULT_SUCCESS) {
        free(result);
        return NULL;
    }
    *out_size = info.canonical_size;
    return result;
}

static int create_document(ctex_document** out_document, char** out_id) {
    ctex_texture_set_descriptor descriptor = {
        .size = CTEX_TEXTURE_SET_DESCRIPTOR_CURRENT_SIZE,
        .display_name = "Body",
        .partition_kind = CTEX_PARTITION_SOURCE_MATERIAL,
        .partition_key = "body",
        .uv_set = "uv0",
        .width = 2,
        .height = 2,
        .default_bit_depth = 8,
    };
    size_t required_size = 0;
    size_t count = 0;
    if (ctex_document_create(out_document) != CTEX_RESULT_SUCCESS ||
        ctex_document_create_texture_set(*out_document, &descriptor) != CTEX_RESULT_SUCCESS ||
        ctex_document_get_texture_set_ids(*out_document, NULL, 0, &required_size, &count) !=
            CTEX_RESULT_SUCCESS ||
        count != 1) {
        return 0;
    }
    *out_id = (char*)malloc(required_size);
    return *out_id != NULL &&
           ctex_document_get_texture_set_ids(*out_document, *out_id, required_size, &required_size,
                                             &count) == CTEX_RESULT_SUCCESS &&
           ctex_texture_set_set_channel_enabled(*out_document, *out_id, "pbr.base_color", 1, 8) ==
               CTEX_RESULT_SUCCESS;
}

int main(void) {
    ctex_document* document = NULL;
    char* texture_set_id = NULL;
    int passed = expect(create_document(&document, &texture_set_id), "document creation failed");

    size_t eligible_size = 0;
    size_t checkpoint_size = 0;
    void* eligible = make_record("operations/stroke", CTEX_OPERATION_REPLAY_RESOLUTION_INDEPENDENT,
                                 &eligible_size);
    void* checkpoint = make_record("operations/checkpoint", CTEX_OPERATION_REPLAY_CHECKPOINT_ONLY,
                                   &checkpoint_size);
    if (eligible == NULL || checkpoint == NULL) return 1;
    ctex_resolution_operation_record_descriptor records[] = {
        {.size = CTEX_RESOLUTION_OPERATION_RECORD_DESCRIPTOR_CURRENT_SIZE,
         .canonical_record = eligible,
         .canonical_record_size = eligible_size},
        {.size = CTEX_RESOLUTION_OPERATION_RECORD_DESCRIPTOR_CURRENT_SIZE,
         .canonical_record = checkpoint,
         .canonical_record_size = checkpoint_size},
    };
    ctex_operation_algorithm_support_descriptor support = {
        .size = CTEX_OPERATION_ALGORITHM_SUPPORT_DESCRIPTOR_CURRENT_SIZE,
        .identifier = "cybertexel.paint.brush",
        .minimum_version = 3,
        .maximum_version = 3,
    };
    uint8_t replay_pixels[4 * 4 * 3] = {0};
    replay_pixels[sizeof(replay_pixels) - 3] = 255;
    ctex_resolution_replay_raster_descriptor raster = {
        .size = CTEX_RESOLUTION_REPLAY_RASTER_DESCRIPTOR_CURRENT_SIZE,
        .semantic_id = "pbr.base_color",
        .pixels = replay_pixels,
        .pixel_bytes = sizeof(replay_pixels),
    };
    ctex_texture_set_resolution_change_descriptor resize = {
        .size = CTEX_TEXTURE_SET_RESOLUTION_CHANGE_DESCRIPTOR_CURRENT_SIZE,
        .width = 4,
        .height = 4,
        .policy = CTEX_RESOLUTION_REPLAY_ELIGIBLE,
        .checkpoint_policy = CTEX_CHECKPOINT_RESAMPLE_REFUSE,
        .operation_records = records,
        .operation_record_count = 2,
        .supported_algorithms = &support,
        .supported_algorithm_count = 1,
        .replay_rasters = &raster,
        .replay_raster_count = 1,
    };
    ctex_texture_set_resolution_change_info info = {
        .size = CTEX_TEXTURE_SET_RESOLUTION_CHANGE_INFO_CURRENT_SIZE,
    };
    passed = passed &&
             expect(ctex_texture_set_change_resolution(document, texture_set_id, &resize, &info) ==
                        CTEX_RESULT_UNSUPPORTED_OPERATION,
                    "checkpoint fallback was accepted without an explicit filter");

    resize.checkpoint_policy = CTEX_CHECKPOINT_RESAMPLE_BILINEAR;
    info.size = CTEX_TEXTURE_SET_RESOLUTION_CHANGE_INFO_CURRENT_SIZE;
    passed = passed && expect(ctex_texture_set_change_resolution(document, texture_set_id, &resize,
                                                                 &info) == CTEX_RESULT_SUCCESS &&
                                  info.committed == 1 && info.source_width == 2 &&
                                  info.target_width == 4 && info.replayed_source_count == 1 &&
                                  info.resampled_source_count == 1 && info.raster_count == 1,
                              "mixed operation records were not resized atomically");

    ctex_texture_set_resolution_change_descriptor over_budget = {
        .size = CTEX_TEXTURE_SET_RESOLUTION_CHANGE_DESCRIPTOR_CURRENT_SIZE,
        .width = 8,
        .height = 8,
        .policy = CTEX_RESOLUTION_RESAMPLE_ALL,
        .checkpoint_policy = CTEX_CHECKPOINT_RESAMPLE_NEAREST,
        .maximum_working_bytes = 1,
    };
    info.size = CTEX_TEXTURE_SET_RESOLUTION_CHANGE_INFO_CURRENT_SIZE;
    passed =
        passed && expect(ctex_texture_set_change_resolution(document, texture_set_id, &over_budget,
                                                            &info) == CTEX_RESULT_OVER_BUDGET,
                         "over-budget resolution change was accepted");

    ctex_texture_set_resolution_restore_info restored = {
        .size = CTEX_TEXTURE_SET_RESOLUTION_RESTORE_INFO_CURRENT_SIZE,
    };
    passed = passed && expect(ctex_texture_set_undo_resolution_change(
                                  document, texture_set_id, &restored) == CTEX_RESULT_SUCCESS &&
                                  restored.width == 2 && restored.height == 2,
                              "resolution undo did not restore original dimensions");
    restored.size = CTEX_TEXTURE_SET_RESOLUTION_RESTORE_INFO_CURRENT_SIZE;
    passed = passed && expect(ctex_texture_set_redo_resolution_change(
                                  document, texture_set_id, &restored) == CTEX_RESULT_SUCCESS &&
                                  restored.width == 4 && restored.height == 4,
                              "resolution redo did not restore target dimensions");

    free(checkpoint);
    free(eligible);
    free(texture_set_id);
    ctex_document_destroy(document);
    return passed ? 0 : 1;
}
