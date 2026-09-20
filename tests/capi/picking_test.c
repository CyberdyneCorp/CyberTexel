#include <ctex/capi.h>
#include <math.h>
#include <stdint.h>
#include <string.h>

typedef struct mesh_fixture {
    ctex_vec3f positions[6];
    ctex_vec3f normals[6];
    ctex_vec2f uv[6];
    uint32_t indices[6];
    uint32_t face_partitions[2];
    uint32_t face_materials[2];
    ctex_uv_set_descriptor uv_set;
    ctex_mesh_partition_descriptor partition;
    ctex_mesh_descriptor descriptor;
} mesh_fixture;

static void fixture(mesh_fixture* result, float x_offset) {
    const ctex_vec3f positions[6] = {
        {x_offset, 0.0F, 0.0F},  {x_offset + 1.0F, 0.0F, 0.0F},  {x_offset, 1.0F, 0.0F},
        {x_offset, 0.0F, -1.0F}, {x_offset + 1.0F, 0.0F, -1.0F}, {x_offset, 1.0F, -1.0F},
    };
    const ctex_vec2f uv[6] = {
        {0.0F, 0.0F}, {1.0F, 0.0F}, {0.0F, 1.0F}, {0.0F, 0.0F}, {1.0F, 0.0F}, {0.0F, 1.0F},
    };
    const uint32_t indices[6] = {0, 1, 2, 3, 4, 5};
    size_t index = 0;
    memset(result, 0, sizeof(*result));
    memcpy(result->positions, positions, sizeof(positions));
    memcpy(result->uv, uv, sizeof(uv));
    memcpy(result->indices, indices, sizeof(indices));
    for (index = 0; index < 6; ++index) {
        result->normals[index] = (ctex_vec3f){0.0F, 0.0F, 1.0F};
    }
    result->face_partitions[0] = 0;
    result->face_partitions[1] = 0;
    result->face_materials[0] = 17;
    result->face_materials[1] = 23;
    result->uv_set =
        (ctex_uv_set_descriptor){CTEX_UV_SET_DESCRIPTOR_CURRENT_SIZE, "paint", result->uv, 6};
    result->partition = (ctex_mesh_partition_descriptor){
        CTEX_MESH_PARTITION_DESCRIPTOR_CURRENT_SIZE,
        CTEX_PARTITION_SOURCE_MATERIAL,
        "body",
        "Body",
    };
    result->descriptor = (ctex_mesh_descriptor){
        CTEX_MESH_DESCRIPTOR_CURRENT_SIZE,
        result->positions,
        6,
        result->normals,
        6,
        NULL,
        0,
        result->indices,
        6,
        &result->uv_set,
        1,
        "paint",
        &result->partition,
        1,
        result->face_partitions,
        2,
        result->face_materials,
        2,
    };
}

static ctex_pick_texture_set_binding_descriptor binding(void) {
    return (ctex_pick_texture_set_binding_descriptor){
        CTEX_PICK_TEXTURE_SET_BINDING_DESCRIPTOR_CURRENT_SIZE, 0, "paint"};
}

static ctex_pick_query_info query_info(void) {
    ctex_pick_query_info result = {0};
    result.size = CTEX_PICK_QUERY_INFO_CURRENT_SIZE;
    return result;
}

static int close_float(float left, float right) { return fabsf(left - right) <= 1.0e-5F; }

