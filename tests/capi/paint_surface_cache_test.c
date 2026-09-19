#include <ctex/capi.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

typedef struct allocation_counts {
    size_t allocations;
    size_t deallocations;
} allocation_counts;

typedef struct surface_cache_fixture {
    ctex_vec3f positions[3];
    ctex_vec3f normals[3];
    ctex_vec2f uv0[3];
    ctex_vec2f uv1[3];
    uint32_t indices[3];
    uint32_t face_partitions[1];
    uint32_t face_materials[1];
    ctex_uv_set_descriptor uv_sets[2];
    ctex_mesh_partition_descriptor partitions[1];
    ctex_mesh_descriptor descriptor;
    allocation_counts counts;
    ctex_allocator_descriptor allocator;
    ctex_mesh* mesh;
    ctex_paint_surface_map_cache* cache;
    ctex_paint_surface_map_request request;
    ctex_paint_surface_map_info info;
    ctex_paint_surface_map_statistics statistics;
    char texture_set_id[64];
    char uv_set[8];
    ctex_paint_surface_texel surface[16];
    uint8_t coverage[16];
    uint32_t triangles[16];
    uint32_t islands[16];
    ctex_paint_surface_map_buffers buffers;
    size_t allocations_before_lookup;
} surface_cache_fixture;

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

static void initialize_fixture(surface_cache_fixture* fixture) {
    const ctex_vec3f positions[3] = {{0.0F, 0.0F, 0.0F}, {1.0F, 0.0F, 0.0F}, {0.0F, 1.0F, 0.0F}};
    const ctex_vec3f normals[3] = {{0.0F, 0.0F, 1.0F}, {0.0F, 0.0F, 1.0F}, {0.0F, 0.0F, 1.0F}};
    const ctex_vec2f uv0[3] = {{0.0F, 0.0F}, {1.0F, 0.0F}, {0.0F, 1.0F}};
    const ctex_vec2f uv1[3] = {{0.0F, 0.0F}, {0.0F, 1.0F}, {1.0F, 0.0F}};
    const uint32_t indices[3] = {0, 1, 2};
    memcpy(fixture->positions, positions, sizeof(positions));
    memcpy(fixture->normals, normals, sizeof(normals));
    memcpy(fixture->uv0, uv0, sizeof(uv0));
    memcpy(fixture->uv1, uv1, sizeof(uv1));
    memcpy(fixture->indices, indices, sizeof(indices));
    fixture->face_materials[0] = 7;
    fixture->uv_sets[0] =
        (ctex_uv_set_descriptor){CTEX_UV_SET_DESCRIPTOR_CURRENT_SIZE, "uv0", fixture->uv0, 3};
    fixture->uv_sets[1] =
        (ctex_uv_set_descriptor){CTEX_UV_SET_DESCRIPTOR_CURRENT_SIZE, "uv1", fixture->uv1, 3};
    fixture->partitions[0] = (ctex_mesh_partition_descriptor){
        CTEX_MESH_PARTITION_DESCRIPTOR_CURRENT_SIZE,
        CTEX_PARTITION_SOURCE_MATERIAL,
        "body",
        "Body",
    };
    fixture->descriptor = (ctex_mesh_descriptor){
        CTEX_MESH_DESCRIPTOR_CURRENT_SIZE,
        fixture->positions,
        3,
        fixture->normals,
        3,
        NULL,
        0,
        fixture->indices,
        3,
        fixture->uv_sets,
        2,
        "uv0",
        fixture->partitions,
        1,
        fixture->face_partitions,
        1,
        fixture->face_materials,
        1,
    };
    fixture->allocator = (ctex_allocator_descriptor){
        CTEX_ALLOCATOR_DESCRIPTOR_CURRENT_SIZE,
        count_allocate,
        count_deallocate,
        &fixture->counts,
    };
    fixture->request = (ctex_paint_surface_map_request){
        .size = CTEX_PAINT_SURFACE_MAP_REQUEST_CURRENT_SIZE,
        .partition_index = 0,
        .uv_set = "uv0",
        .width = 4,
        .height = 4,
    };
    fixture->info.size = CTEX_PAINT_SURFACE_MAP_INFO_CURRENT_SIZE;
    fixture->statistics.size = CTEX_PAINT_SURFACE_MAP_STATISTICS_CURRENT_SIZE;
    fixture->coverage[0] = 99;
    fixture->triangles[0] = 99;
    fixture->islands[0] = 99;
    fixture->buffers = (ctex_paint_surface_map_buffers){
        .size = CTEX_PAINT_SURFACE_MAP_BUFFERS_CURRENT_SIZE,
        .texture_set_id = fixture->texture_set_id,
        .texture_set_id_size = sizeof(fixture->texture_set_id),
        .uv_set = fixture->uv_set,
        .uv_set_size = sizeof(fixture->uv_set),
        .surface_texels = fixture->surface,
        .surface_texel_capacity = 15,
        .coverage = fixture->coverage,
        .coverage_capacity = 16,
        .triangle_identity = fixture->triangles,
        .triangle_identity_capacity = 16,
        .uv_island_identity = fixture->islands,
        .uv_island_identity_capacity = 16,
    };
}

