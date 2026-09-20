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
    ctex_mesh_replacement_plan* replacement_plan = NULL;
    ctex_mesh_replacement_plan* stale_plan = NULL;
    ctex_pick_index* pick_index = NULL;
    ctex_uv_pick_index* uv_pick_index = NULL;
    ctex_document* document = NULL;
    ctex_mesh_info initial = {.size = CTEX_MESH_INFO_CURRENT_SIZE};
    ctex_mesh_info replaced = {.size = CTEX_MESH_INFO_CURRENT_SIZE};
    ctex_mesh_info after_rejection = {.size = CTEX_MESH_INFO_CURRENT_SIZE};
    size_t required_size = 0;
    size_t uv_set_count = 0;
    char names[16] = {0};
    char texture_set_ids[64] = {0};
    size_t texture_set_ids_size = 0;
    size_t texture_set_count = 0;
    ctex_mesh_replacement_plan_info plan_info = {.size =
                                                     CTEX_MESH_REPLACEMENT_PLAN_INFO_CURRENT_SIZE};
    ctex_mesh_replacement_entry replacement_entries[1] = {0};
    char replacement_ids[64] = {0};
    ctex_mesh_replacement_apply_info apply_info = {
        .size = CTEX_MESH_REPLACEMENT_APPLY_INFO_CURRENT_SIZE};

    if (!expect(ctex_set_allocator(&allocator) == CTEX_RESULT_SUCCESS) ||
        !expect(ctex_mesh_create(&descriptor, &mesh) == CTEX_RESULT_SUCCESS && mesh != NULL) ||
        !expect(ctex_pick_index_create(mesh, &pick_index) == CTEX_RESULT_SUCCESS &&
                pick_index != NULL) ||
        !expect(ctex_uv_pick_index_create(mesh, "uv0", &uv_pick_index) == CTEX_RESULT_SUCCESS &&
                uv_pick_index != NULL) ||
        !expect(ctex_document_create(&document) == CTEX_RESULT_SUCCESS && document != NULL) ||
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

    if (!expect(ctex_document_create_texture_sets_from_mesh(document, mesh, "missing", 1024, 512,
                                                            16) == CTEX_RESULT_MISSING_RESOURCE) ||
        !expect(ctex_get_last_diagnostic_code() == CTEX_DIAGNOSTIC_MISSING_UV_SET) ||
        !expect(ctex_document_get_texture_set_ids(document, NULL, 0, &texture_set_ids_size,
                                                  &texture_set_count) == CTEX_RESULT_SUCCESS) ||
        !expect(texture_set_count == 0) ||
        !expect(ctex_document_create_texture_sets_from_mesh(document, mesh, "uv1", 1024, 512, 16) ==
                CTEX_RESULT_SUCCESS) ||
        !expect(ctex_document_get_texture_set_ids(document, texture_set_ids,
                                                  sizeof(texture_set_ids), &texture_set_ids_size,
                                                  &texture_set_count) == CTEX_RESULT_SUCCESS) ||
        !expect(texture_set_count == 1) ||
        !expect(strcmp(texture_set_ids, "material/4:body/uv/3:uv1") == 0)) {
        return 3;
    }

    uv_values[0].x = 0.25F;
    if (!expect(ctex_mesh_replacement_plan_create(document, mesh, &descriptor, &replacement_plan) ==
                CTEX_RESULT_SUCCESS) ||
        !expect(replacement_plan != NULL) ||
        !expect(ctex_mesh_replacement_plan_get_info(replacement_plan, &plan_info, NULL, 0, NULL,
                                                    0) == CTEX_RESULT_SUCCESS) ||
        !expect(plan_info.texture_set_count == 1 &&
                plan_info.required_texture_set_id_size == strlen(texture_set_ids) + 1) ||
        !expect(ctex_mesh_replacement_plan_get_info(
                    replacement_plan, &plan_info, replacement_entries, 1, replacement_ids,
                    sizeof(replacement_ids)) == CTEX_RESULT_SUCCESS) ||
        !expect(plan_info.source_mesh_revision == initial.revision &&
                plan_info.texture_set_count == 1 && plan_info.changed_texture_set_count == 1) ||
        !expect(replacement_entries[0].uv_change == CTEX_MESH_UV_CHANGED &&
                replacement_entries[0].source_partition_index == 0 &&
                replacement_entries[0].replacement_partition_index == 0 &&
                strcmp(replacement_ids + replacement_entries[0].texture_set_id_offset,
                       texture_set_ids) == 0)) {
        return 4;
    }
    ctex_mesh_replacement_decision decision = {
        CTEX_MESH_REPLACEMENT_DECISION_CURRENT_SIZE,
        replacement_ids + replacement_entries[0].texture_set_id_offset,
        CTEX_MESH_REPLACEMENT_REQUEST_REPROJECTION,
    };
    if (!expect(ctex_mesh_replacement_plan_apply(replacement_plan, &decision, 1, &apply_info) ==
                CTEX_RESULT_SUCCESS) ||
        !expect(apply_info.replacement_applied == 0 &&
                apply_info.reprojection_pending_texture_set_count == 1) ||
        !expect(ctex_mesh_get_info(mesh, &after_rejection) == CTEX_RESULT_SUCCESS) ||
        !expect(after_rejection.revision == initial.revision)) {
        return 5;
    }
    decision.policy = CTEX_MESH_REPLACEMENT_KEEP_TEXELS;
    apply_info.size = CTEX_MESH_REPLACEMENT_APPLY_INFO_CURRENT_SIZE;
    if (!expect(ctex_mesh_replacement_plan_apply(replacement_plan, &decision, 1, &apply_info) ==
                CTEX_RESULT_SUCCESS) ||
        !expect(apply_info.replacement_applied == 1 && apply_info.kept_texture_set_count == 1 &&
                apply_info.replacement_mesh_revision > initial.revision)) {
        return 6;
    }
    ctex_mesh_replacement_plan_destroy(replacement_plan);
    replacement_plan = NULL;

    if (!expect(ctex_mesh_replacement_plan_create(document, mesh, &descriptor, &stale_plan) ==
                CTEX_RESULT_SUCCESS)) {
        return 7;
    }
    positions[0].x = 2.0F;
    if (!expect(ctex_mesh_replace(mesh, &descriptor) == CTEX_RESULT_SUCCESS) ||
        !expect(ctex_mesh_get_info(mesh, &replaced) == CTEX_RESULT_SUCCESS) ||
        !expect(replaced.revision > initial.revision)) {
        return 8;
    }
    apply_info.size = CTEX_MESH_REPLACEMENT_APPLY_INFO_CURRENT_SIZE;
    if (!expect(ctex_mesh_replacement_plan_apply(stale_plan, NULL, 0, &apply_info) ==
                CTEX_RESULT_INVALID_ARGUMENT) ||
        !expect(ctex_mesh_get_info(mesh, &after_rejection) == CTEX_RESULT_SUCCESS) ||
        !expect(after_rejection.revision == replaced.revision)) {
        return 9;
    }
    ctex_mesh_replacement_plan_destroy(stale_plan);
    stale_plan = NULL;

    descriptor.default_uv_set = "missing";
    if (!expect(ctex_mesh_replace(mesh, &descriptor) == CTEX_RESULT_INVALID_ARGUMENT) ||
        !expect(ctex_get_last_diagnostic_code() == CTEX_DIAGNOSTIC_INVALID_MESH) ||
        !expect(ctex_mesh_get_info(mesh, &after_rejection) == CTEX_RESULT_SUCCESS) ||
        !expect(after_rejection.revision == replaced.revision)) {
        return 10;
    }

    descriptor.default_uv_set = "uv0";
    descriptor.position_count = CTEX_MAX_MESH_VERTEX_COUNT + 1;
    if (!expect(ctex_mesh_replace(mesh, &descriptor) == CTEX_RESULT_OVER_BUDGET) ||
        !expect(ctex_get_last_diagnostic_code() == CTEX_DIAGNOSTIC_MESH_LIMIT_EXCEEDED) ||
        !expect(strstr(ctex_get_last_diagnostic(), "vertex_count supplied=100000001") != NULL) ||
        !expect(strstr(ctex_get_last_diagnostic(), "maximum=100000000") != NULL)) {
        return 11;
    }

    if (!expect(ctex_set_allocator(NULL) == CTEX_RESULT_SUCCESS)) {
        return 12;
    }
    ctex_document_destroy(document);
    ctex_uv_pick_index_destroy(uv_pick_index);
    ctex_pick_index_destroy(pick_index);
    ctex_mesh_destroy(mesh);
    if (!expect(counts.allocations > 1 && counts.allocations == counts.deallocations)) {
        return 13;
    }
    return 0;
}