static int ray_queries(ctex_pick_index* index) {
    const ctex_pick_ray ray = {{0.25F, 0.25F, 1.0F}, {0.0F, 0.0F, -1.0F}};
    const ctex_pick_texture_set_binding_descriptor texture_set = binding();
    const ctex_pick_options_descriptor all_options = {
        CTEX_PICK_OPTIONS_DESCRIPTOR_CURRENT_SIZE,
        5.0F,
        CTEX_PICK_OCCLUSION_ALL_HITS,
        CTEX_PICK_BACKFACE_ACCEPT,
    };
    ctex_pick_query_info info = query_info();
    ctex_pick_hit hits[2] = {0};
    char ids[64] = {0};
    ctex_pick_hit sentinel = {0};
    char sentinel_id[64];
    sentinel.triangle_index = 999;
    memset(sentinel_id, 'X', sizeof(sentinel_id));

    if (ctex_pick_ray_query(index, ray, &all_options, &texture_set, 1, NULL, 0, NULL, 0, &info) !=
            CTEX_RESULT_SUCCESS ||
        info.result_count != 2 || info.required_texture_set_id_size != 54 ||
        info.visited_nodes == 0 || info.tested_leaf_triangles == 0) {
        return 0;
    }
    if (ctex_pick_ray_query(index, ray, &all_options, &texture_set, 1, &sentinel, 1, sentinel_id,
                            sizeof(sentinel_id), &info) != CTEX_RESULT_BUFFER_TOO_SMALL ||
        sentinel.triangle_index != 999 || sentinel_id[0] != 'X') {
        return 0;
    }
    if (ctex_pick_ray_query(index, ray, &all_options, &texture_set, 1, hits, 2, ids, sizeof(ids),
                            &info) != CTEX_RESULT_SUCCESS ||
        hits[0].has_hit != 1 || hits[1].has_hit != 1 || hits[0].triangle_index != 0 ||
        hits[1].triangle_index != 1 || !close_float(hits[0].distance, 1.0F) ||
        !close_float(hits[1].distance, 2.0F) ||
        strcmp(ids + hits[0].texture_set_id_offset, "material/4:body/uv/5:paint") != 0 ||
        strcmp(ids + hits[1].texture_set_id_offset, "material/4:body/uv/5:paint") != 0) {
        return 0;
    }
    return 1;
}

static int ray_and_snap_edge_cases(ctex_pick_index* index) {
    const ctex_pick_texture_set_binding_descriptor texture_set = binding();
    ctex_pick_options_descriptor options = {
        CTEX_PICK_OPTIONS_DESCRIPTOR_CURRENT_SIZE,
        5.0F,
        CTEX_PICK_OCCLUSION_NEAREST,
        CTEX_PICK_BACKFACE_REJECT,
    };
    ctex_pick_query_info info = query_info();
    ctex_pick_hit hit = {0};
    char id[32] = {0};
    const ctex_pick_ray back_ray = {{0.25F, 0.25F, -2.0F}, {0.0F, 0.0F, 1.0F}};
    const ctex_pick_ray miss_ray = {{4.0F, 4.0F, 1.0F}, {0.0F, 0.0F, -1.0F}};

    if (ctex_pick_ray_query(index, back_ray, &options, &texture_set, 1, NULL, 0, NULL, 0, &info) !=
            CTEX_RESULT_SUCCESS ||
        info.result_count != 0 ||
        ctex_pick_ray_query(index, miss_ray, &options, &texture_set, 1, NULL, 0, NULL, 0, &info) !=
            CTEX_RESULT_SUCCESS ||
        info.result_count != 0) {
        return 0;
    }
    if (ctex_pick_snap_to_surface(index, (ctex_vec3f){0.25F, 0.25F, 0.2F}, 0.5F, &texture_set, 1,
                                  &hit, id, sizeof(id), &info) != CTEX_RESULT_SUCCESS ||
        info.result_count != 1 || hit.triangle_index != 0 || !close_float(hit.distance, 0.2F)) {
        return 0;
    }
    hit.has_hit = 1;
    if (ctex_pick_snap_to_surface(index, (ctex_vec3f){4.0F, 4.0F, 0.2F}, 0.1F, &texture_set, 1,
                                  &hit, id, sizeof(id), &info) != CTEX_RESULT_SUCCESS ||
        info.result_count != 0 || hit.has_hit != 0) {
        return 0;
    }
    return 1;
}

static ctex_pick_screen_view_descriptor screen_view(void) {
    ctex_pick_screen_view_descriptor result = {0};
    size_t diagonal = 0;
    result.size = CTEX_PICK_SCREEN_VIEW_DESCRIPTOR_CURRENT_SIZE;
    result.viewport_width = 100;
    result.viewport_height = 100;
    for (diagonal = 0; diagonal < 4; ++diagonal) {
        result.view[diagonal * 5] = 1.0F;
        result.projection[diagonal * 5] = 1.0F;
    }
    return result;
}

