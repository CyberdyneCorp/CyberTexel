#include <ctex/capi.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static int expect(int condition, const char* message) {
    if (!condition) {
        fprintf(stderr, "%s\n", message);
    }
    return condition;
}

typedef struct fixture {
    ctex_document* document;
    ctex_mesh* mesh;
    ctex_mesh_map_set* maps;
    ctex_mesh_descriptor mesh_descriptor;
    ctex_mesh_tangent_data_descriptor tangent_data;
    char texture_set_id[128];
} fixture;

static int create_fixture(fixture* value) {
    static ctex_vec3f positions[] = {{0.0F, 0.0F, 0.0F}, {1.0F, 0.0F, 0.0F}, {0.0F, 1.0F, 0.0F}};
    static ctex_vec3f normals[] = {{0.0F, 0.0F, 1.0F}, {0.0F, 0.0F, 1.0F}, {0.0F, 0.0F, 1.0F}};
    static ctex_vec2f uv_values[] = {{0.0F, 0.0F}, {1.0F, 0.0F}, {0.0F, 1.0F}};
    static uint32_t triangle_indices[] = {0, 1, 2};
    static uint32_t face_partitions[] = {0};
    static uint32_t face_materials[] = {1};
    static ctex_vec4f corner_tangents[] = {
        {1.0F, 0.0F, 0.0F, 1.0F},
        {1.0F, 0.0F, 0.0F, -1.0F},
        {1.0F, 0.0F, 0.0F, 1.0F},
    };
    static ctex_uv_set_descriptor uv_sets[] = {{
        CTEX_UV_SET_DESCRIPTOR_CURRENT_SIZE,
        "uv0",
        uv_values,
        3,
    }};
    static ctex_mesh_partition_descriptor partitions[] = {{
        CTEX_MESH_PARTITION_DESCRIPTOR_CURRENT_SIZE,
        CTEX_PARTITION_SOURCE_MATERIAL,
        "body",
        "Body",
    }};
    const ctex_texture_set_descriptor texture_set = {
        .size = CTEX_TEXTURE_SET_DESCRIPTOR_CURRENT_SIZE,
        .display_name = "Body",
        .partition_kind = CTEX_PARTITION_SOURCE_MATERIAL,
        .partition_key = "body",
        .uv_set = "uv0",
        .width = 4,
        .height = 4,
        .default_bit_depth = 8,
    };
    size_t required_size = 0;
    size_t count = 0;

    value->mesh_descriptor = (ctex_mesh_descriptor){
        CTEX_MESH_DESCRIPTOR_CURRENT_SIZE,
        positions,
        3,
        normals,
        3,
        NULL,
        0,
        triangle_indices,
        3,
        uv_sets,
        1,
        "uv0",
        partitions,
        1,
        face_partitions,
        1,
        face_materials,
        1,
    };
    value->tangent_data = (ctex_mesh_tangent_data_descriptor){
        .size = CTEX_MESH_TANGENT_DATA_DESCRIPTOR_CURRENT_SIZE,
        .frame =
            {
                .size = CTEX_TANGENT_FRAME_DESCRIPTOR_CURRENT_SIZE,
                .algorithm = CTEX_TANGENT_BASIS_UV_DERIVATIVE,
                .algorithm_version = 1,
                .normal_orientation = CTEX_TANGENT_NORMAL_VERTEX,
                .coordinate_handedness = CTEX_COORDINATE_RIGHT_HANDED,
                .uv_v_axis = CTEX_UV_V_AXIS_UPWARD,
                .handedness_encoding = CTEX_TANGENT_HANDEDNESS_W_SIGN,
                .uv_set = "uv0",
            },
        .corner_tangents = corner_tangents,
        .corner_tangent_count = 3,
    };
    return ctex_document_create(&value->document) == CTEX_RESULT_SUCCESS &&
           ctex_document_create_texture_set(value->document, &texture_set) == CTEX_RESULT_SUCCESS &&
           ctex_document_get_texture_set_ids(value->document, NULL, 0, &required_size, &count) ==
               CTEX_RESULT_SUCCESS &&
           count == 1 && required_size <= sizeof(value->texture_set_id) &&
           ctex_document_get_texture_set_ids(value->document, value->texture_set_id,
                                             sizeof(value->texture_set_id), &required_size,
                                             &count) == CTEX_RESULT_SUCCESS &&
           ctex_mesh_create_with_tangent_data(&value->mesh_descriptor, &value->tangent_data,
                                              &value->mesh) == CTEX_RESULT_SUCCESS &&
           ctex_mesh_map_set_create(value->document, value->texture_set_id, value->mesh,
                                    &value->maps) == CTEX_RESULT_SUCCESS;
}

