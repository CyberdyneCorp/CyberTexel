#include <ctex/capi.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

typedef struct allocator_capture {
    size_t allocation_count;
    size_t deallocation_count;
    size_t allocation_size;
    size_t allocation_alignment;
    size_t deallocation_size;
    size_t deallocation_alignment;
    int fail_allocation;
} allocator_capture;

static void* capture_allocate(size_t size, size_t alignment, void* user_data) {
    allocator_capture* capture = (allocator_capture*)user_data;
    ++capture->allocation_count;
    capture->allocation_size = size;
    capture->allocation_alignment = alignment;
    if (capture->fail_allocation) {
        return NULL;
    }
    return malloc(size);
}

static void capture_deallocate(void* allocation, size_t size, size_t alignment, void* user_data) {
    allocator_capture* capture = (allocator_capture*)user_data;
    ++capture->deallocation_count;
    capture->deallocation_size = size;
    capture->deallocation_alignment = alignment;
    free(allocation);
}

static void* misaligned_allocate(size_t size, size_t alignment, void* user_data) {
    allocator_capture* capture = (allocator_capture*)user_data;
    unsigned char* allocation = NULL;
    ++capture->allocation_count;
    capture->allocation_size = size;
    capture->allocation_alignment = alignment;
    allocation = (unsigned char*)malloc(size + 1);
    return allocation == NULL ? NULL : allocation + 1;
}

static void misaligned_deallocate(void* allocation, size_t size, size_t alignment,
                                  void* user_data) {
    allocator_capture* capture = (allocator_capture*)user_data;
    ++capture->deallocation_count;
    capture->deallocation_size = size;
    capture->deallocation_alignment = alignment;
    free((unsigned char*)allocation - 1);
}