static int screen_and_region_queries(ctex_pick_index* index) {
    ctex_pick_screen_view_descriptor view = screen_view();
    ctex_pick_query_info info = query_info();
    ctex_pick_ray ray = {0};
    uint32_t triangles[2] = {99, 99};
    const ctex_vec2f lasso[4] = {{45.0F, 45.0F}, {80.0F, 45.0F}, {80.0F, 20.0F}, {45.0F, 20.0F}};

    if (ctex_pick_ray_from_screen((ctex_vec2f){50.0F, 50.0F}, &view,
                                  CTEX_PICK_PROJECTION_PERSPECTIVE, &ray) != CTEX_RESULT_SUCCESS ||
        !close_float(ray.direction.z, 1.0F)) {
        return 0;
    }
    if (ctex_pick_query_screen_rectangle(index, (ctex_vec2f){45.0F, 20.0F},
                                         (ctex_vec2f){80.0F, 55.0F}, &view, triangles, 2,
                                         &info) != CTEX_RESULT_SUCCESS ||
        info.result_count != 2 || triangles[0] != 0 || triangles[1] != 1 ||
        ctex_pick_query_screen_lasso(index, lasso, 4, &view, triangles, 2, &info) !=
            CTEX_RESULT_SUCCESS ||
        info.result_count != 2 ||
        ctex_pick_query_world_sphere(index, (ctex_vec3f){0.25F, 0.25F, 0.0F}, 0.1F, triangles, 2,
                                     &info) != CTEX_RESULT_SUCCESS ||
        info.result_count != 1 || triangles[0] != 0 ||
        ctex_pick_query_world_box(index, (ctex_vec3f){0.0F, 0.0F, -1.1F},
                                  (ctex_vec3f){0.5F, 0.5F, -0.9F}, triangles, 2,
                                  &info) != CTEX_RESULT_SUCCESS ||
        info.result_count != 1 || triangles[0] != 1) {
        return 0;
    }
    return 1;
}

static int uv_query(ctex_mesh* mesh) {
    ctex_uv_pick_index* index = NULL;
    ctex_pick_texture_set_binding_descriptor texture_set = binding();
    ctex_pick_query_info info = query_info();
    ctex_pick_hit hit = {0};
    char id[32] = {0};
    ctex_pick_index_info index_info = {.size = CTEX_PICK_INDEX_INFO_CURRENT_SIZE};
    int success = 0;
    if (ctex_uv_pick_index_create(mesh, "paint", &index) != CTEX_RESULT_SUCCESS || index == NULL ||
        ctex_uv_pick_index_get_info(index, &index_info) != CTEX_RESULT_SUCCESS ||
        index_info.build_count != 1 || index_info.triangle_count != 2 ||
        ctex_pick_uv_query(index, (ctex_vec2f){0.25F, 0.25F}, &texture_set, &hit, id, sizeof(id),
                           &info) != CTEX_RESULT_SUCCESS ||
        info.result_count != 1 || hit.triangle_index != 0 || hit.material_id != 17 ||
        strcmp(id, "material/4:body/uv/5:paint") != 0) {
        goto cleanup;
    }
    hit.has_hit = 1;
    if (ctex_pick_uv_query(index, (ctex_vec2f){2.0F, 2.0F}, &texture_set, &hit, id, sizeof(id),
                           &info) != CTEX_RESULT_SUCCESS ||
        info.result_count != 0 || hit.has_hit != 0) {
        goto cleanup;
    }
    success = 1;
cleanup:
    ctex_uv_pick_index_destroy(index);
    return success;
}

typedef struct callback_capture {
    size_t progress_calls;
    int cancel;
} callback_capture;

static uint32_t cancelled(void* user_data) { return ((callback_capture*)user_data)->cancel != 0; }

static void progressed(size_t completed, size_t total, void* user_data) {
    callback_capture* capture = (callback_capture*)user_data;
    (void)completed;
    (void)total;
    ++capture->progress_calls;
}