static void destroy_fixture(fixture* value) {
    ctex_mesh_map_set_destroy(value->maps);
    ctex_mesh_destroy(value->mesh);
    ctex_document_destroy(value->document);
}

static ctex_mesh_map_import_descriptor ao_import(const uint8_t* pixels) {
    ctex_mesh_map_import_descriptor result = {
        .size = CTEX_MESH_MAP_IMPORT_DESCRIPTOR_CURRENT_SIZE,
        .kind = CTEX_MESH_MAP_AMBIENT_OCCLUSION,
        .channel_meaning = CTEX_MESH_MAP_SCALAR_DATA,
        .color_space = CTEX_COLOR_SPACE_LINEAR_REC709,
        .buffer =
            {
                .size = CTEX_MESH_MAP_PIXEL_BUFFER_DESCRIPTOR_CURRENT_SIZE,
                .width = 2,
                .height = 2,
                .component_type = CTEX_TRANSPORT_COMPONENT_UINT8_UNORM,
                .component_count = 1,
                .row_stride_bytes = 2,
                .pixels = pixels,
                .pixel_bytes = 4,
            },
    };
    return result;
}

static int inventory_and_external_import(fixture* value) {
    ctex_mesh_map_set_info before = {.size = CTEX_MESH_MAP_SET_INFO_CURRENT_SIZE};
    ctex_mesh_map_set_info after = {.size = CTEX_MESH_MAP_SET_INFO_CURRENT_SIZE};
    ctex_mesh_map_import_info imported = {.size = CTEX_MESH_MAP_IMPORT_INFO_CURRENT_SIZE};
    ctex_mesh_map_sample_info sample = {.size = CTEX_MESH_MAP_SAMPLE_INFO_CURRENT_SIZE};
    ctex_texture_set_memory_report document_memory = {
        .size = CTEX_TEXTURE_SET_MEMORY_REPORT_CURRENT_SIZE};
    uint8_t pixels[] = {0, 64, 128, 255};
    ctex_mesh_map_import_descriptor descriptor = ao_import(pixels);
    char texture_set_id[128] = {0};
    char uv_set[16] = {0};
    char name[32] = {0};
    size_t name_size = 0;

    if (!expect(
            ctex_mesh_map_set_get_info(value->maps, &before, texture_set_id, sizeof(texture_set_id),
                                       uv_set, sizeof(uv_set)) == CTEX_RESULT_SUCCESS,
            "mesh-map set metadata query failed") ||
        !expect(strcmp(texture_set_id, value->texture_set_id) == 0 && strcmp(uv_set, "uv0") == 0 &&
                    before.texture_set_width == 4 && before.texture_set_height == 4 &&
                    before.mesh_revision != 0 && before.bound_map_count == 0 &&
                    before.resident_pixel_bytes == 0,
                "mesh-map set metadata is incorrect") ||
        !expect(ctex_mesh_map_kind_get_name(CTEX_MESH_MAP_AMBIENT_OCCLUSION, NULL, 0, &name_size) ==
                        CTEX_RESULT_SUCCESS &&
                    name_size <= sizeof(name),
                "mesh-map stable-name sizing failed") ||
        !expect(ctex_mesh_map_kind_get_name(CTEX_MESH_MAP_AMBIENT_OCCLUSION, name, sizeof(name),
                                            &name_size) == CTEX_RESULT_SUCCESS &&
                    strcmp(name, "ambient-occlusion") == 0,
                "mesh-map stable name is incorrect")) {
        return 0;
    }
    descriptor.buffer.component_count = 257;
    if (!expect(ctex_mesh_map_set_import_external(value->maps, &descriptor, &imported) ==
                    CTEX_RESULT_INVALID_ARGUMENT,
                "an overflowing mesh-map component count was accepted") ||
        !expect(
            ctex_mesh_map_set_get_info(value->maps, &after, texture_set_id, sizeof(texture_set_id),
                                       uv_set, sizeof(uv_set)) == CTEX_RESULT_SUCCESS &&
                after.bound_map_count == 0,
            "a refused mesh-map import changed the binding set")) {
        return 0;
    }
    descriptor.buffer.component_count = 1;
    if (!expect(ctex_mesh_map_set_import_external(value->maps, &descriptor, &imported) ==
                    CTEX_RESULT_SUCCESS,
                "external AO import failed") ||
        !expect(imported.replaced_existing == 0 && imported.resolution_mismatch == 1 &&
                    imported.stale == 0 && imported.map_width == 2 && imported.map_height == 2 &&
                    imported.texture_set_width == 4 && imported.texture_set_height == 4,
                "external AO import report is incorrect")) {
        return 0;
    }
    pixels[0] = 255;
    if (!expect(ctex_mesh_map_set_sample(value->maps, CTEX_MESH_MAP_AMBIENT_OCCLUSION, 0.0, 1.0,
                                         &sample) == CTEX_RESULT_SUCCESS &&
                    sample.component_count == 1 && fabs(sample.values[0]) < 0.000001,
                "imported map did not retain an owned pixel copy") ||
        !expect(
            ctex_mesh_map_set_get_info(value->maps, &after, texture_set_id, sizeof(texture_set_id),
                                       uv_set, sizeof(uv_set)) == CTEX_RESULT_SUCCESS &&
                after.bound_map_count == 1 && after.resident_pixel_bytes > 0,
            "mesh-map memory accounting is incorrect") ||
        !expect(ctex_texture_set_get_memory_report(value->document, value->texture_set_id,
                                                   &document_memory) == CTEX_RESULT_SUCCESS &&
                    document_memory.mesh_map_pixel_bytes == after.resident_pixel_bytes &&
                    document_memory.total_resident_bytes >= after.resident_pixel_bytes,
                "document memory report omitted bound mesh maps")) {
        return 0;
    }
    return 1;
}

