#include <ctex/capi.h>
#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

static int expect(int condition) { return condition ? 1 : 0; }

static int near(double left, double right) { return fabs(left - right) < 1.0e-9; }

static ctex_mesh* coverage_mesh(void) {
    static const ctex_vec3f positions[4] = {
        {0.0f, 0.0f, 0.0f}, {10.0f, 0.0f, 0.0f}, {10.0f, 1.0f, 0.0f}, {0.0f, 1.0f, 0.0f}};
    static const ctex_vec3f normals[4] = {
        {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 1.0f}};
    static const ctex_vec2f uv[4] = {{0.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 1.0f}, {0.0f, 1.0f}};
    static const uint32_t triangles[6] = {0, 1, 2, 0, 2, 3};
    static const uint32_t face_partitions[2] = {0, 0};
    static const uint32_t face_materials[2] = {0, 0};
    static const ctex_uv_set_descriptor uv_set = {CTEX_UV_SET_DESCRIPTOR_CURRENT_SIZE, "uv0", uv,
                                                  4};
    static const ctex_mesh_partition_descriptor partition = {
        CTEX_MESH_PARTITION_DESCRIPTOR_CURRENT_SIZE, CTEX_PARTITION_SOURCE_MATERIAL, "material:0",
        "Material"};
    const ctex_mesh_descriptor descriptor = {
        .size = CTEX_MESH_DESCRIPTOR_CURRENT_SIZE,
        .positions = positions,
        .position_count = 4,
        .normals = normals,
        .normal_count = 4,
        .triangle_indices = triangles,
        .triangle_index_count = 6,
        .uv_sets = &uv_set,
        .uv_set_count = 1,
        .default_uv_set = "uv0",
        .partitions = &partition,
        .partition_count = 1,
        .face_partition_indices = face_partitions,
        .face_partition_index_count = 2,
        .face_material_ids = face_materials,
        .face_material_id_count = 2,
    };
    ctex_mesh* mesh = NULL;
    return ctex_mesh_create(&descriptor, &mesh) == CTEX_RESULT_SUCCESS ? mesh : NULL;
}

static ctex_resolved_stamp stamp(double x, uint64_t ordinal) {
    const ctex_resolved_stamp value = {
        .position = {x, 0.5, 0.0},
        .frame = {{1.0, 0.0, 0.0}, {0.0, 1.0, 0.0}, {0.0, 0.0, 1.0}},
        .radius = 2.0,
        .opacity = 0.25,
        .hardness = 1.0,
        .rotation_radians = 0.0,
        .elongation = 1.0,
        .flow = 0.4,
        .tip_resource_identity = "builtin.circle",
        .source_ordinal = ordinal,
        .symmetry_instance = 0,
        .ordinal = ordinal,
    };
    return value;
}

static int continuous_and_external_strokes_produce_tile_coverage(void) {
    ctex_mesh* mesh = coverage_mesh();
    const ctex_paint_tile_coverage_descriptor tile = {
        CTEX_PAINT_TILE_COVERAGE_DESCRIPTOR_CURRENT_SIZE, "uv0", 3, 1, {0.0, 0.0}};
    ctex_resolved_stamp stamps[2] = {stamp(0.0, 0), stamp(10.0, 1)};
    const ctex_swept_segment segment = {0, 1};
    ctex_resolved_stroke_descriptor stroke = {
        .size = CTEX_RESOLVED_STROKE_DESCRIPTOR_CURRENT_SIZE,
        .reconstruction_version = 1,
        .tip_mode = CTEX_STROKE_TIP_CONTINUOUS_SWEEP,
        .symmetry_instance_count = 1,
        .stamps = stamps,
        .stamp_count = 2,
        .swept_segments = &segment,
        .swept_segment_count = 1,
    };
    double coverage[3] = {-1.0, -1.0, -1.0};
    size_t count = 0;
    int passed = mesh != NULL;
    if (passed) {
        passed = expect(ctex_paint_evaluate_tile_coverage(mesh, &tile, &stroke, NULL, 0, &count) ==
                        CTEX_RESULT_SUCCESS) &&
                 expect(count == 3);
    }
    if (passed) {
        passed =
            expect(ctex_paint_evaluate_tile_coverage(mesh, &tile, &stroke, coverage, 2, &count) ==
                   CTEX_RESULT_BUFFER_TOO_SMALL) &&
            expect(coverage[0] == -1.0 && coverage[1] == -1.0 && coverage[2] == -1.0);
    }
    if (passed) {
        passed = expect(ctex_paint_evaluate_tile_coverage(mesh, &tile, &stroke, coverage, 3,
                                                          &count) == CTEX_RESULT_SUCCESS) &&
                 expect(near(coverage[0], 1.0) && near(coverage[1], 1.0) && near(coverage[2], 1.0));
    }
    ctex_mesh_destroy(mesh);
    return passed;
}

