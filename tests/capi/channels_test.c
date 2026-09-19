#include <ctex/capi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int expect(int condition, const char* message) {
    if (!condition) {
        fprintf(stderr, "%s\n", message);
    }
    return condition;
}

static int create_document_with_set(ctex_document** document, char** texture_set_id) {
    ctex_texture_set_descriptor descriptor = {
        .size = CTEX_TEXTURE_SET_DESCRIPTOR_CURRENT_SIZE,
        .display_name = "Body",
        .partition_kind = CTEX_PARTITION_SOURCE_MATERIAL,
        .partition_key = "body",
        .uv_set = "uv0",
        .width = 8,
        .height = 8,
        .default_bit_depth = 8,
    };
    if (ctex_document_create(document) != CTEX_RESULT_SUCCESS ||
        ctex_document_create_texture_set(*document, &descriptor) != CTEX_RESULT_SUCCESS) {
        return 0;
    }
    size_t required_size = 0;
    size_t count = 0;
    if (ctex_document_get_texture_set_ids(*document, NULL, 0, &required_size, &count) !=
            CTEX_RESULT_SUCCESS ||
        count != 1) {
        return 0;
    }
    *texture_set_id = (char*)malloc(required_size);
    return *texture_set_id != NULL &&
           ctex_document_get_texture_set_ids(*document, *texture_set_id, required_size,
                                             &required_size, &count) == CTEX_RESULT_SUCCESS;
}

static int built_in_channels_are_visible(ctex_document* document, const char* texture_set_id) {
    size_t required_size = 0;
    size_t count = 0;
    if (!expect(ctex_texture_set_get_channel_ids(document, texture_set_id, NULL, 0, &required_size,
                                                 &count) == CTEX_RESULT_SUCCESS,
                "channel sizing failed") ||
        !expect(count == 9, "built-in channel count is not nine")) {
        return 0;
    }
    char* identifiers = (char*)malloc(required_size);
    if (!expect(identifiers != NULL, "channel ID buffer allocation failed") ||
        !expect(
            ctex_texture_set_get_channel_ids(document, texture_set_id, identifiers, required_size,
                                             &required_size, &count) == CTEX_RESULT_SUCCESS,
            "channel enumeration failed")) {
        free(identifiers);
        return 0;
    }
    const int contains_base_color = strstr(identifiers, "pbr.base_color") != NULL;
    free(identifiers);

    ctex_channel_info info = {.size = CTEX_CHANNEL_INFO_CURRENT_SIZE};
    size_t mapping_size = 0;
    if (!expect(ctex_texture_set_get_channel_info(document, texture_set_id, "pbr.base_color", &info,
                                                  NULL, 0, &mapping_size) == CTEX_RESULT_SUCCESS,
                "built-in channel query failed")) {
        return 0;
    }
    char short_mapping[4] = {'x', 'x', 'x', 'x'};
    if (!expect(ctex_texture_set_get_channel_info(document, texture_set_id, "pbr.base_color", &info,
                                                  short_mapping, sizeof(short_mapping),
                                                  &mapping_size) == CTEX_RESULT_BUFFER_TOO_SMALL,
                "short export mapping buffer was accepted") ||
        !expect(memcmp(short_mapping, "xxxx", sizeof(short_mapping)) == 0,
                "short export mapping buffer was modified")) {
        return 0;
    }
    char mapping[16] = {0};
    return expect(contains_base_color, "base-colour channel was not enumerated") &&
           expect(mapping_size == strlen("baseColor") + 1, "wrong export mapping size") &&
           expect(ctex_texture_set_get_channel_info(document, texture_set_id, "pbr.base_color",
                                                    &info, mapping, sizeof(mapping),
                                                    &mapping_size) == CTEX_RESULT_SUCCESS,
                  "built-in channel detail query failed") &&
           expect(info.component_count == 3 && info.default_value_count == 3,
                  "base-colour shape changed") &&
           expect(info.enabled == 0 && info.storage_bit_depth == 0,
                  "built-in channel unexpectedly allocated storage") &&
           expect(strcmp(mapping, "baseColor") == 0, "base-colour export mapping changed");
}

