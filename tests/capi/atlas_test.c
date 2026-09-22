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

static int add_set(ctex_document* document, const char* name, const char* key) {
    const ctex_texture_set_descriptor descriptor = {
        .size = CTEX_TEXTURE_SET_DESCRIPTOR_CURRENT_SIZE,
        .display_name = name,
        .partition_kind = CTEX_PARTITION_SOURCE_MATERIAL,
        .partition_key = key,
        .uv_set = "uv0",
        .width = 64,
        .height = 64,
        .default_bit_depth = 8,
    };
    return ctex_document_create_texture_set(document, &descriptor) == CTEX_RESULT_SUCCESS;
}

static int get_set_ids(const ctex_document* document, char* first, size_t first_size, char* second,
                       size_t second_size) {
    char packed[256] = {0};
    size_t required_size = 0;
    size_t count = 0;
    if (ctex_document_get_texture_set_ids(document, packed, sizeof(packed), &required_size,
                                          &count) != CTEX_RESULT_SUCCESS ||
        count != 2) {
        return 0;
    }
    const char* next = packed + strlen(packed) + 1;
    if (strlen(packed) + 1 > first_size || strlen(next) + 1 > second_size) {
        return 0;
    }
    memcpy(first, packed, strlen(packed) + 1);
    memcpy(second, next, strlen(next) + 1);
    return 1;
}

static int atlas_round_trips_and_refuses_overlap(void) {
    ctex_document* document = NULL;
    char first[128] = {0};
    char second[128] = {0};
    if (!expect(ctex_document_create(&document) == CTEX_RESULT_SUCCESS &&
                    add_set(document, "Crate", "crate") && add_set(document, "Barrel", "barrel") &&
                    get_set_ids(document, first, sizeof(first), second, sizeof(second)),
                "atlas texture-set fixture creation failed")) {
        ctex_document_destroy(document);
        return 0;
    }

    const ctex_atlas_region_descriptor regions[2] = {
        {.size = CTEX_ATLAS_REGION_DESCRIPTOR_CURRENT_SIZE,
         .texture_set_id = first,
         .x = 0,
         .y = 0,
         .width = 2,
         .height = 2},
        {.size = CTEX_ATLAS_REGION_DESCRIPTOR_CURRENT_SIZE,
         .texture_set_id = second,
         .x = 2,
         .y = 0,
         .width = 2,
         .height = 2},
    };
    const ctex_atlas_descriptor descriptor = {
        .size = CTEX_ATLAS_DESCRIPTOR_CURRENT_SIZE,
        .identifier = "props",
        .display_name = "Props",
        .width = 4,
        .height = 2,
        .regions = regions,
        .region_count = 2,
    };
    ctex_atlas_info info = {.size = CTEX_ATLAS_INFO_CURRENT_SIZE};
    size_t atlas_id_size = 0;
    size_t atlas_count = 0;
    ctex_atlas_region output_regions[2] = {{0}};
    char display_name[16] = {0};
    char texture_set_ids[256] = {0};

    int passed =
        expect(ctex_document_create_atlas(document, &descriptor) == CTEX_RESULT_SUCCESS,
               "valid atlas creation failed") &&
        expect(ctex_document_get_atlas_ids(document, NULL, 0, &atlas_id_size, &atlas_count) ==
                       CTEX_RESULT_SUCCESS &&
                   atlas_count == 1 && atlas_id_size == strlen("props") + 1,
               "atlas identity sizing failed") &&
        expect(ctex_document_get_atlas(document, "props", &info, NULL, 0, NULL, 0, NULL, 0) ==
                       CTEX_RESULT_SUCCESS &&
                   info.width == 4 && info.height == 2 && info.region_count == 2,
               "atlas sizing query failed") &&
        expect(ctex_document_get_atlas(document, "props", &info, output_regions, 1, display_name,
                                       sizeof(display_name), texture_set_ids,
                                       sizeof(texture_set_ids)) == CTEX_RESULT_BUFFER_TOO_SMALL &&
                   output_regions[0].texture_set_id_size == 0 && display_name[0] == 0 &&
                   texture_set_ids[0] == 0,
               "short atlas region buffer was accepted or partially written") &&
        expect(ctex_document_get_atlas(document, "props", &info, output_regions, 2, display_name,
                                       sizeof(display_name), texture_set_ids,
                                       sizeof(texture_set_ids)) == CTEX_RESULT_SUCCESS,
               "atlas detail query failed") &&
        expect(strcmp(display_name, "Props") == 0 &&
                   strcmp(texture_set_ids + output_regions[0].texture_set_id_offset, first) == 0 &&
                   strcmp(texture_set_ids + output_regions[1].texture_set_id_offset, second) == 0 &&
                   output_regions[1].x == 2 && output_regions[1].width == 2,
               "atlas regions or packed identities changed");

    ctex_atlas_region_descriptor overlapping[2] = {regions[0], regions[1]};
    overlapping[1].x = 1;
    ctex_atlas_descriptor invalid = descriptor;
    invalid.identifier = "overlap";
    invalid.regions = overlapping;
    passed = passed &&
             expect(ctex_document_create_atlas(document, &invalid) == CTEX_RESULT_INVALID_ARGUMENT,
                    "overlapping atlas regions were accepted") &&
             expect(ctex_document_get_atlas_ids(document, NULL, 0, &atlas_id_size, &atlas_count) ==
                            CTEX_RESULT_SUCCESS &&
                        atlas_count == 1,
                    "invalid atlas partially changed the document");

    ctex_document_destroy(document);
    return passed;
}

int main(void) { return atlas_round_trips_and_refuses_overlap() ? 0 : 1; }