int main(void) {
    static const char cube_source[] =
        "LUT_3D_SIZE 2\n0 0 0\n1 0 0\n0 1 0\n1 1 0\n"
        "0 0 1\n1 0 1\n0 1 1\n1 1 1\n";
    allocator_capture capture = {0};
    ctex_allocator_descriptor allocator = {
        CTEX_ALLOCATOR_DESCRIPTOR_CURRENT_SIZE,
        capture_allocate,
        capture_deallocate,
        &capture,
    };
    ctex_document* document = NULL;
    ctex_cube_lut* lut = NULL;
    ctex_paint_dilation_session* dilation_session = NULL;
    ctex_paint_preview_session* preview_session = NULL;
    const double dilation_pixels[3] = {-1.0, 2.0, -1.0};
    const uint8_t dilation_coverage[3] = {0, 1, 0};
    const ctex_paint_dilation_tile_descriptor dilation_tile = {
        CTEX_PAINT_DILATION_TILE_DESCRIPTOR_CURRENT_SIZE,
        0,
        0,
        3,
        1,
        1,
        dilation_pixels,
        3,
        dilation_coverage,
        3,
    };
    ctex_texture_set_descriptor texture_set = {
        CTEX_TEXTURE_SET_DESCRIPTOR_CURRENT_SIZE,
        "Body",
        CTEX_PARTITION_SOURCE_MATERIAL,
        "material:body-with-a-persistent-allocator-key",
        "UV0",
        16,
        16,
        8,
    };
    size_t successful_allocation_count = 0;
    size_t document_allocation_count = 0;
    char texture_set_id[128] = {0};
    size_t texture_set_id_size = 0;
    size_t texture_set_count = 0;
    const uint8_t preview_pixel[3] = {10, 20, 30};
    uint8_t preview_coverage[16 * 16] = {0};
    ctex_paint_preview_info preview_info = {.size = CTEX_PAINT_PREVIEW_INFO_CURRENT_SIZE};

    if (ctex_set_allocator(&allocator) != CTEX_RESULT_SUCCESS ||
        ctex_document_create(&document) != CTEX_RESULT_SUCCESS || document == NULL ||
        capture.allocation_count != 1 || capture.deallocation_count != 0 ||
        capture.allocation_size == 0 || capture.allocation_alignment == 0) {
        return 1;
    }
    if (ctex_document_create_texture_set(document, &texture_set) != CTEX_RESULT_SUCCESS ||
        capture.allocation_count < 13) {
        return 2;
    }
    document_allocation_count = capture.allocation_count;
    if (ctex_cube_lut_create(cube_source, sizeof(cube_source) - 1, &lut) != CTEX_RESULT_SUCCESS ||
        lut == NULL || capture.allocation_count <= document_allocation_count) {
        return 2;
    }
    document_allocation_count = capture.allocation_count;
    if (ctex_paint_dilation_session_create(1, &dilation_session) != CTEX_RESULT_SUCCESS ||
        dilation_session == NULL ||
        ctex_paint_dilation_session_stage_tile(dilation_session, &dilation_tile) !=
            CTEX_RESULT_SUCCESS ||
        capture.allocation_count <= document_allocation_count) {
        return 2;
    }
    successful_allocation_count = capture.allocation_count;

    if (ctex_set_allocator(NULL) != CTEX_RESULT_SUCCESS) {
        return 3;
    }
    if (ctex_document_get_texture_set_ids(document, texture_set_id, sizeof(texture_set_id),
                                          &texture_set_id_size,
                                          &texture_set_count) != CTEX_RESULT_SUCCESS ||
        texture_set_count != 1 ||
        ctex_texture_set_set_channel_enabled(document, texture_set_id, "pbr.base_color", 1, 0) !=
            CTEX_RESULT_SUCCESS ||
        ctex_paint_preview_session_create(document, texture_set_id, "pbr.base_color",
                                          &preview_session) != CTEX_RESULT_SUCCESS ||
        ctex_paint_preview_session_write_pixel(preview_session, 0, 0, preview_pixel,
                                               sizeof(preview_pixel)) != CTEX_RESULT_SUCCESS ||
        ctex_paint_preview_session_finalize(preview_session, preview_coverage,
                                            sizeof(preview_coverage), UINT32_MAX,
                                            &preview_info) != CTEX_RESULT_SUCCESS ||
        capture.allocation_count <= successful_allocation_count) {
        return 4;
    }
    successful_allocation_count = capture.allocation_count;
    ctex_paint_preview_session_destroy(preview_session);
    ctex_document_destroy(document);
    ctex_cube_lut_destroy(lut);
    ctex_paint_dilation_session_destroy(dilation_session);
    if (capture.deallocation_count != successful_allocation_count) {
        return 5;
    }

    document = NULL;
    if (ctex_document_create(&document) != CTEX_RESULT_SUCCESS || document == NULL ||
        capture.allocation_count != successful_allocation_count) {
        return 6;
    }
    ctex_document_destroy(document);

    allocator.deallocate = NULL;
    if (ctex_set_allocator(&allocator) != CTEX_RESULT_INVALID_ARGUMENT ||
        ctex_get_last_diagnostic_code() != CTEX_DIAGNOSTIC_NULL_ARGUMENT) {
        return 7;
    }

    allocator.deallocate = capture_deallocate;
    capture.fail_allocation = 1;
    if (ctex_set_allocator(&allocator) != CTEX_RESULT_SUCCESS) {
        return 8;
    }
    document = (ctex_document*)(uintptr_t)1;
    if (ctex_document_create(&document) != CTEX_RESULT_OUT_OF_MEMORY || document != NULL ||
        ctex_get_last_diagnostic_code() != CTEX_DIAGNOSTIC_ALLOCATION_FAILED ||
        capture.allocation_count != successful_allocation_count + 1 ||
        capture.deallocation_count != successful_allocation_count) {
        return 9;
    }

    capture.fail_allocation = 0;
    allocator.allocate = misaligned_allocate;
    allocator.deallocate = misaligned_deallocate;
    if (ctex_set_allocator(&allocator) != CTEX_RESULT_SUCCESS) {
        return 10;
    }
    document = (ctex_document*)(uintptr_t)1;
    if (ctex_document_create(&document) != CTEX_RESULT_INVALID_ARGUMENT || document != NULL ||
        ctex_get_last_diagnostic_code() != CTEX_DIAGNOSTIC_ALLOCATOR_CONTRACT_VIOLATION ||
        capture.allocation_count != successful_allocation_count + 2 ||
        capture.deallocation_count != successful_allocation_count + 1 ||
        capture.deallocation_size != capture.allocation_size ||
        capture.deallocation_alignment != capture.allocation_alignment) {
        return 11;
    }

    if (ctex_set_allocator(NULL) != CTEX_RESULT_SUCCESS) {
        return 12;
    }
    return 0;
}
