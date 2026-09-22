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
        .display_name = "Layers",
        .partition_kind = CTEX_PARTITION_SOURCE_MATERIAL,
        .partition_key = "layers",
        .uv_set = "uv0",
        .width = 64,
        .height = 64,
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

static int inspect(const ctex_document* document, const char* set_id, char* output,
                   size_t output_size) {
    size_t required_size = 0;
    return ctex_texture_set_layer_inspect(document, set_id, output, output_size, &required_size) ==
               CTEX_RESULT_SUCCESS &&
           required_size <= output_size;
}

static int layer_stack_surface_is_atomic_and_explicit(void) {
    ctex_document* document = NULL;
    char set_id[128] = {0};
    if (!expect(create_fixture(&document, set_id, sizeof(set_id)),
                "layer fixture creation failed")) {
        ctex_document_destroy(document);
        return 0;
    }

    const ctex_layer_channel_descriptor base_channel = {
        .size = CTEX_LAYER_CHANNEL_DESCRIPTOR_CURRENT_SIZE,
        .semantic_id = "pbr.base_color",
        .enabled = 1,
        .opacity = 0.5,
    };
    ctex_layer_entry_descriptor entries[10] = {
        entry("group", CTEX_LAYER_ENTRY_GROUP),
        entry("paint", CTEX_LAYER_ENTRY_PAINT),
        entry("group-mask", CTEX_LAYER_ENTRY_MASK),
        entry("fill", CTEX_LAYER_ENTRY_FILL),
        entry("paint-filter", CTEX_LAYER_ENTRY_FILTER),
        entry("source", CTEX_LAYER_ENTRY_PAINT),
        entry("instance", CTEX_LAYER_ENTRY_INSTANCE),
        entry("decal", CTEX_LAYER_ENTRY_EDITABLE_DECAL),
        entry("text", CTEX_LAYER_ENTRY_EDITABLE_TEXT),
        entry("path", CTEX_LAYER_ENTRY_SURFACE_PATH),
    };
    entries[0].opacity = 0.5;
    entries[1].parent_identifier = "group";
    entries[1].channels = &base_channel;
    entries[1].channel_count = 1;
    entries[1].blend_mode = "multiply";
    entries[2].target_identifier = "group";
    entries[4].target_identifier = "paint";
    entries[5].channels = &base_channel;
    entries[5].channel_count = 1;
    entries[6].source_identifier = "source";
    entries[6].channels = &base_channel;
    entries[6].channel_count = 1;

    char before[4096] = {0};
    char after[4096] = {0};
    int passed =
        expect(ctex_texture_set_layer_append(document, set_id, entries, 10) == CTEX_RESULT_SUCCESS,
               "valid layer batch was rejected") &&
        expect(inspect(document, set_id, before, sizeof(before)), "layer inspection failed") &&
        expect(strstr(before, "\"kind\":\"paint\"") != NULL &&
                   strstr(before, "\"kind\":\"fill\"") != NULL &&
                   strstr(before, "\"kind\":\"group\"") != NULL &&
                   strstr(before, "\"kind\":\"mask\"") != NULL &&
                   strstr(before, "\"kind\":\"filter\"") != NULL &&
                   strstr(before, "\"kind\":\"instance\"") != NULL &&
                   strstr(before, "\"kind\":\"editable_decal\"") != NULL &&
                   strstr(before, "\"kind\":\"editable_text\"") != NULL &&
                   strstr(before, "\"kind\":\"surface_path\"") != NULL,
               "inspection omitted an explicit layer kind") &&
        expect(ctex_texture_set_layer_set_layout(document, set_id, "group", "group", NULL) ==
                   CTEX_RESULT_INVALID_ARGUMENT,
               "self-parenting group was accepted") &&
        expect(inspect(document, set_id, after, sizeof(after)) && strcmp(before, after) == 0,
               "invalid layout partially changed the stack") &&
        expect(ctex_texture_set_layer_set_state(document, set_id, "paint", "paint", 1, 1.0,
                                                "pass_through") == CTEX_RESULT_INVALID_ARGUMENT,
               "pass-through paint layer was accepted") &&
        expect(ctex_texture_set_layer_record_paint(document, set_id, "instance") ==
                       CTEX_RESULT_INVALID_ARGUMENT &&
                   strstr(ctex_get_last_diagnostic(), "source") != NULL,
               "direct instance paint did not name its source");

    const char* removed[] = {"source"};
    passed =
        passed &&
        expect(ctex_texture_set_layer_remove(document, set_id, removed, 1,
                                             CTEX_LAYER_SOURCE_DELETION_REFUSE) ==
                   CTEX_RESULT_INVALID_ARGUMENT,
               "live instance source deletion was not refused") &&
        expect(ctex_texture_set_layer_remove(
                   document, set_id, removed, 1,
                   CTEX_LAYER_SOURCE_DELETION_MAKE_INSTANCES_INDEPENDENT) == CTEX_RESULT_SUCCESS,
               "make-independent source deletion failed") &&
        expect(inspect(document, set_id, after, sizeof(after)) &&
                   strstr(after, "\"id\":\"instance\"") != NULL &&
                   strstr(after, "\"source\":\"\"") != NULL,
               "independent instance was not inspectable");

    ctex_document_destroy(document);
    return passed;
}

