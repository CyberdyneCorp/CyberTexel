#include <ctex/capi.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

typedef struct backing_fixture {
    ctex_tile_backing_key key;
    uint8_t bytes[64 * 64 * 3];
    size_t byte_count;
    uint32_t fail_store;
    size_t stores;
    size_t loads;
    size_t discards;
    size_t releases;
} backing_fixture;

static int expect(int condition, const char* message) {
    if (!condition) {
        fprintf(stderr, "%s\n", message);
    }
    return condition;
}

static int key_equal(ctex_tile_backing_key left, ctex_tile_backing_key right) {
    return left.namespace_identity == right.namespace_identity && left.tile_x == right.tile_x &&
           left.tile_y == right.tile_y && left.generation == right.generation;
}

static uint32_t store_tile(ctex_tile_backing_key key, const void* bytes, size_t byte_count,
                           void* user_data) {
    backing_fixture* fixture = (backing_fixture*)user_data;
    ++fixture->stores;
    if (fixture->fail_store || byte_count > sizeof(fixture->bytes)) {
        return 0;
    }
    fixture->key = key;
    fixture->byte_count = byte_count;
    memcpy(fixture->bytes, bytes, byte_count);
    return 1;
}

static uint32_t load_tile(ctex_tile_backing_key key, void* bytes, size_t byte_count,
                          void* user_data) {
    backing_fixture* fixture = (backing_fixture*)user_data;
    ++fixture->loads;
    if (!key_equal(key, fixture->key) || byte_count != fixture->byte_count) {
        return 0;
    }
    memcpy(bytes, fixture->bytes, byte_count);
    return 1;
}

static void discard_tile(ctex_tile_backing_key key, void* user_data) {
    backing_fixture* fixture = (backing_fixture*)user_data;
    if (key_equal(key, fixture->key)) {
        ++fixture->discards;
        fixture->byte_count = 0;
    }
}

static void release_namespace(uint64_t namespace_identity, void* user_data) {
    backing_fixture* fixture = (backing_fixture*)user_data;
    if (namespace_identity == fixture->key.namespace_identity) {
        ++fixture->releases;
        fixture->byte_count = 0;
    }
}

int main(void) {
    ctex_document* document = NULL;
    ctex_texture_set_descriptor set = {
        .size = CTEX_TEXTURE_SET_DESCRIPTOR_CURRENT_SIZE,
        .display_name = "Backed UDIM",
        .partition_kind = CTEX_PARTITION_SOURCE_MATERIAL,
        .partition_key = "backed",
        .uv_set = "uv0",
        .width = 64,
        .height = 64,
        .default_bit_depth = 8,
        .udim_tiling = 1,
    };
    char set_id[128] = {0};
    size_t required = 0;
    size_t count = 0;
    uint32_t udim = 1001;
    const uint8_t authored[3] = {17, 29, 43};
    ctex_udim_pixel_write_descriptor write = {
        .size = CTEX_UDIM_PIXEL_WRITE_DESCRIPTOR_CURRENT_SIZE,
        .u = 0.25,
        .v = 0.25,
        .pixel = authored,
        .pixel_size = sizeof(authored),
    };
    ctex_udim_write_info write_info = {.size = CTEX_UDIM_WRITE_INFO_CURRENT_SIZE};
    if (!expect(ctex_document_create(&document) == CTEX_RESULT_SUCCESS &&
                    ctex_document_create_texture_set(document, &set) == CTEX_RESULT_SUCCESS &&
                    ctex_document_get_texture_set_ids(document, NULL, 0, &required, &count) ==
                        CTEX_RESULT_SUCCESS &&
                    required <= sizeof(set_id) && count == 1 &&
                    ctex_document_get_texture_set_ids(document, set_id, sizeof(set_id), &required,
                                                      &count) == CTEX_RESULT_SUCCESS &&
                    ctex_texture_set_set_channel_enabled(document, set_id, "pbr.base_color", 1,
                                                         0) == CTEX_RESULT_SUCCESS &&
                    ctex_texture_set_ensure_udim_tiles(document, set_id, &udim, 1, &count) ==
                        CTEX_RESULT_SUCCESS &&
                    ctex_texture_set_write_udim_pixels(document, set_id, "pbr.base_color", &write,
                                                       1, &write_info) == CTEX_RESULT_SUCCESS,
                "backed UDIM fixture creation failed")) {
        ctex_document_destroy(document);
        return 1;
    }

    backing_fixture fixture = {0};
    ctex_tile_backing_store_descriptor backing = {
        .size = CTEX_TILE_BACKING_STORE_DESCRIPTOR_CURRENT_SIZE,
        .store = store_tile,
        .load = load_tile,
        .discard = discard_tile,
        .release_namespace = release_namespace,
        .user_data = &fixture,
    };
    ctex_tile_eviction_report report = {.size = CTEX_TILE_EVICTION_REPORT_CURRENT_SIZE};
    fixture.fail_store = 1;
    int passed =
        expect(ctex_texture_set_set_channel_backing_store(document, set_id, "pbr.base_color", udim,
                                                          &backing) == CTEX_RESULT_SUCCESS,
               "UDIM backing-store registration failed") &&
        expect(ctex_texture_set_evict_channel_tile(document, set_id, "pbr.base_color", udim, 0, 0,
                                                   &report) == CTEX_RESULT_SUCCESS &&
                   report.status == CTEX_TILE_BACKING_STORE_FAILED &&
                   report.resident_pixel_bytes == sizeof(fixture.bytes),
               "failed backing write evicted authored content");
    fixture.fail_store = 0;
    passed = passed &&
             expect(ctex_texture_set_evict_channel_tile(document, set_id, "pbr.base_color", udim, 0,
                                                        0, &report) == CTEX_RESULT_SUCCESS &&
                        report.status == CTEX_TILE_EVICTED &&
                        report.resident_bytes_released == sizeof(fixture.bytes) &&
                        report.backing_bytes_written == sizeof(fixture.bytes) &&
                        report.resident_pixel_bytes == 0 &&
                        report.backed_pixel_bytes == sizeof(fixture.bytes),
                    "lossless UDIM tile eviction report is wrong");
    uint8_t restored[3] = {0, 0, 0};
    passed = passed &&
             expect(ctex_texture_set_read_udim_pixel(document, set_id, "pbr.base_color", udim, 16,
                                                     16, restored, sizeof(restored),
                                                     &required) == CTEX_RESULT_SUCCESS &&
                        memcmp(restored, authored, sizeof(restored)) == 0 && fixture.loads == 1,
                    "evicted UDIM tile did not reload bit-identically");
    ctex_document_destroy(document);
    return passed && expect(fixture.releases == 1 && fixture.byte_count == 0,
                            "document destruction leaked the backing namespace")
               ? 0
               : 1;
}
