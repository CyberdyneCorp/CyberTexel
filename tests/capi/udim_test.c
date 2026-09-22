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

static int create_set(ctex_document** document, uint32_t descriptor_size, uint32_t udim_tiling,
                      char* identifier, size_t identifier_capacity) {
    ctex_texture_set_descriptor descriptor = {
        .size = descriptor_size,
        .display_name = "Material",
        .partition_kind = CTEX_PARTITION_SOURCE_MATERIAL,
        .partition_key = udim_tiling != 0 ? "udim" : "flat",
        .uv_set = "uv0",
        .width = 64,
        .height = 64,
        .default_bit_depth = 8,
        .udim_tiling = udim_tiling,
    };
    size_t required_size = 0;
    size_t count = 0;
    return ctex_document_create(document) == CTEX_RESULT_SUCCESS &&
           ctex_document_create_texture_set(*document, &descriptor) == CTEX_RESULT_SUCCESS &&
           ctex_document_get_texture_set_ids(*document, NULL, 0, &required_size, &count) ==
               CTEX_RESULT_SUCCESS &&
           expect(count == 1 && required_size <= identifier_capacity,
                  "texture-set identity sizing failed") &&
           ctex_document_get_texture_set_ids(*document, identifier, identifier_capacity,
                                             &required_size, &count) == CTEX_RESULT_SUCCESS;
}

static int cross_border_write_is_sparse_and_readable(void) {
    ctex_document* document = NULL;
    char identifier[128] = {0};
    if (!expect(create_set(&document, CTEX_TEXTURE_SET_DESCRIPTOR_CURRENT_SIZE, 1, identifier,
                           sizeof(identifier)),
                "UDIM texture-set creation failed") ||
        !expect(ctex_texture_set_set_channel_enabled(document, identifier, "pbr.base_color", 1,
                                                     0) == CTEX_RESULT_SUCCESS,
                "UDIM channel enable failed")) {
        ctex_document_destroy(document);
        return 0;
    }

    const uint8_t left[3] = {1, 2, 3};
    const uint8_t right[3] = {4, 5, 6};
    const ctex_udim_pixel_write_descriptor writes[2] = {
        {.size = CTEX_UDIM_PIXEL_WRITE_DESCRIPTOR_CURRENT_SIZE,
         .u = 0.999,
         .v = 0.5,
         .pixel = left,
         .pixel_size = sizeof(left)},
        {.size = CTEX_UDIM_PIXEL_WRITE_DESCRIPTOR_CURRENT_SIZE,
         .u = 1.001,
         .v = 0.5,
         .pixel = right,
         .pixel_size = sizeof(right)},
    };
    ctex_udim_write_info info = {.size = CTEX_UDIM_WRITE_INFO_CURRENT_SIZE};
    size_t tile_count = 0;
    uint32_t tiles[2] = {0, 0};
    size_t required_pixel_size = 0;
    uint8_t pixel[3] = {0, 0, 0};

    const int passed =
        expect(ctex_texture_set_write_udim_pixels(document, identifier, "pbr.base_color", writes, 2,
                                                  &info) == CTEX_RESULT_SUCCESS,
               "cross-border UDIM write failed") &&
        expect(info.changed_tile_count == 2 && info.allocated_tile_count == 2 &&
                   info.changed_pixel_count == 2,
               "cross-border UDIM write report is wrong") &&
        expect(ctex_texture_set_get_udim_tiles(document, identifier, NULL, 0, &tile_count) ==
                       CTEX_RESULT_SUCCESS &&
                   tile_count == 2,
               "UDIM tile sizing query failed") &&
        expect(ctex_texture_set_get_udim_tiles(document, identifier, tiles, 1, &tile_count) ==
                       CTEX_RESULT_BUFFER_TOO_SMALL &&
                   tiles[0] == 0,
               "short UDIM tile buffer was accepted or modified") &&
        expect(ctex_texture_set_get_udim_tiles(document, identifier, tiles, 2, &tile_count) ==
                       CTEX_RESULT_SUCCESS &&
                   tiles[0] == 1001 && tiles[1] == 1002,
               "occupied UDIM tiles were not returned in order") &&
        expect(ctex_texture_set_read_udim_pixel(document, identifier, "pbr.base_color", 1002, 0, 32,
                                                NULL, 0,
                                                &required_pixel_size) == CTEX_RESULT_SUCCESS &&
                   required_pixel_size == sizeof(pixel),
               "UDIM pixel sizing query failed") &&
        expect(ctex_texture_set_read_udim_pixel(document, identifier, "pbr.base_color", 1002, 0, 32,
                                                pixel, sizeof(pixel),
                                                &required_pixel_size) == CTEX_RESULT_SUCCESS &&
                   memcmp(pixel, right, sizeof(pixel)) == 0,
               "UDIM pixel did not round-trip");
    ctex_document_destroy(document);
    return passed;
}

static int invalid_batch_is_atomic(void) {
    ctex_document* document = NULL;
    char identifier[128] = {0};
    if (!create_set(&document, CTEX_TEXTURE_SET_DESCRIPTOR_CURRENT_SIZE, 1, identifier,
                    sizeof(identifier)) ||
        ctex_texture_set_set_channel_enabled(document, identifier, "pbr.base_color", 1, 0) !=
            CTEX_RESULT_SUCCESS) {
        ctex_document_destroy(document);
        return 0;
    }
    const uint8_t pixel[3] = {1, 2, 3};
    const ctex_udim_pixel_write_descriptor writes[2] = {
        {.size = CTEX_UDIM_PIXEL_WRITE_DESCRIPTOR_CURRENT_SIZE,
         .u = 0.5,
         .v = 0.5,
         .pixel = pixel,
         .pixel_size = sizeof(pixel)},
        {.size = CTEX_UDIM_PIXEL_WRITE_DESCRIPTOR_CURRENT_SIZE,
         .u = 10.0,
         .v = 0.5,
         .pixel = pixel,
         .pixel_size = sizeof(pixel)},
    };
    ctex_udim_write_info info = {.size = CTEX_UDIM_WRITE_INFO_CURRENT_SIZE};
    size_t tile_count = 99;
    const int passed =
        expect(ctex_texture_set_write_udim_pixels(document, identifier, "pbr.base_color", writes, 2,
                                                  &info) == CTEX_RESULT_INVALID_ARGUMENT,
               "invalid UDIM coordinate was accepted") &&
        expect(ctex_texture_set_get_udim_tiles(document, identifier, NULL, 0, &tile_count) ==
                       CTEX_RESULT_SUCCESS &&
                   tile_count == 0,
               "invalid UDIM batch partially allocated a tile");
    ctex_document_destroy(document);
    return passed;
}

static int older_descriptor_defaults_to_flat_storage(void) {
    ctex_document* document = NULL;
    char identifier[128] = {0};
    size_t tile_count = 0;
    const int passed =
        expect(create_set(&document, CTEX_TEXTURE_SET_DESCRIPTOR_V2_SIZE, 1, identifier,
                          sizeof(identifier)),
               "older texture-set descriptor was rejected") &&
        expect(ctex_texture_set_get_udim_tiles(document, identifier, NULL, 0, &tile_count) ==
                   CTEX_RESULT_UNSUPPORTED_OPERATION,
               "older texture-set descriptor did not default UDIM tiling off");
    ctex_document_destroy(document);
    return passed;
}

int main(void) {
    return cross_border_write_is_sparse_and_readable() && invalid_batch_is_atomic() &&
                   older_descriptor_defaults_to_flat_storage()
               ? 0
               : 1;
}