static int subset_and_precision_are_reported(ctex_document* document, const char* texture_set_id) {
    if (!expect(ctex_texture_set_set_channel_enabled(document, texture_set_id, "pbr.base_color", 1,
                                                     0) == CTEX_RESULT_SUCCESS,
                "base-colour enable failed") ||
        !expect(ctex_texture_set_set_channel_enabled(document, texture_set_id, "pbr.height", 1,
                                                     16) == CTEX_RESULT_SUCCESS,
                "height precision override failed")) {
        return 0;
    }
    ctex_channel_info height = {.size = CTEX_CHANNEL_INFO_CURRENT_SIZE};
    size_t mapping_size = 0;
    ctex_texture_set_memory_report report = {
        .size = CTEX_TEXTURE_SET_MEMORY_REPORT_CURRENT_SIZE,
    };
    return expect(ctex_texture_set_get_channel_info(document, texture_set_id, "pbr.height", &height,
                                                    NULL, 0, &mapping_size) == CTEX_RESULT_SUCCESS,
                  "height query failed") &&
           expect(height.enabled == 1 && height.storage_bit_depth == 16,
                  "height precision override was not reported") &&
           expect(ctex_texture_set_get_memory_report(document, texture_set_id, &report) ==
                      CTEX_RESULT_SUCCESS,
                  "texture-set memory report failed") &&
           expect(report.enabled_channel_count == 2,
                  "enabled channel subset was not reflected in the memory report") &&
           expect(report.channel_pixel_bytes == 0 && report.total_resident_bytes == 0,
                  "constant channels allocated resident tiles");
}

static int custom_channel_round_trips(ctex_document* document, const char* texture_set_id) {
    ctex_channel_descriptor descriptor = {
        .size = CTEX_CHANNEL_DESCRIPTOR_CURRENT_SIZE,
        .semantic_id = "openpbr.coat_weight",
        .component_count = 1,
        .scalar_representation = CTEX_SCALAR_REPRESENTATION_UNSIGNED_NORMALIZED,
        .preferred_bit_depth = 8,
        .default_value = {0.25, 0.0, 0.0, 0.0},
        .default_value_count = 1,
        .classification = CTEX_CHANNEL_CLASSIFICATION_DATA,
        .blending_policy = CTEX_BLENDING_POLICY_SCALAR,
        .export_mapping = "coatWeight",
        .evaluable = 0,
    };
    if (!expect(ctex_texture_set_register_channel(document, texture_set_id, &descriptor) ==
                    CTEX_RESULT_SUCCESS,
                "custom channel registration failed")) {
        return 0;
    }
    ctex_channel_info info = {.size = CTEX_CHANNEL_INFO_CURRENT_SIZE};
    char mapping[16] = {0};
    size_t mapping_size = 0;
    const int queried = expect(ctex_texture_set_get_channel_info(
                                   document, texture_set_id, "openpbr.coat_weight", &info, mapping,
                                   sizeof(mapping), &mapping_size) == CTEX_RESULT_SUCCESS,
                               "custom channel query failed") &&
                        expect(info.component_count == 1 && info.default_value_count == 1 &&
                                   info.default_value[0] == 0.25 && info.evaluable == 0,
                               "custom channel descriptor changed") &&
                        expect(strcmp(mapping, "coatWeight") == 0, "custom export mapping changed");
    const ctex_result duplicate =
        ctex_texture_set_register_channel(document, texture_set_id, &descriptor);
    return queried &&
           expect(duplicate == CTEX_RESULT_INVALID_ARGUMENT &&
                      ctex_get_last_diagnostic_code() == CTEX_DIAGNOSTIC_DUPLICATE_CHANNEL,
                  "duplicate channel did not return its stable diagnostic");
}

int main(void) {
    ctex_document* document = NULL;
    char* texture_set_id = NULL;
    if (!expect(create_document_with_set(&document, &texture_set_id),
                "document fixture creation failed")) {
        free(texture_set_id);
        ctex_document_destroy(document);
        return 1;
    }
    const int passed = built_in_channels_are_visible(document, texture_set_id) &&
                       subset_and_precision_are_reported(document, texture_set_id) &&
                       custom_channel_round_trips(document, texture_set_id);
    free(texture_set_id);
    ctex_document_destroy(document);
    return passed ? 0 : 1;
}