static int missing_maps_are_explicit(fixture* value) {
    const uint32_t required[] = {CTEX_MESH_MAP_AMBIENT_OCCLUSION, CTEX_MESH_MAP_CURVATURE};
    uint32_t missing = UINT32_MAX;
    char message[256] = {0};
    ctex_mesh_map_requirement_info info = {.size = CTEX_MESH_MAP_REQUIREMENT_INFO_CURRENT_SIZE};
    ctex_mesh_map_sample_info sample = {.size = CTEX_MESH_MAP_SAMPLE_INFO_CURRENT_SIZE};

    return expect(ctex_mesh_map_set_sample(value->maps, CTEX_MESH_MAP_CURVATURE, 0.5, 0.5,
                                           &sample) == CTEX_RESULT_MISSING_RESOURCE,
                  "sampling a missing map did not fail loudly") &&
           expect(ctex_get_last_diagnostic_code() == CTEX_DIAGNOSTIC_INVALID_MESH_MAP,
                  "missing-map diagnostic code is incorrect") &&
           expect(ctex_mesh_map_set_check_requirements(value->maps, "wear", required, 2, &missing,
                                                       1, NULL, 0, &info, message,
                                                       sizeof(message)) == CTEX_RESULT_SUCCESS,
                  "mesh-map requirement query failed") &&
           expect(info.required_missing_map_count == 1 && info.required_stale_map_count == 0 &&
                      missing == CTEX_MESH_MAP_CURVATURE && strstr(message, "curvature") != NULL,
                  "missing-map requirement report is incorrect");
}