static int create_handles(surface_cache_fixture* fixture) {
    if (!expect(ctex_set_allocator(&fixture->allocator) == CTEX_RESULT_SUCCESS) ||
        !expect(ctex_mesh_create(&fixture->descriptor, &fixture->mesh) == CTEX_RESULT_SUCCESS &&
                fixture->mesh != NULL) ||
        !expect(ctex_paint_surface_map_cache_create(&fixture->cache) == CTEX_RESULT_SUCCESS &&
                fixture->cache != NULL)) {
        return 0;
    }
    fixture->allocations_before_lookup = fixture->counts.allocations;
    return 1;
}

static int first_lookup_and_copy_are_cached(surface_cache_fixture* fixture) {
    if (!expect(ctex_paint_surface_map_cache_lookup(fixture->cache, fixture->mesh,
                                                    &fixture->request, &fixture->info,
                                                    NULL) == CTEX_RESULT_SUCCESS) ||
        !expect(fixture->info.cache_hit == 0 && fixture->info.required_texel_count == 16 &&
                fixture->info.mesh_revision != 0 &&
                fixture->counts.allocations > fixture->allocations_before_lookup)) {
        return 0;
    }
    if (!expect(ctex_paint_surface_map_cache_lookup(
                    fixture->cache, fixture->mesh, &fixture->request, &fixture->info,
                    &fixture->buffers) == CTEX_RESULT_BUFFER_TOO_SMALL) ||
        !expect(fixture->info.cache_hit == 1 && fixture->texture_set_id[0] == '\0' &&
                fixture->coverage[0] == 99 && fixture->triangles[0] == 99 &&
                fixture->islands[0] == 99)) {
        return 0;
    }
    fixture->buffers.surface_texel_capacity = 16;
    return expect(ctex_paint_surface_map_cache_lookup(fixture->cache, fixture->mesh,
                                                      &fixture->request, &fixture->info,
                                                      &fixture->buffers) == CTEX_RESULT_SUCCESS) &&
           expect(fixture->info.cache_hit == 1 &&
                  strcmp(fixture->texture_set_id, "material/4:body/uv/3:uv0") == 0 &&
                  strcmp(fixture->uv_set, "uv0") == 0);
}

static int copied_maps_and_statistics_agree(surface_cache_fixture* fixture) {
    size_t covered_count = 0;
    for (size_t index = 0; index < 16; ++index) {
        if (!expect(fixture->surface[index].triangle == fixture->triangles[index]) ||
            !expect((fixture->coverage[index] != 0) ==
                    (fixture->triangles[index] != CTEX_NO_SURFACE_TRIANGLE)) ||
            !expect((fixture->triangles[index] == CTEX_NO_SURFACE_TRIANGLE) ==
                    (fixture->islands[index] == CTEX_NO_UV_ISLAND))) {
            return 0;
        }
        covered_count += fixture->coverage[index] != 0 ? 1 : 0;
    }
    return expect(covered_count != 0) &&
           expect(ctex_paint_surface_map_cache_get_statistics(
                      fixture->cache, &fixture->statistics) == CTEX_RESULT_SUCCESS) &&
           expect(fixture->statistics.entries == 1 && fixture->statistics.hits == 2 &&
                  fixture->statistics.misses == 1 && fixture->statistics.invalidated_entries == 0);
}