static int blend_and_participation_cross_the_boundary(void) {
    ctex_document* document = NULL;
    char set_id[128] = {0};
    if (!create_fixture(&document, set_id, sizeof(set_id))) {
        ctex_document_destroy(document);
        return 0;
    }
    const ctex_layer_channel_descriptor base_channel = {
        .size = CTEX_LAYER_CHANNEL_DESCRIPTOR_CURRENT_SIZE,
        .semantic_id = "pbr.base_color",
        .enabled = 1,
        .opacity = 0.5,
    };
    ctex_layer_entry_descriptor entries[3] = {
        entry("group", CTEX_LAYER_ENTRY_GROUP),
        entry("paint", CTEX_LAYER_ENTRY_PAINT),
        entry("mask", CTEX_LAYER_ENTRY_MASK),
    };
    entries[0].opacity = 0.5;
    entries[1].parent_identifier = "group";
    entries[1].blend_mode = "multiply";
    entries[1].channels = &base_channel;
    entries[1].channel_count = 1;
    entries[2].target_identifier = "group";
    ctex_vec4f colour = {0};
    ctex_layer_participation_info info = {.size = CTEX_LAYER_PARTICIPATION_INFO_CURRENT_SIZE};
    const ctex_layer_mask_sample sample = {
        .size = CTEX_LAYER_MASK_SAMPLE_CURRENT_SIZE,
        .mask_identifier = "mask",
        .value = 0.5,
    };
    char masks[32] = {0};
    size_t required_size = 0;
    size_t mask_count = 0;

    const int passed =
        expect(ctex_texture_set_layer_append(document, set_id, entries, 3) == CTEX_RESULT_SUCCESS,
               "participation fixture append failed") &&
        expect(ctex_texture_set_layer_get_applicable_masks(document, set_id, "paint", masks,
                                                           sizeof(masks), &required_size,
                                                           &mask_count) == CTEX_RESULT_SUCCESS &&
                   mask_count == 1 && strcmp(masks, "mask") == 0,
               "applicable mask discovery failed") &&
        expect(ctex_texture_set_layer_get_participation(document, set_id, "paint", "pbr.base_color",
                                                        &sample, 1, &info, masks,
                                                        sizeof(masks)) == CTEX_RESULT_SUCCESS &&
                   info.participates == 1 && info.mask_count == 1 &&
                   fabs(info.effective_opacity - 0.125) < 1.0e-12,
               "effective channel opacity was not flattened correctly") &&
        expect(ctex_texture_set_layer_get_participation(document, set_id, "paint", "pbr.base_color",
                                                        NULL, 0, &info, NULL,
                                                        0) == CTEX_RESULT_INVALID_ARGUMENT,
               "incomplete mask samples were accepted") &&
        expect(ctex_texture_set_layer_evaluate_blend(
                   document, set_id, "paint", (ctex_vec4f){0.25F, 0.5F, 0.75F, 1.0F},
                   (ctex_vec4f){0.8F, 0.4F, 0.2F, 1.0F}, 1.0, &colour) == CTEX_RESULT_SUCCESS &&
                   fabsf(colour.x - 0.2F) < 1.0e-6F && fabsf(colour.y - 0.2F) < 1.0e-6F &&
                   fabsf(colour.z - 0.15F) < 1.0e-6F,
               "multiply blend formula changed across the C boundary");
    ctex_document_destroy(document);
    return passed;
}

int main(void) {
    return layer_stack_surface_is_atomic_and_explicit() &&
                   blend_and_participation_cross_the_boundary()
               ? 0
               : 1;
}