static int normal_convention_and_tangent_contract(fixture* value) {
    const uint8_t normal_pixels[] = {128, 64, 255};
    ctex_tangent_frame_descriptor tangent = {
        .size = CTEX_TANGENT_FRAME_DESCRIPTOR_CURRENT_SIZE,
        .algorithm = CTEX_TANGENT_BASIS_UV_DERIVATIVE,
        .algorithm_version = 1,
        .normal_orientation = CTEX_TANGENT_NORMAL_VERTEX,
        .coordinate_handedness = CTEX_COORDINATE_RIGHT_HANDED,
        .uv_v_axis = CTEX_UV_V_AXIS_UPWARD,
        .handedness_encoding = CTEX_TANGENT_HANDEDNESS_W_SIGN,
        .uv_set = "uv0",
    };
    ctex_mesh_map_import_descriptor descriptor = {
        .size = CTEX_MESH_MAP_IMPORT_DESCRIPTOR_CURRENT_SIZE,
        .kind = CTEX_MESH_MAP_TANGENT_SPACE_NORMAL,
        .channel_meaning = CTEX_MESH_MAP_NORMAL_XYZ,
        .color_space = CTEX_COLOR_SPACE_LINEAR_REC709,
        .has_normal_convention = 1,
        .normal_convention = CTEX_MESH_MAP_NORMAL_DIRECTX,
        .tangent_frame = &tangent,
        .buffer =
            {
                .size = CTEX_MESH_MAP_PIXEL_BUFFER_DESCRIPTOR_CURRENT_SIZE,
                .width = 1,
                .height = 1,
                .component_type = CTEX_TRANSPORT_COMPONENT_UINT8_UNORM,
                .component_count = 3,
                .row_stride_bytes = 3,
                .pixels = normal_pixels,
                .pixel_bytes = sizeof(normal_pixels),
            },
    };
    ctex_mesh_map_import_info imported = {.size = CTEX_MESH_MAP_IMPORT_INFO_CURRENT_SIZE};
    ctex_mesh_map_sample_info sample = {.size = CTEX_MESH_MAP_SAMPLE_INFO_CURRENT_SIZE};
    ctex_mesh_map_entry_info entries[2] = {{0}};
    ctex_mesh_tangent_frame_info mesh_frame = {.size = CTEX_MESH_TANGENT_FRAME_INFO_CURRENT_SIZE};
    char mesh_frame_uv[16] = {0};
    size_t entry_count = 0;

    if (!expect(ctex_mesh_get_tangent_frame(value->mesh, &mesh_frame, mesh_frame_uv,
                                            sizeof(mesh_frame_uv)) == CTEX_RESULT_SUCCESS &&
                    mesh_frame.source == CTEX_TANGENT_FRAME_SUPPLIED &&
                    mesh_frame.corner_tangent_count == 3 &&
                    mesh_frame.algorithm == CTEX_TANGENT_BASIS_UV_DERIVATIVE &&
                    strcmp(mesh_frame_uv, "uv0") == 0,
                "supplied mesh tangent frame was not retained") ||
        !expect(ctex_mesh_map_set_import_external(value->maps, &descriptor, &imported) ==
                    CTEX_RESULT_SUCCESS,
                "DirectX tangent-space normal import failed") ||
        !expect(ctex_mesh_map_set_sample(value->maps, CTEX_MESH_MAP_TANGENT_SPACE_NORMAL, 0.5, 0.5,
                                         &sample) == CTEX_RESULT_SUCCESS &&
                    sample.component_count == 3 &&
                    fabs(sample.values[1] - (191.0 / 255.0)) < 0.000001,
                "DirectX normal was not converted to the canonical convention") ||
        !expect(ctex_mesh_map_set_get_entries(value->maps, entries, 2, &entry_count) ==
                        CTEX_RESULT_SUCCESS &&
                    entry_count == 2 && entries[0].kind == CTEX_MESH_MAP_TANGENT_SPACE_NORMAL &&
                    entries[0].has_normal_convention == 1 &&
                    entries[0].normal_convention == CTEX_MESH_MAP_NORMAL_DIRECTX &&
                    entries[0].has_tangent_frame == 1 &&
                    entries[0].tangent_algorithm == CTEX_TANGENT_BASIS_UV_DERIVATIVE,
                "normal-map convention or tangent metadata was not retained")) {
        return 0;
    }

    tangent.algorithm = CTEX_TANGENT_BASIS_MIKKTSPACE;
    return expect(ctex_mesh_map_set_import_external(value->maps, &descriptor, &imported) ==
                      CTEX_RESULT_INVALID_ARGUMENT,
                  "an incompatible tangent basis was accepted") &&
           expect(ctex_mesh_map_set_get_entries(value->maps, entries, 2, &entry_count) ==
                          CTEX_RESULT_SUCCESS &&
                      entry_count == 2,
                  "a refused normal-map import changed the binding set");
}