static int changing_uv_and_mesh_invalidates(surface_cache_fixture* fixture) {
    fixture->request.uv_set = "uv1";
    if (!expect(ctex_paint_surface_map_cache_lookup(fixture->cache, fixture->mesh,
                                                    &fixture->request, &fixture->info,
                                                    NULL) == CTEX_RESULT_SUCCESS) ||
        !expect(fixture->info.cache_hit == 0) ||
        !expect(ctex_paint_surface_map_cache_get_statistics(fixture->cache, &fixture->statistics) ==
                CTEX_RESULT_SUCCESS) ||
        !expect(fixture->statistics.entries == 1 && fixture->statistics.misses == 2 &&
                fixture->statistics.invalidated_entries == 1)) {
        return 0;
    }
    fixture->positions[0].x = 0.25F;
    return expect(ctex_mesh_replace(fixture->mesh, &fixture->descriptor) == CTEX_RESULT_SUCCESS) &&
           expect(ctex_paint_surface_map_cache_lookup(fixture->cache, fixture->mesh,
                                                      &fixture->request, &fixture->info,
                                                      NULL) == CTEX_RESULT_SUCCESS) &&
           expect(fixture->info.cache_hit == 0) &&
           expect(ctex_paint_surface_map_cache_get_statistics(
                      fixture->cache, &fixture->statistics) == CTEX_RESULT_SUCCESS) &&
           expect(fixture->statistics.entries == 1 && fixture->statistics.misses == 3 &&
                  fixture->statistics.invalidated_entries == 2);
}

static int invalid_lookup_is_transactional_and_clear_resets(surface_cache_fixture* fixture) {
    fixture->request.partition_index = 9;
    if (!expect(ctex_paint_surface_map_cache_lookup(fixture->cache, fixture->mesh,
                                                    &fixture->request, &fixture->info,
                                                    NULL) == CTEX_RESULT_INVALID_ARGUMENT) ||
        !expect(ctex_get_last_diagnostic_code() == CTEX_DIAGNOSTIC_INVALID_PAINT_SURFACE_CACHE) ||
        !expect(ctex_paint_surface_map_cache_get_statistics(fixture->cache, &fixture->statistics) ==
                CTEX_RESULT_SUCCESS) ||
        !expect(fixture->statistics.entries == 1 && fixture->statistics.misses == 3 &&
                fixture->statistics.invalidated_entries == 2)) {
        return 0;
    }
    fixture->request.partition_index = 0;
    return expect(ctex_paint_surface_map_cache_clear(fixture->cache) == CTEX_RESULT_SUCCESS) &&
           expect(ctex_paint_surface_map_cache_get_statistics(
                      fixture->cache, &fixture->statistics) == CTEX_RESULT_SUCCESS) &&
           expect(fixture->statistics.entries == 0 && fixture->statistics.hits == 0 &&
                  fixture->statistics.misses == 0 &&
                  fixture->statistics.invalidated_entries == 0) &&
           expect(ctex_paint_surface_map_cache_lookup(fixture->cache, fixture->mesh,
                                                      &fixture->request, &fixture->info,
                                                      NULL) == CTEX_RESULT_SUCCESS);
}

static int destroy_uses_captured_allocator(surface_cache_fixture* fixture) {
    const int reset = expect(ctex_set_allocator(NULL) == CTEX_RESULT_SUCCESS);
    ctex_paint_surface_map_cache_destroy(fixture->cache);
    ctex_mesh_destroy(fixture->mesh);
    return reset && expect(fixture->counts.allocations > fixture->allocations_before_lookup &&
                           fixture->counts.allocations == fixture->counts.deallocations);
}

int main(void) {
    surface_cache_fixture fixture = {0};
    initialize_fixture(&fixture);
    if (!create_handles(&fixture)) {
        return 1;
    }
    const int passed = first_lookup_and_copy_are_cached(&fixture) &&
                       copied_maps_and_statistics_agree(&fixture) &&
                       changing_uv_and_mesh_invalidates(&fixture) &&
                       invalid_lookup_is_transactional_and_clear_resets(&fixture);
    return destroy_uses_captured_allocator(&fixture) && passed ? 0 : 1;
}
