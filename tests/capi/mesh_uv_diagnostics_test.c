#include <ctex/capi.h>
#include <stdint.h>

static int expect(int condition) { return condition ? 1 : 0; }

int main(void) {
    const ctex_vec3f positions[6] = {{0.0F, 0.0F, 0.0F}, {1.0F, 0.0F, 0.0F}, {0.0F, 1.0F, 0.0F},
                                     {0.1F, 0.1F, 0.0F}, {0.7F, 0.1F, 0.0F}, {0.1F, 0.7F, 0.0F}};
    const ctex_vec3f normals[6] = {{0.0F, 0.0F, 1.0F}, {0.0F, 0.0F, 1.0F}, {0.0F, 0.0F, 1.0F},
                                   {0.0F, 0.0F, 1.0F}, {0.0F, 0.0F, 1.0F}, {0.0F, 0.0F, 1.0F}};
    ctex_vec2f uv[6] = {{0.0F, 0.0F}, {1.0F, 0.0F}, {0.0F, 1.0F},
                        {0.1F, 0.1F}, {0.7F, 0.1F}, {0.1F, 0.7F}};
    const uint32_t indices[6] = {0, 1, 2, 3, 4, 5};
    const uint32_t face_partitions[2] = {0, 0};
    const uint32_t face_materials[2] = {1, 1};
    const ctex_uv_set_descriptor uv_set = {CTEX_UV_SET_DESCRIPTOR_CURRENT_SIZE, "paint", uv, 6};
    const ctex_mesh_partition_descriptor partition = {CTEX_MESH_PARTITION_DESCRIPTOR_CURRENT_SIZE,
                                                      CTEX_PARTITION_SOURCE_MATERIAL, "body",
                                                      "Body"};
    const ctex_mesh_descriptor descriptor = {
        CTEX_MESH_DESCRIPTOR_CURRENT_SIZE,
        positions,
        6,
        normals,
        6,
        NULL,
        0,
        indices,
        6,
        &uv_set,
        1,
        "paint",
        &partition,
        1,
        face_partitions,
        2,
        face_materials,
        2,
    };
    ctex_mesh* mesh = NULL;
    ctex_mesh_uv_overlap_info info = {.size = CTEX_MESH_UV_OVERLAP_INFO_CURRENT_SIZE};
    ctex_mesh_uv_coverage_info coverage = {.size = CTEX_MESH_UV_COVERAGE_INFO_CURRENT_SIZE};
    uint32_t faces[2] = {UINT32_MAX, UINT32_MAX};
    uint32_t outside_face = UINT32_MAX;

    if (!expect(ctex_mesh_create(&descriptor, &mesh) == CTEX_RESULT_SUCCESS && mesh != NULL)) {
        return 1;
    }
    info.size = 0;
    if (!expect(ctex_mesh_analyze_uv_overlaps(mesh, "paint", 0, &info, NULL, 0) ==
                CTEX_RESULT_INVALID_ARGUMENT)) {
        ctex_mesh_destroy(mesh);
        return 1;
    }
    info.size = CTEX_MESH_UV_OVERLAP_INFO_CURRENT_SIZE;
    if (!expect(ctex_mesh_analyze_uv_overlaps(mesh, "paint", 0, &info, NULL, 0) ==
                CTEX_RESULT_SUCCESS) ||
        !expect(info.required_face_count == 2 && info.overlap_pair_count == 1 &&
                info.candidate_pair_count == 1) ||
        !expect(ctex_mesh_analyze_uv_overlaps(mesh, "paint", 0, &info, faces, 1) ==
                CTEX_RESULT_BUFFER_TOO_SMALL) ||
        !expect(ctex_mesh_analyze_uv_overlaps(mesh, "paint", 0, &info, faces, 2) ==
                CTEX_RESULT_SUCCESS) ||
        !expect(faces[0] == 0 && faces[1] == 1) ||
        !expect(ctex_mesh_analyze_uv_overlaps(mesh, "missing", 0, &info, NULL, 0) ==
                CTEX_RESULT_MISSING_RESOURCE) ||
        !expect(ctex_mesh_analyze_uv_overlaps(mesh, "paint", 1, &info, NULL, 0) ==
                CTEX_RESULT_INVALID_ARGUMENT) ||
        !expect(ctex_mesh_analyze_uv_coverage(mesh, "paint", 0, 4, 4, &coverage, NULL, 0) ==
                CTEX_RESULT_SUCCESS) ||
        !expect(coverage.selected_face_count == 2 && coverage.covered_texel_count == 10 &&
                coverage.uncovered_texel_count == 6 && coverage.uncovered_fraction == 0.375 &&
                coverage.required_outside_face_count == 0) ||
        !expect(ctex_mesh_analyze_uv_coverage(mesh, "paint", 0, 0, 4, &coverage, NULL, 0) ==
                CTEX_RESULT_INVALID_ARGUMENT) ||
        !expect(ctex_mesh_analyze_uv_coverage(mesh, "paint", 0, 16385, 16384, &coverage, NULL, 0) ==
                CTEX_RESULT_OVER_BUDGET) ||
        !expect(ctex_mesh_analyze_uv_coverage(mesh, "missing", 0, 4, 4, &coverage, NULL, 0) ==
                CTEX_RESULT_MISSING_RESOURCE)) {
        ctex_mesh_destroy(mesh);
        return 1;
    }

    uv[1].x = 1.25F;
    if (!expect(ctex_mesh_replace(mesh, &descriptor) == CTEX_RESULT_SUCCESS) ||
        !expect(ctex_mesh_analyze_uv_coverage(mesh, "paint", 0, 4, 4, &coverage, NULL, 0) ==
                CTEX_RESULT_SUCCESS) ||
        !expect(coverage.required_outside_face_count == 1) ||
        !expect(ctex_mesh_analyze_uv_coverage(mesh, "paint", 0, 4, 4, &coverage, &outside_face,
                                              0) == CTEX_RESULT_BUFFER_TOO_SMALL) ||
        !expect(ctex_mesh_analyze_uv_coverage(mesh, "paint", 0, 4, 4, &coverage, &outside_face,
                                              1) == CTEX_RESULT_SUCCESS) ||
        !expect(outside_face == 0)) {
        ctex_mesh_destroy(mesh);
        return 1;
    }
    ctex_mesh_destroy(mesh);
    return 0;
}