static int staleness_and_release(fixture* value) {
    ctex_mesh_info before_replacement = {.size = CTEX_MESH_INFO_CURRENT_SIZE};
    ctex_mesh_info after_refusal = {.size = CTEX_MESH_INFO_CURRENT_SIZE};
    ctex_mesh_tangent_data_descriptor invalid_tangents = value->tangent_data;
    ctex_mesh_map_staleness stale[2] = {{0}};
    ctex_mesh_map_entry_info entries[2] = {{0}};
    ctex_mesh_map_sample_info sample = {.size = CTEX_MESH_MAP_SAMPLE_INFO_CURRENT_SIZE};
    ctex_mesh_map_release_info released = {.size = CTEX_MESH_MAP_RELEASE_INFO_CURRENT_SIZE};
    ctex_mesh_map_set_info final = {.size = CTEX_MESH_MAP_SET_INFO_CURRENT_SIZE};
    ctex_texture_set_memory_report document_memory = {
        .size = CTEX_TEXTURE_SET_MEMORY_REPORT_CURRENT_SIZE};
    size_t stale_count = 0;
    size_t entry_count = 0;
    size_t normal_resident_bytes = 0;
    size_t ao_resident_bytes = 0;
    char texture_set_id[128] = {0};
    char uv_set[16] = {0};

    invalid_tangents.corner_tangent_count = 2;
    if (!expect(ctex_mesh_get_info(value->mesh, &before_replacement) == CTEX_RESULT_SUCCESS,
                "pre-replacement mesh query failed") ||
        !expect(
            ctex_mesh_replace_with_tangent_data(value->mesh, &value->mesh_descriptor,
                                                &invalid_tangents) == CTEX_RESULT_INVALID_ARGUMENT,
            "an incomplete per-corner tangent array was accepted") ||
        !expect(ctex_mesh_get_info(value->mesh, &after_refusal) == CTEX_RESULT_SUCCESS &&
                    after_refusal.revision == before_replacement.revision,
                "a refused tangent replacement changed the mesh revision") ||
        !expect(ctex_mesh_replace_with_tangent_data(value->mesh, &value->mesh_descriptor,
                                                    &value->tangent_data) == CTEX_RESULT_SUCCESS,
                "mesh replacement failed") ||
        !expect(ctex_mesh_map_set_synchronize_mesh(value->maps, value->mesh, NULL, 0,
                                                   &stale_count) == CTEX_RESULT_SUCCESS &&
                    stale_count == 2,
                "mesh-map staleness sizing failed") ||
        !expect(ctex_mesh_map_set_get_entries(value->maps, entries, 2, &entry_count) ==
                        CTEX_RESULT_SUCCESS &&
                    entries[0].stale == 0 && entries[1].stale == 0 &&
                    (normal_resident_bytes = entries[0].resident_pixel_bytes) > 0 &&
                    (ao_resident_bytes = entries[1].resident_pixel_bytes) > 0,
                "staleness sizing unexpectedly mutated the map set") ||
        !expect(ctex_mesh_map_set_synchronize_mesh(value->maps, value->mesh, stale, 2,
                                                   &stale_count) == CTEX_RESULT_SUCCESS &&
                    stale_count == 2 &&
                    stale[0].produced_mesh_revision < stale[0].current_mesh_revision,
                "mesh-map revision synchronization failed") ||
        !expect(ctex_mesh_map_set_sample(value->maps, CTEX_MESH_MAP_AMBIENT_OCCLUSION, 0.0, 0.0,
                                         &sample) == CTEX_RESULT_SUCCESS &&
                    sample.stale == 1 &&
                    sample.produced_mesh_revision < sample.current_mesh_revision,
                "stale mesh-map sample was not identified") ||
        !expect(ctex_mesh_map_set_release(value->maps, CTEX_MESH_MAP_AMBIENT_OCCLUSION,
                                          &released) == CTEX_RESULT_SUCCESS &&
                    released.released_map_count == 1 &&
                    released.resident_pixel_bytes_released == ao_resident_bytes,
                "single mesh-map release accounting failed") ||
        !expect(ctex_mesh_map_set_release_all(value->maps, &released) == CTEX_RESULT_SUCCESS &&
                    released.released_map_count == 1 &&
                    released.resident_pixel_bytes_released == normal_resident_bytes,
                "all-map release accounting failed") ||
        !expect(
            ctex_mesh_map_set_get_info(value->maps, &final, texture_set_id, sizeof(texture_set_id),
                                       uv_set, sizeof(uv_set)) == CTEX_RESULT_SUCCESS &&
                final.bound_map_count == 0 && final.resident_pixel_bytes == 0,
            "released mesh maps remain resident") ||
        !expect(ctex_texture_set_get_memory_report(value->document, value->texture_set_id,
                                                   &document_memory) == CTEX_RESULT_SUCCESS &&
                    document_memory.mesh_map_pixel_bytes == 0,
                "released mesh maps remain in document memory accounting")) {
        return 0;
    }
    return 1;
}

int main(void) {
    fixture value = {0};
    const int passed = expect(create_fixture(&value), "mesh-map C fixture creation failed") &&
                       inventory_and_external_import(&value) && missing_maps_are_explicit(&value) &&
                       normal_convention_and_tangent_contract(&value) &&
                       staleness_and_release(&value);
    destroy_fixture(&value);
    return passed ? 0 : 1;
}
