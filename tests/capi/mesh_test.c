#include <ctex/capi.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

typedef struct allocation_counts {
    size_t allocations;
    size_t deallocations;
} allocation_counts;

static void* count_allocate(size_t size, size_t alignment, void* user_data) {
    allocation_counts* counts = (allocation_counts*)user_data;
    (void)alignment;
    ++counts->allocations;
    return malloc(size);
}

static void count_deallocate(void* allocation, size_t size, size_t alignment, void* user_data) {
    allocation_counts* counts = (allocation_counts*)user_data;
    (void)size;
    (void)alignment;
    ++counts->deallocations;
    free(allocation);
}

static int expect(int condition) { return condition ? 1 : 0; }

int main(void) {
    ctex_vec3f positions[] = {{0.0F, 0.0F, 0.0F}, {1.0F, 0.0F, 0.0F}, {0.0F, 1.0F, 0.0F}};
    const ctex_vec3f positions_before[] = {
        {0.0F, 0.0F, 0.0F}, {1.0F, 0.0F, 0.0F}, {0.0F, 1.0F, 0.0F}};
    ctex_vec3f normals[] = {{0.0F, 0.0F, 1.0F}, {0.0F, 0.0F, 1.0F}, {0.0F, 0.0F, 1.0F}};
    ctex_vec2f uv_values[] = {{0.0F, 0.0F}, {1.0F, 0.0F}, {0.0F, 1.0F}};
    uint32_t indices[] = {0, 1, 2};
    uint32_t face_partitions[] = {0};
    uint32_t face_materials[] = {7};
    char first_uv_name[] = "uv0";
    ctex_uv_set_descriptor uv_sets[] = {
        {CTEX_UV_SET_DESCRIPTOR_CURRENT_SIZE, first_uv_name, uv_values, 3},
        {CTEX_UV_SET_DESCRIPTOR_CURRENT_SIZE, "uv1", uv_values, 3},
        {CTEX_UV_SET_DESCRIPTOR_CURRENT_SIZE, "uv2", uv_values, 3},
        {CTEX_UV_SET_DESCRIPTOR_CURRENT_SIZE, "uv3", uv_values, 3},
    };
    ctex_mesh_partition_descriptor partitions[] = {{
        CTEX_MESH_PARTITION_DESCRIPTOR_CURRENT_SIZE,
        CTEX_PARTITION_SOURCE_MATERIAL,
        "body",
        "Body",
    }};
    ctex_mesh_descriptor descriptor = {
        CTEX_MESH_DESCRIPTOR_CURRENT_SIZE,
        positions,
        3,
        normals,
        3,
        NULL,
        0,
        indices,
        3,
        uv_sets,
        4,
        "uv0",
        partitions,
        1,
        face_partitions,
        1,
        face_materials,
        1,
    };
    allocation_counts counts = {0};
    ctex_allocator_descriptor allocator = {CTEX_ALLOCATOR_DESCRIPTOR_CURRENT_SIZE, count_allocate,
                                           count_deallocate, &counts};
    ctex_mesh* mesh = NULL;
    ctex_mesh_info initial = {.size = CTEX_MESH_INFO_CURRENT_SIZE};
    ctex_mesh_info replaced = {.size = CTEX_MESH_INFO_CURRENT_SIZE};
    ctex_mesh_info after_rejection = {.size = CTEX_MESH_INFO_CURRENT_SIZE};
    size_t required_size = 0;
    size_t uv_set_count = 0;
    char names[16] = {0};

    if (!expect(ctex_set_allocator(&allocator) == CTEX_RESULT_SUCCESS) ||
        !expect(ctex_mesh_create(&descriptor, &mesh) == CTEX_RESULT_SUCCESS && mesh != NULL) ||
        !expect(memcmp(positions, positions_before, sizeof(positions)) == 0) ||
        !expect(ctex_mesh_get_info(mesh, &initial) == CTEX_RESULT_SUCCESS) ||
        !expect(initial.vertex_count == 3 && initial.triangle_count == 1 &&
                initial.uv_set_count == 4 && initial.partition_count == 1 &&
                initial.has_vertex_colors == 0 && initial.revision != 0)) {
        return 1;
    }

    first_uv_name[0] = 'X';
    if (!expect(ctex_mesh_get_uv_set_names(mesh, NULL, 0, &required_size, &uv_set_count) ==
                CTEX_RESULT_SUCCESS) ||
        !expect(required_size == 16 && uv_set_count == 4) ||
        !expect(ctex_mesh_get_uv_set_names(mesh, names, sizeof(names), &required_size,
                                           &uv_set_count) == CTEX_RESULT_SUCCESS) ||
        !expect(memcmp(names, "uv0\0uv1\0uv2\0uv3\0", sizeof(names)) == 0)) {
        return 2;
    }
    first_uv_name[0] = 'u';

    positions[0].x = 2.0F;
    if (!expect(ctex_mesh_replace(mesh, &descriptor) == CTEX_RESULT_SUCCESS) ||
        !expect(ctex_mesh_get_info(mesh, &replaced) == CTEX_RESULT_SUCCESS) ||
        !expect(replaced.revision > initial.revision)) {
        return 3;
    }

    descriptor.default_uv_set = "missing";
    if (!expect(ctex_mesh_replace(mesh, &descriptor) == CTEX_RESULT_INVALID_ARGUMENT) ||
        !expect(ctex_get_last_diagnostic_code() == CTEX_DIAGNOSTIC_INVALID_MESH) ||
        !expect(ctex_mesh_get_info(mesh, &after_rejection) == CTEX_RESULT_SUCCESS) ||
        !expect(after_rejection.revision == replaced.revision)) {
        return 4;
    }

    descriptor.default_uv_set = "uv0";
    descriptor.position_count = CTEX_MAX_MESH_VERTEX_COUNT + 1;
    if (!expect(ctex_mesh_replace(mesh, &descriptor) == CTEX_RESULT_OVER_BUDGET) ||
        !expect(ctex_get_last_diagnostic_code() == CTEX_DIAGNOSTIC_MESH_LIMIT_EXCEEDED) ||
        !expect(strstr(ctex_get_last_diagnostic(), "vertex_count supplied=100000001") != NULL) ||
        !expect(strstr(ctex_get_last_diagnostic(), "maximum=100000000") != NULL)) {
        return 5;
    }

    if (!expect(ctex_set_allocator(NULL) == CTEX_RESULT_SUCCESS)) {
        return 6;
    }
    ctex_mesh_destroy(mesh);
    if (!expect(counts.allocations > 1 && counts.allocations == counts.deallocations)) {
        return 7;
    }
    return 0;
}