static int discrete_tips_do_not_synthesize_sweeps(void) {
    ctex_mesh* mesh = coverage_mesh();
    const ctex_paint_tile_coverage_descriptor tile = {
        CTEX_PAINT_TILE_COVERAGE_DESCRIPTOR_CURRENT_SIZE, "uv0", 3, 1, {0.0, 0.0}};
    const ctex_resolved_stamp stamps[2] = {stamp(0.0, 0), stamp(10.0, 1)};
    const ctex_resolved_stroke_descriptor stroke = {
        .size = CTEX_RESOLVED_STROKE_DESCRIPTOR_CURRENT_SIZE,
        .reconstruction_version = 1,
        .tip_mode = CTEX_STROKE_TIP_DISCRETE_ALPHA,
        .symmetry_instance_count = 1,
        .stamps = stamps,
        .stamp_count = 2,
    };
    double coverage[3] = {0.0, 0.0, 0.0};
    size_t count = 0;
    const int passed =
        mesh != NULL &&
        expect(ctex_paint_evaluate_tile_coverage(mesh, &tile, &stroke, coverage, 3, &count) ==
               CTEX_RESULT_SUCCESS) &&
        expect(near(coverage[0], 1.0) && near(coverage[1], 0.0) && near(coverage[2], 1.0));
    ctex_mesh_destroy(mesh);
    return passed;
}

static int invalid_inputs_are_stable_diagnostics(void) {
    ctex_mesh* mesh = coverage_mesh();
    ctex_paint_tile_coverage_descriptor tile = {
        CTEX_PAINT_TILE_COVERAGE_DESCRIPTOR_CURRENT_SIZE, "missing", 3, 1, {0.0, 0.0}};
    ctex_resolved_stamp invalid_stamp = stamp(0.0, 4);
    const ctex_resolved_stroke_descriptor stroke = {
        .size = CTEX_RESOLVED_STROKE_DESCRIPTOR_CURRENT_SIZE,
        .reconstruction_version = 1,
        .tip_mode = CTEX_STROKE_TIP_DISCRETE_ALPHA,
        .symmetry_instance_count = 1,
        .stamps = &invalid_stamp,
        .stamp_count = 1,
    };
    double coverage[3] = {0.0, 0.0, 0.0};
    size_t count = 0;
    int passed = mesh != NULL;
    if (passed) {
        passed =
            expect(ctex_paint_evaluate_tile_coverage(mesh, &tile, &stroke, coverage, 3, &count) ==
                   CTEX_RESULT_MISSING_RESOURCE) &&
            expect(ctex_get_last_diagnostic_code() == CTEX_DIAGNOSTIC_MISSING_UV_SET);
    }
    tile.uv_set = "uv0";
    if (passed) {
        passed =
            expect(ctex_paint_evaluate_tile_coverage(mesh, &tile, &stroke, coverage, 3, &count) ==
                   CTEX_RESULT_INVALID_ARGUMENT) &&
            expect(ctex_get_last_diagnostic_code() == CTEX_DIAGNOSTIC_INVALID_STROKE);
    }
    tile.width = (uint32_t)CTEX_MAX_PAINT_TILE_TEXEL_COUNT + 1U;
    if (passed) {
        passed = expect(ctex_paint_evaluate_tile_coverage(mesh, &tile, &stroke, coverage, 3,
                                                          &count) == CTEX_RESULT_OVER_BUDGET) &&
                 expect(ctex_get_last_diagnostic_code() == CTEX_DIAGNOSTIC_PAINT_LIMIT_EXCEEDED);
    }
    ctex_mesh_destroy(mesh);
    return passed;
}

int main(void) {
    return continuous_and_external_strokes_produce_tile_coverage() &&
                   discrete_tips_do_not_synthesize_sweeps() &&
                   invalid_inputs_are_stable_diagnostics()
               ? 0
               : 1;
}