static int batch_queries(ctex_pick_index* index) {
    const ctex_pick_ray rays[2] = {
        {{0.25F, 0.25F, 1.0F}, {0.0F, 0.0F, -1.0F}},
        {{4.0F, 4.0F, 1.0F}, {0.0F, 0.0F, -1.0F}},
    };
    const ctex_pick_texture_set_binding_descriptor texture_set = binding();
    callback_capture capture = {0};
    ctex_pick_batch_control_descriptor control = {
        CTEX_PICK_BATCH_CONTROL_DESCRIPTOR_CURRENT_SIZE,
        SIZE_MAX,
        1,
        &capture,
        cancelled,
        progressed,
    };
    ctex_pick_batch_info info = {.size = CTEX_PICK_BATCH_INFO_CURRENT_SIZE};
    ctex_pick_hit hits[2] = {0};
    char ids[32] = {0};

    if (ctex_pick_nearest_batch(index, rays, 2, 5.0F, CTEX_PICK_BACKFACE_ACCEPT, &texture_set, 1,
                                &control, NULL, 0, NULL, 0, &info) != CTEX_RESULT_SUCCESS ||
        info.status != CTEX_PICK_BATCH_COMPLETE || info.processed_rays != 2 ||
        info.required_hit_count != 2 || info.required_texture_set_id_size != 27 ||
        info.visited_nodes == 0 || capture.progress_calls < 2 ||
        ctex_pick_nearest_batch(index, rays, 2, 5.0F, CTEX_PICK_BACKFACE_ACCEPT, &texture_set, 1,
                                &control, hits, 2, ids, sizeof(ids),
                                &info) != CTEX_RESULT_SUCCESS ||
        hits[0].has_hit != 1 || hits[0].triangle_index != 0 || hits[1].has_hit != 0) {
        return 0;
    }
    control.memory_ceiling_bytes = 1;
    if (ctex_pick_nearest_batch(index, rays, 2, 5.0F, CTEX_PICK_BACKFACE_ACCEPT, &texture_set, 1,
                                &control, NULL, 0, NULL, 0, &info) != CTEX_RESULT_OVER_BUDGET ||
        info.status != CTEX_PICK_BATCH_MEMORY_CEILING_EXCEEDED || info.required_memory_bytes <= 1) {
        return 0;
    }
    control.memory_ceiling_bytes = SIZE_MAX;
    capture.cancel = 1;
    if (ctex_pick_nearest_batch(index, rays, 2, 5.0F, CTEX_PICK_BACKFACE_ACCEPT, &texture_set, 1,
                                &control, NULL, 0, NULL, 0, &info) != CTEX_RESULT_CANCELLED ||
        info.status != CTEX_PICK_BATCH_CANCELLED || info.processed_rays != 0) {
        return 0;
    }
    return 1;
}

static int replacement_rebuilds(ctex_mesh* mesh, ctex_pick_index* index) {
    mesh_fixture replacement;
    const ctex_pick_ray ray = {{10.25F, 0.25F, 1.0F}, {0.0F, 0.0F, -1.0F}};
    const ctex_pick_texture_set_binding_descriptor texture_set = binding();
    const ctex_pick_options_descriptor options = {
        CTEX_PICK_OPTIONS_DESCRIPTOR_CURRENT_SIZE,
        5.0F,
        CTEX_PICK_OCCLUSION_NEAREST,
        CTEX_PICK_BACKFACE_ACCEPT,
    };
    ctex_pick_query_info info = query_info();
    ctex_pick_index_info index_info = {.size = CTEX_PICK_INDEX_INFO_CURRENT_SIZE};
    fixture(&replacement, 10.0F);
    if (ctex_mesh_replace(mesh, &replacement.descriptor) != CTEX_RESULT_SUCCESS ||
        ctex_pick_ray_query(index, ray, &options, &texture_set, 1, NULL, 0, NULL, 0, &info) !=
            CTEX_RESULT_SUCCESS ||
        info.result_count != 1 || info.index_build_count != 2 ||
        ctex_pick_index_get_info(index, &index_info) != CTEX_RESULT_SUCCESS ||
        index_info.build_count != 2 || index_info.mesh_revision != info.mesh_revision) {
        return 0;
    }
    return 1;
}

int main(void) {
    mesh_fixture buffers;
    ctex_mesh* mesh = NULL;
    ctex_pick_index* index = NULL;
    ctex_pick_index_info index_info = {.size = CTEX_PICK_INDEX_INFO_CURRENT_SIZE};
    fixture(&buffers, 0.0F);
    if (ctex_mesh_create(&buffers.descriptor, &mesh) != CTEX_RESULT_SUCCESS || mesh == NULL ||
        ctex_pick_index_create(mesh, &index) != CTEX_RESULT_SUCCESS || index == NULL ||
        ctex_pick_index_get_info(index, &index_info) != CTEX_RESULT_SUCCESS ||
        index_info.build_count != 1 || index_info.triangle_count != 2) {
        return 1;
    }
    if (!ray_queries(index)) {
        return 2;
    }
    if (!ray_and_snap_edge_cases(index)) {
        return 3;
    }
    if (!screen_and_region_queries(index)) {
        return 4;
    }
    if (!uv_query(mesh)) {
        return 5;
    }
    if (!batch_queries(index)) {
        return 6;
    }
    if (!replacement_rebuilds(mesh, index)) {
        return 7;
    }
    ctex_pick_index_destroy(index);
    ctex_mesh_destroy(mesh);
    return 0;
}
