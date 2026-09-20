#include <ctex/capi.h>
#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

static int expect(int condition) { return condition ? 1 : 0; }

static int near(double left, double right) { return fabs(left - right) < 1.0e-9; }

static int near_float(float left, float right) { return fabsf(left - right) < 1.0e-6f; }

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

static ctex_mesh* selection_mesh(void) {
    static const ctex_vec3f positions[9] = {
        {-0.8F, -0.2F, 0.0F}, {-0.4F, -0.2F, 0.0F}, {-0.6F, 0.2F, 0.0F},
        {-0.2F, 0.0F, 0.0F},  {0.2F, 0.0F, 0.0F},   {0.0F, 0.4F, 0.0F},
        {0.4F, -0.2F, 0.0F},  {0.8F, -0.2F, 0.0F},  {0.6F, 0.2F, 0.0F}};
    static const ctex_vec3f normals[9] = {{0, 0, 1}, {0, 0, 1}, {0, 0, 1}, {0, 0, 1}, {0, 0, 1},
                                          {0, 0, 1}, {0, 0, 1}, {0, 0, 1}, {0, 0, 1}};
    static const ctex_vec2f uv[9] = {0};
    static const uint32_t triangles[9] = {0, 1, 2, 3, 4, 5, 6, 7, 8};
    static const uint32_t face_partitions[3] = {0, 0, 0};
    static const uint32_t face_materials[3] = {0, 0, 0};
    static const ctex_uv_set_descriptor uv_set = {CTEX_UV_SET_DESCRIPTOR_CURRENT_SIZE, "paint", uv,
                                                  9};
    static const ctex_mesh_partition_descriptor partition = {
        CTEX_MESH_PARTITION_DESCRIPTOR_CURRENT_SIZE, CTEX_PARTITION_SOURCE_OBJECT, "selection",
        "Selection"};
    const ctex_mesh_descriptor descriptor = {.size = CTEX_MESH_DESCRIPTOR_CURRENT_SIZE,
                                             .positions = positions,
                                             .position_count = 9,
                                             .normals = normals,
                                             .normal_count = 9,
                                             .triangle_indices = triangles,
                                             .triangle_index_count = 9,
                                             .uv_sets = &uv_set,
                                             .uv_set_count = 1,
                                             .default_uv_set = "paint",
                                             .partitions = &partition,
                                             .partition_count = 1,
                                             .face_partition_indices = face_partitions,
                                             .face_partition_index_count = 3,
                                             .face_material_ids = face_materials,
                                             .face_material_id_count = 3};
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

static int material_coordinate_modes_are_exposed_per_texel(void) {
    ctex_mesh* mesh = coverage_mesh();
    const ctex_paint_tile_coverage_descriptor tile = {
        CTEX_PAINT_TILE_COVERAGE_DESCRIPTOR_CURRENT_SIZE, "uv0", 1, 1, {0.0, 0.0}};
    ctex_paint_material_coordinate_descriptor descriptor = {
        .size = CTEX_PAINT_MATERIAL_COORDINATE_DESCRIPTOR_CURRENT_SIZE,
        .mode = CTEX_PAINT_MATERIAL_COORDINATE_UV,
    };
    ctex_paint_material_coordinate_sample sample = {.covered = 7};
    size_t count = 0;
    int passed = mesh != NULL;
    if (passed) {
        passed = expect(ctex_paint_evaluate_material_coordinates(mesh, &tile, &descriptor, NULL, 0,
                                                                 &count) == CTEX_RESULT_SUCCESS) &&
                 expect(count == 1);
    }
    if (passed) {
        passed = expect(ctex_paint_evaluate_material_coordinates(mesh, &tile, &descriptor, &sample,
                                                                 0, &count) ==
                        CTEX_RESULT_BUFFER_TOO_SMALL) &&
                 expect(sample.covered == 7);
    }
    if (passed) {
        passed = expect(ctex_paint_evaluate_material_coordinates(
                            mesh, &tile, &descriptor, &sample, 1, &count) == CTEX_RESULT_SUCCESS) &&
                 expect(sample.covered == 1 && sample.projection_count == 1 &&
                        near(sample.coordinates[0].x, 0.5) && near(sample.coordinates[0].y, 0.5) &&
                        near(sample.weights[0], 1.0));
    }
    descriptor.mode = CTEX_PAINT_MATERIAL_COORDINATE_TRIPLANAR;
    if (passed) {
        passed = expect(ctex_paint_evaluate_material_coordinates(
                            mesh, &tile, &descriptor, &sample, 1, &count) == CTEX_RESULT_SUCCESS) &&
                 expect(sample.projection_count == 3 && near(sample.coordinates[0].x, 0.5) &&
                        near(sample.coordinates[0].y, 0.0) && near(sample.coordinates[1].x, 5.0) &&
                        near(sample.coordinates[1].y, 0.0) && near(sample.coordinates[2].x, 5.0) &&
                        near(sample.coordinates[2].y, 0.5) && near(sample.weights[0], 0.0) &&
                        near(sample.weights[1], 0.0) && near(sample.weights[2], 1.0));
    }
    descriptor.mode = CTEX_PAINT_MATERIAL_COORDINATE_PLANAR;
    descriptor.planar_u_axis = (ctex_vec3d){1.0, 0.0, 0.0};
    descriptor.planar_v_axis = (ctex_vec3d){0.0, 1.0, 0.0};
    if (passed) {
        passed = expect(ctex_paint_evaluate_material_coordinates(
                            mesh, &tile, &descriptor, &sample, 1, &count) == CTEX_RESULT_SUCCESS) &&
                 expect(sample.projection_count == 1 && near(sample.coordinates[0].x, 5.0) &&
                        near(sample.coordinates[0].y, 0.5));
    }
    descriptor.planar_v_axis = descriptor.planar_u_axis;
    sample.covered = 9;
    if (passed) {
        passed =
            expect(ctex_paint_evaluate_material_coordinates(mesh, &tile, &descriptor, &sample, 1,
                                                            &count) ==
                   CTEX_RESULT_INVALID_ARGUMENT) &&
            expect(ctex_get_last_diagnostic_code() == CTEX_DIAGNOSTIC_INVALID_PAINT_COORDINATES) &&
            expect(sample.covered == 9);
    }
    ctex_mesh_destroy(mesh);
    return passed;
}

static int uncovered_material_texels_have_no_projections(void) {
    ctex_mesh* mesh = coverage_mesh();
    const ctex_paint_tile_coverage_descriptor tile = {
        CTEX_PAINT_TILE_COVERAGE_DESCRIPTOR_CURRENT_SIZE, "uv0", 1, 1, {2.0, 0.0}};
    const ctex_paint_material_coordinate_descriptor descriptor = {
        .size = CTEX_PAINT_MATERIAL_COORDINATE_DESCRIPTOR_CURRENT_SIZE,
        .mode = CTEX_PAINT_MATERIAL_COORDINATE_UV,
    };
    ctex_paint_material_coordinate_sample sample = {.covered = 7, .projection_count = 7};
    size_t count = 0;
    const int passed =
        mesh != NULL &&
        expect(ctex_paint_evaluate_material_coordinates(mesh, &tile, &descriptor, &sample, 1,
                                                        &count) == CTEX_RESULT_SUCCESS) &&
        expect(count == 1 && sample.covered == 0 && sample.projection_count == 0);
    ctex_mesh_destroy(mesh);
    return passed;
}

static ctex_resolved_stroke_descriptor one_stamp_descriptor(ctex_resolved_stamp* value) {
    const ctex_resolved_stroke_descriptor stroke = {
        .size = CTEX_RESOLVED_STROKE_DESCRIPTOR_CURRENT_SIZE,
        .reconstruction_version = 1,
        .tip_mode = CTEX_STROKE_TIP_DISCRETE_ALPHA,
        .symmetry_instance_count = 1,
        .stamps = value,
        .stamp_count = 1,
    };
    return stroke;
}

static int depth_rejection_defaults_are_exposed(void) {
    ctex_mesh* mesh = coverage_mesh();
    const ctex_paint_tile_coverage_descriptor tile = {
        CTEX_PAINT_TILE_COVERAGE_DESCRIPTOR_CURRENT_SIZE, "uv0", 1, 1, {0.0, 0.0}};
    ctex_resolved_stamp resolved_stamp = stamp(5.0, 0);
    const ctex_resolved_stroke_descriptor stroke = one_stamp_descriptor(&resolved_stamp);
    const ctex_vec2d screen_position = {0.5, 0.5};
    const double surface_depth = 0.7;
    const double visible_depth = 0.5;
    const ctex_paint_depth_context_descriptor context = {
        .size = CTEX_PAINT_DEPTH_CONTEXT_DESCRIPTOR_CURRENT_SIZE,
        .viewport_width = 1,
        .viewport_height = 1,
        .screen_positions = &screen_position,
        .surface_depth = &surface_depth,
        .surface_sample_count = 1,
        .visible_depth = &visible_depth,
        .visible_depth_count = 1,
        .transform_consistent = 1,
    };
    ctex_paint_rejection_descriptor rejection;
    ctex_paint_rejection_info info = {.size = CTEX_PAINT_REJECTION_INFO_CURRENT_SIZE};
    double coverage = -1.0;
    size_t count = 0;
    int passed =
        mesh != NULL && expect(ctex_paint_rejection_init(&rejection) == CTEX_RESULT_SUCCESS);
    rejection.depth_contexts = &context;
    rejection.depth_context_count = 1;
    if (passed) {
        passed =
            expect(ctex_paint_evaluate_rejected_coverage(mesh, &tile, &stroke, &rejection, &info,
                                                         NULL, 0, &count) == CTEX_RESULT_SUCCESS) &&
            expect(
                count == 1 && info.depth_disposition == CTEX_PAINT_DEPTH_CONSISTENT_PER_INSTANCE &&
                info.depth_rejected_contributions == 1 && near(info.resolved_depth_bias, 1.0e-4) &&
                near(info.resolved_minimum_normal_dot, 0.5));
    }
    info.depth_rejected_contributions = 9;
    if (passed) {
        passed = expect(ctex_paint_evaluate_rejected_coverage(mesh, &tile, &stroke, &rejection,
                                                              &info, &coverage, 0, &count) ==
                        CTEX_RESULT_BUFFER_TOO_SMALL) &&
                 expect(coverage == -1.0 && info.depth_rejected_contributions == 9);
    }
    if (passed) {
        passed = expect(ctex_paint_evaluate_rejected_coverage(mesh, &tile, &stroke, &rejection,
                                                              &info, &coverage, 1,
                                                              &count) == CTEX_RESULT_SUCCESS) &&
                 expect(near(coverage, 0.0) && info.depth_rejected_contributions == 1);
    }
    ctex_mesh_destroy(mesh);
    return passed;
}

static int angle_and_backface_rejection_are_exposed(void) {
    ctex_mesh* mesh = coverage_mesh();
    const ctex_paint_tile_coverage_descriptor tile = {
        CTEX_PAINT_TILE_COVERAGE_DESCRIPTOR_CURRENT_SIZE, "uv0", 1, 1, {0.0, 0.0}};
    ctex_resolved_stamp resolved_stamp = stamp(5.0, 0);
    resolved_stamp.frame = (ctex_stroke_frame){{0.0, 1.0, 0.0}, {0.0, 0.0, 1.0}, {1.0, 0.0, 0.0}};
    const ctex_resolved_stroke_descriptor stroke = one_stamp_descriptor(&resolved_stamp);
    ctex_paint_rejection_descriptor rejection;
    ctex_paint_rejection_info info = {.size = CTEX_PAINT_REJECTION_INFO_CURRENT_SIZE};
    double coverage = -1.0;
    size_t count = 0;
    int passed =
        mesh != NULL && expect(ctex_paint_rejection_init(&rejection) == CTEX_RESULT_SUCCESS);
    rejection.depth_enabled = 0;
    if (passed) {
        passed = expect(ctex_paint_evaluate_rejected_coverage(mesh, &tile, &stroke, &rejection,
                                                              &info, &coverage, 1,
                                                              &count) == CTEX_RESULT_SUCCESS) &&
                 expect(near(coverage, 0.0) && info.angle_rejected_contributions == 1);
    }
    resolved_stamp.frame = (ctex_stroke_frame){{1.0, 0.0, 0.0}, {0.0, 1.0, 0.0}, {0.0, 0.0, 1.0}};
    const ctex_vec3d away_view = {0.0, 0.0, -1.0};
    rejection.angle_enabled = 0;
    rejection.backface_enabled = 1;
    rejection.view_directions = &away_view;
    rejection.view_direction_count = 1;
    if (passed) {
        passed = expect(ctex_paint_evaluate_rejected_coverage(mesh, &tile, &stroke, &rejection,
                                                              &info, &coverage, 1,
                                                              &count) == CTEX_RESULT_SUCCESS) &&
                 expect(near(coverage, 0.0) && info.backface_rejected_texels == 1 &&
                        info.depth_disposition == CTEX_PAINT_DEPTH_DISABLED_BY_OPERATION);
    }
    ctex_mesh_destroy(mesh);
    return passed;
}

static int derived_symmetry_depth_policy_is_reported(void) {
    ctex_mesh* mesh = coverage_mesh();
    const ctex_paint_tile_coverage_descriptor tile = {
        CTEX_PAINT_TILE_COVERAGE_DESCRIPTOR_CURRENT_SIZE, "uv0", 1, 1, {0.0, 0.0}};
    ctex_resolved_stamp stamps[2] = {stamp(10.0, 0), stamp(5.0, 1)};
    stamps[1].source_ordinal = 0;
    stamps[1].symmetry_instance = 1;
    const ctex_resolved_stroke_descriptor stroke = {
        .size = CTEX_RESOLVED_STROKE_DESCRIPTOR_CURRENT_SIZE,
        .reconstruction_version = 1,
        .tip_mode = CTEX_STROKE_TIP_DISCRETE_ALPHA,
        .symmetry_instance_count = 2,
        .stamps = stamps,
        .stamp_count = 2,
    };
    const ctex_vec2d screen_position = {0.5, 0.5};
    const double surface_depth = 0.7;
    const double visible_depth = 0.5;
    const ctex_paint_depth_context_descriptor context = {
        .size = CTEX_PAINT_DEPTH_CONTEXT_DESCRIPTOR_CURRENT_SIZE,
        .symmetry_instance = 0,
        .viewport_width = 1,
        .viewport_height = 1,
        .screen_positions = &screen_position,
        .surface_depth = &surface_depth,
        .surface_sample_count = 1,
        .visible_depth = &visible_depth,
        .visible_depth_count = 1,
        .transform_consistent = 1,
    };
    ctex_paint_rejection_descriptor rejection;
    ctex_paint_rejection_info info = {.size = CTEX_PAINT_REJECTION_INFO_CURRENT_SIZE};
    double coverage = 0.0;
    size_t count = 0;
    int passed =
        mesh != NULL && expect(ctex_paint_rejection_init(&rejection) == CTEX_RESULT_SUCCESS);
    rejection.symmetry_depth_policy = CTEX_PAINT_SYMMETRY_DEPTH_DISABLE_DERIVED;
    rejection.depth_contexts = &context;
    rejection.depth_context_count = 1;
    if (passed) {
        passed = expect(ctex_paint_evaluate_rejected_coverage(mesh, &tile, &stroke, &rejection,
                                                              &info, &coverage, 1,
                                                              &count) == CTEX_RESULT_SUCCESS) &&
                 expect(near(coverage, 1.0) &&
                        info.depth_disposition == CTEX_PAINT_DEPTH_DISABLED_FOR_DERIVED_SYMMETRY);
    }
    ctex_mesh_destroy(mesh);
    return passed;
}

static int rejection_parameters_are_clamped_and_invalid_flags_are_transactional(void) {
    ctex_mesh* mesh = coverage_mesh();
    const ctex_paint_tile_coverage_descriptor tile = {
        CTEX_PAINT_TILE_COVERAGE_DESCRIPTOR_CURRENT_SIZE, "uv0", 1, 1, {0.0, 0.0}};
    ctex_resolved_stamp resolved_stamp = stamp(5.0, 0);
    const ctex_resolved_stroke_descriptor stroke = one_stamp_descriptor(&resolved_stamp);
    ctex_paint_rejection_descriptor rejection;
    ctex_paint_rejection_info info = {.size = CTEX_PAINT_REJECTION_INFO_CURRENT_SIZE};
    double coverage = -1.0;
    size_t count = 0;
    int passed =
        mesh != NULL && expect(ctex_paint_rejection_init(&rejection) == CTEX_RESULT_SUCCESS);
    rejection.depth_enabled = 0;
    rejection.depth_bias = -1.0;
    rejection.minimum_normal_dot = 2.0;
    if (passed) {
        passed = expect(ctex_paint_evaluate_rejected_coverage(mesh, &tile, &stroke, &rejection,
                                                              &info, &coverage, 1,
                                                              &count) == CTEX_RESULT_SUCCESS) &&
                 expect(near(coverage, 1.0) && near(info.resolved_depth_bias, 0.0) &&
                        near(info.resolved_minimum_normal_dot, 1.0) &&
                        info.depth_bias_clamped == 1 && info.minimum_normal_dot_clamped == 1);
    }
    rejection.backface_enabled = 2;
    coverage = 7.0;
    info.backface_rejected_texels = 9;
    if (passed) {
        passed =
            expect(ctex_paint_evaluate_rejected_coverage(mesh, &tile, &stroke, &rejection, &info,
                                                         &coverage, 1,
                                                         &count) == CTEX_RESULT_INVALID_ARGUMENT) &&
            expect(ctex_get_last_diagnostic_code() == CTEX_DIAGNOSTIC_INVALID_PAINT_REJECTION) &&
            expect(coverage == 7.0 && info.backface_rejected_texels == 9);
    }
    ctex_mesh_destroy(mesh);
    return passed;
}

static int deposition_mask_extension_is_append_only(
    ctex_mesh* mesh, const ctex_paint_tile_coverage_descriptor* tile,
    const ctex_resolved_stroke_descriptor* stroke, ctex_paint_deposition_descriptor* deposition,
    ctex_paint_deposition_info* info, ctex_paint_deposition_sample* sample, size_t* count) {
    const double layer_value = 0.5;
    const double screen_value = 0.5;
    const ctex_paint_mask_view layer = {&layer_value, 1};
    const ctex_paint_mask_view screen = {&screen_value, 1};
    const ctex_paint_mask_inputs_descriptor masks = {
        .size = CTEX_PAINT_MASK_INPUTS_DESCRIPTOR_CURRENT_SIZE,
        .active_layer_masks = &layer,
        .active_layer_mask_count = 1,
        .screen_selection = &screen,
    };
    deposition->masks = &masks;
    deposition->size = CTEX_PAINT_DEPOSITION_DESCRIPTOR_V1_SIZE;
    int passed =
        expect(ctex_paint_evaluate_tile_deposition(mesh, tile, stroke, deposition, info, sample, 1,
                                                   count) == CTEX_RESULT_SUCCESS) &&
        expect(near(sample->strength, 0.1));
    deposition->size = CTEX_PAINT_DEPOSITION_DESCRIPTOR_CURRENT_SIZE;
    if (passed) {
        passed =
            expect(ctex_paint_evaluate_tile_deposition(mesh, tile, stroke, deposition, info, sample,
                                                       1, count) == CTEX_RESULT_SUCCESS) &&
            expect(near(sample->non_building_coverage, 0.25) && near(sample->strength, 0.025));
    }
    deposition->masks = NULL;
    return passed;
}

static int deposition_accumulates_and_discards_canonical_stamps(void) {
    ctex_mesh* mesh = coverage_mesh();
    const ctex_paint_tile_coverage_descriptor tile = {
        CTEX_PAINT_TILE_COVERAGE_DESCRIPTOR_CURRENT_SIZE, "uv0", 1, 1, {0.0, 0.0}};
    ctex_resolved_stamp stamps[3] = {stamp(5.0, 0), stamp(5.0, 1), stamp(5.0, 2)};
    size_t index = 0;
    for (index = 0; index < 3; ++index) {
        stamps[index].opacity = 0.5;
        stamps[index].flow = 0.2;
    }
    const ctex_resolved_stroke_descriptor stroke = {
        .size = CTEX_RESOLVED_STROKE_DESCRIPTOR_CURRENT_SIZE,
        .reconstruction_version = 1,
        .tip_mode = CTEX_STROKE_TIP_DISCRETE_ALPHA,
        .symmetry_instance_count = 1,
        .stamps = stamps,
        .stamp_count = 3,
    };
    ctex_paint_deposition_descriptor deposition = {
        .size = CTEX_PAINT_DEPOSITION_DESCRIPTOR_CURRENT_SIZE,
        .mode = CTEX_PAINT_DEPOSITION_NON_BUILDING,
        .alpha_discard_format = CTEX_ALPHA_DISCARD_UNORM8,
        .has_custom_alpha_discard_threshold = 1,
        .custom_alpha_discard_threshold = 0.25,
    };
    ctex_paint_deposition_info info = {.size = CTEX_PAINT_DEPOSITION_INFO_CURRENT_SIZE};
    ctex_paint_deposition_sample sample = {-1.0, -1.0, -1.0, -1.0, 7};
    size_t count = 0;
    int passed = mesh != NULL;
    if (passed) {
        passed =
            expect(ctex_paint_evaluate_tile_deposition(mesh, &tile, &stroke, &deposition, &info,
                                                       NULL, 0, &count) == CTEX_RESULT_SUCCESS) &&
            expect(count == 1 && info.applied_stamp_count == 3 &&
                   near(info.alpha_discard_threshold, 0.25));
    }
    if (passed) {
        passed = expect(ctex_paint_evaluate_tile_deposition(mesh, &tile, &stroke, &deposition,
                                                            &info, &sample, 0, &count) ==
                        CTEX_RESULT_BUFFER_TOO_SMALL) &&
                 expect(sample.write == 7 && sample.strength == -1.0);
    }
    if (passed) {
        passed = expect(ctex_paint_evaluate_tile_deposition(mesh, &tile, &stroke, &deposition,
                                                            &info, &sample, 1,
                                                            &count) == CTEX_RESULT_SUCCESS) &&
                 expect(near(sample.non_building_coverage, 1.0) &&
                        near(sample.build_up_deposition, 0.0) && near(sample.strength, 0.1) &&
                        near(sample.retained_strength, 0.1) && sample.write == 0);
    }
    if (passed) {
        passed = deposition_mask_extension_is_append_only(mesh, &tile, &stroke, &deposition, &info,
                                                          &sample, &count);
    }
    deposition.mode = CTEX_PAINT_DEPOSITION_BUILD_UP;
    deposition.alpha_discard_format = CTEX_ALPHA_DISCARD_FLOATING_POINT;
    deposition.has_custom_alpha_discard_threshold = 0;
    if (passed) {
        passed = expect(ctex_paint_evaluate_tile_deposition(mesh, &tile, &stroke, &deposition,
                                                            &info, &sample, 1,
                                                            &count) == CTEX_RESULT_SUCCESS) &&
                 expect(info.mode == CTEX_PAINT_DEPOSITION_BUILD_UP &&
                        near(info.alpha_discard_threshold, 0.004) &&
                        near(sample.non_building_coverage, 0.0) &&
                        near(sample.build_up_deposition, 0.488) && near(sample.strength, 0.244) &&
                        near(sample.retained_strength, 0.244) && sample.write == 1);
    }
    deposition.mode = CTEX_PAINT_DEPOSITION_NON_BUILDING;
    deposition.has_custom_alpha_discard_threshold = 1;
    deposition.custom_alpha_discard_threshold = 2.0;
    if (passed) {
        passed = expect(ctex_paint_evaluate_tile_deposition(mesh, &tile, &stroke, &deposition,
                                                            &info, &sample, 1,
                                                            &count) == CTEX_RESULT_SUCCESS) &&
                 expect(near(info.alpha_discard_threshold, 1.0) &&
                        info.alpha_discard_threshold_clamped == 1 && sample.write == 0);
    }
    ctex_mesh_destroy(mesh);
    return passed;
}

static int paint_mask_classes_intersect_before_deposition(void) {
    const double layer_a_values[2] = {0.5, 1.0};
    const double layer_b_values[2] = {1.0, 0.5};
    const ctex_paint_mask_view layers[2] = {{layer_a_values, 2}, {layer_b_values, 2}};
    const double colour_values[2] = {0.8, 1.0};
    const double geometry_values[2] = {0.5, 1.0};
    const double screen_values[2] = {0.25, 1.0};
    const double island_values[2] = {1.0, 0.5};
    const ctex_paint_mask_view colour = {colour_values, 2};
    const ctex_paint_mask_view geometry = {geometry_values, 2};
    const ctex_paint_mask_view screen = {screen_values, 2};
    const ctex_paint_mask_view island = {island_values, 2};
    ctex_paint_mask_inputs_descriptor masks = {
        .size = CTEX_PAINT_MASK_INPUTS_DESCRIPTOR_CURRENT_SIZE,
        .active_layer_masks = layers,
        .active_layer_mask_count = 2,
        .colour_id_selection = &colour,
        .geometry_selection = &geometry,
        .screen_selection = &screen,
        .uv_island_selection = &island,
    };
    ctex_paint_mask_info info = {.size = CTEX_PAINT_MASK_INFO_CURRENT_SIZE};
    double combined[2] = {-1.0, -1.0};
    size_t count = 0;
    int passed = expect(ctex_paint_combine_masks(2, 1, &masks, &info, NULL, 0, &count) ==
                        CTEX_RESULT_SUCCESS) &&
                 expect(count == 2 && info.active_input_count == 6);
    if (passed) {
        passed = expect(ctex_paint_combine_masks(2, 1, &masks, &info, combined, 1, &count) ==
                        CTEX_RESULT_BUFFER_TOO_SMALL) &&
                 expect(combined[0] == -1.0 && combined[1] == -1.0);
    }
    if (passed) {
        passed = expect(ctex_paint_combine_masks(2, 1, &masks, &info, combined, 2, &count) ==
                        CTEX_RESULT_SUCCESS) &&
                 expect(near(combined[0], 0.05) && near(combined[1], 0.25));
    }
    if (passed) {
        passed = expect(ctex_paint_combine_masks(2, 1, NULL, &info, combined, 2, &count) ==
                        CTEX_RESULT_SUCCESS) &&
                 expect(info.active_input_count == 0 && near(combined[0], 1.0) &&
                        near(combined[1], 1.0));
    }
    {
        const double invalid_values[2] = {1.0, NAN};
        const ctex_paint_mask_view invalid = {invalid_values, 2};
        masks.geometry_selection = &invalid;
        combined[0] = -2.0;
        if (passed) {
            passed =
                expect(ctex_paint_combine_masks(2, 1, &masks, &info, combined, 2, &count) ==
                       CTEX_RESULT_INVALID_ARGUMENT) &&
                expect(ctex_get_last_diagnostic_code() == CTEX_DIAGNOSTIC_INVALID_PAINT_MASK) &&
                expect(combined[0] == -2.0);
        }
    }
    return passed;
}

static int all_blend_modes_are_reachable(ctex_paint_blend_descriptor* descriptor,
                                         ctex_vec4f* pixels, size_t* count) {
    static const char* const blend_modes[20] = {
        "normal",       "darken",      "multiply",  "color_burn", "lighten",
        "screen",       "color_dodge", "add",       "overlay",    "soft_light",
        "linear_light", "difference",  "exclusion", "subtract",   "divide",
        "hue",          "saturation",  "color",     "value",      "pass_through"};
    size_t index = 0;
    for (index = 0; index < 20; ++index) {
        descriptor->blend_mode = blend_modes[index];
        if (!expect(ctex_paint_blend_snapshot(descriptor, pixels, 2, count) ==
                    CTEX_RESULT_SUCCESS)) {
            return 0;
        }
    }
    return 1;
}

static int snapshot_blending_uses_deposition_write_mask(void) {
    const ctex_vec4f snapshot[2] = {{0.25f, 0.5f, 0.75f, 0.2f}, {0.1f, 0.2f, 0.3f, 0.4f}};
    const ctex_vec4f paint[2] = {{0.8f, 0.4f, 0.2f, 1.0f}, {1.0f, 0.9f, 0.8f, 0.7f}};
    ctex_paint_deposition_sample deposition[2] = {
        {.strength = 0.5, .retained_strength = 0.5, .write = 1},
        {.strength = 1.0, .retained_strength = 1.0, .write = 0}};
    ctex_paint_blend_descriptor descriptor = {
        CTEX_PAINT_BLEND_DESCRIPTOR_CURRENT_SIZE, 2, 1, "multiply", snapshot, paint, deposition, 2};
    ctex_vec4f pixels[2] = {{-1.0f, -1.0f, -1.0f, -1.0f}, {-1.0f, -1.0f, -1.0f, -1.0f}};
    size_t count = 0;
    int passed =
        expect(ctex_paint_blend_snapshot(&descriptor, NULL, 0, &count) == CTEX_RESULT_SUCCESS) &&
        expect(count == 2);
    if (passed) {
        passed = expect(ctex_paint_blend_snapshot(&descriptor, pixels, 1, &count) ==
                        CTEX_RESULT_BUFFER_TOO_SMALL) &&
                 expect(pixels[0].x == -1.0f && pixels[1].x == -1.0f);
    }
    if (passed) {
        passed = expect(ctex_paint_blend_snapshot(&descriptor, pixels, 2, &count) ==
                        CTEX_RESULT_SUCCESS) &&
                 expect(near_float(pixels[0].x, 0.225f) && near_float(pixels[0].y, 0.35f) &&
                        near_float(pixels[0].z, 0.45f) && near_float(pixels[0].w, 0.6f)) &&
                 expect(near_float(pixels[1].x, snapshot[1].x) &&
                        near_float(pixels[1].y, snapshot[1].y) &&
                        near_float(pixels[1].z, snapshot[1].z) &&
                        near_float(pixels[1].w, snapshot[1].w));
    }
    deposition[0].strength = 0.75;
    if (passed) {
        passed = expect(ctex_paint_blend_snapshot(&descriptor, pixels, 2, &count) ==
                        CTEX_RESULT_SUCCESS) &&
                 expect(near_float(pixels[0].x, 0.2125f) && near_float(pixels[0].y, 0.275f) &&
                        near_float(pixels[0].z, 0.3f) && near_float(pixels[0].w, 0.8f));
    }
    if (passed) {
        passed = all_blend_modes_are_reachable(&descriptor, pixels, &count);
    }
    descriptor.blend_mode = "unknown";
    pixels[0].x = -2.0f;
    if (passed) {
        passed = expect(ctex_paint_blend_snapshot(&descriptor, pixels, 2, &count) ==
                        CTEX_RESULT_INVALID_ARGUMENT) &&
                 expect(ctex_get_last_diagnostic_code() == CTEX_DIAGNOSTIC_INVALID_PAINT_BLEND) &&
                 expect(pixels[0].x == -2.0f);
    }
    descriptor.blend_mode = "normal";
    descriptor.width = 0;
    if (passed) {
        passed = expect(ctex_paint_blend_snapshot(&descriptor, pixels, 2, &count) ==
                        CTEX_RESULT_INVALID_ARGUMENT) &&
                 expect(ctex_get_last_diagnostic_code() == CTEX_DIAGNOSTIC_INVALID_PAINT_BLEND);
    }
    return passed;
}

static int brush_applies_every_enabled_channel_atomically(void) {
    const ctex_vec4f layer_base[2] = {{0.0f, 0.0f, 0.0f, 1.0f}, {0.1f, 0.2f, 0.3f, 1.0f}};
    const ctex_vec4f layer_roughness[2] = {{0.2f, 0.2f, 0.2f, 1.0f}, {0.4f, 0.4f, 0.4f, 1.0f}};
    const ctex_vec4f material_base[2] = {{1.0f, 0.0f, 0.0f, 1.0f}, {1.0f, 0.0f, 0.0f, 1.0f}};
    const ctex_vec4f material_roughness[2] = {{0.8f, 0.8f, 0.8f, 1.0f}, {0.8f, 0.8f, 0.8f, 1.0f}};
    const ctex_vec4f material_metallic[2] = {{1.0f, 1.0f, 1.0f, 1.0f}, {1.0f, 1.0f, 1.0f, 1.0f}};
    const ctex_paint_tool_channel_descriptor layer[2] = {
        {CTEX_PAINT_TOOL_CHANNEL_DESCRIPTOR_CURRENT_SIZE, "pbr.base_color", 3, layer_base, 2},
        {CTEX_PAINT_TOOL_CHANNEL_DESCRIPTOR_CURRENT_SIZE, "pbr.roughness", 1, layer_roughness, 2}};
    const ctex_paint_tool_channel_descriptor material[3] = {
        {CTEX_PAINT_TOOL_CHANNEL_DESCRIPTOR_CURRENT_SIZE, "pbr.base_color", 3, material_base, 2},
        {CTEX_PAINT_TOOL_CHANNEL_DESCRIPTOR_CURRENT_SIZE, "pbr.roughness", 1, material_roughness,
         2},
        {CTEX_PAINT_TOOL_CHANNEL_DESCRIPTOR_CURRENT_SIZE, "pbr.metallic", 1, material_metallic, 2}};
    const ctex_paint_deposition_sample deposition[2] = {
        {.strength = 0.5, .retained_strength = 0.5, .write = 1},
        {.strength = 1.0, .retained_strength = 1.0, .write = 0}};
    const ctex_paint_brush_descriptor descriptor = {CTEX_PAINT_BRUSH_DESCRIPTOR_CURRENT_SIZE,
                                                    2,
                                                    1,
                                                    layer,
                                                    2,
                                                    material,
                                                    3,
                                                    deposition,
                                                    2,
                                                    "normal"};
    ctex_paint_brush_info info = {.size = CTEX_PAINT_BRUSH_INFO_CURRENT_SIZE};
    ctex_vec4f base_output[2] = {{-1.0f, -1.0f, -1.0f, -1.0f}, {-1.0f, -1.0f, -1.0f, -1.0f}};
    ctex_vec4f roughness_output[2] = {{-1.0f, -1.0f, -1.0f, -1.0f}, {-1.0f, -1.0f, -1.0f, -1.0f}};
    const ctex_paint_tool_channel_output outputs[2] = {
        {CTEX_PAINT_TOOL_CHANNEL_OUTPUT_CURRENT_SIZE, base_output, 2},
        {CTEX_PAINT_TOOL_CHANNEL_OUTPUT_CURRENT_SIZE, roughness_output, 2}};
    int passed =
        expect(ctex_paint_apply_brush(&descriptor, &info, NULL, 0) == CTEX_RESULT_SUCCESS) &&
        expect(info.applied_channel_count == 2 && info.required_pixels_per_channel == 2);
    if (passed) {
        passed = expect(ctex_paint_apply_brush(&descriptor, &info, outputs, 1) ==
                        CTEX_RESULT_BUFFER_TOO_SMALL) &&
                 expect(base_output[0].x == -1.0f && roughness_output[0].x == -1.0f);
    }
    if (passed) {
        passed =
            expect(ctex_paint_apply_brush(&descriptor, &info, outputs, 2) == CTEX_RESULT_SUCCESS) &&
            expect(near_float(base_output[0].x, 0.5f) && near_float(roughness_output[0].x, 0.5f) &&
                   near_float(base_output[1].x, layer_base[1].x) &&
                   near_float(roughness_output[1].x, layer_roughness[1].x));
    }
    return passed;
}

static int eraser_reduces_the_selected_target_atomically(void) {
    const double snapshot[2] = {0.8, 0.4};
    const ctex_paint_deposition_sample deposition[2] = {
        {.strength = 0.5, .retained_strength = 0.5, .write = 1},
        {.strength = 0.25, .retained_strength = 0.25, .write = 1}};
    ctex_paint_eraser_descriptor descriptor = {CTEX_PAINT_ERASER_DESCRIPTOR_CURRENT_SIZE,
                                               2,
                                               1,
                                               CTEX_PAINT_ERASER_TARGET_MASK,
                                               snapshot,
                                               2,
                                               deposition,
                                               2};
    ctex_paint_eraser_info info = {.size = CTEX_PAINT_ERASER_INFO_CURRENT_SIZE};
    double values[2] = {-1.0, -1.0};
    int passed =
        expect(ctex_paint_apply_eraser(&descriptor, &info, NULL, 0) == CTEX_RESULT_SUCCESS) &&
        expect(info.target == CTEX_PAINT_ERASER_TARGET_MASK && info.required_value_count == 2);
    if (passed) {
        passed = expect(ctex_paint_apply_eraser(&descriptor, &info, values, 1) ==
                        CTEX_RESULT_BUFFER_TOO_SMALL) &&
                 expect(values[0] == -1.0 && values[1] == -1.0);
    }
    if (passed) {
        passed =
            expect(ctex_paint_apply_eraser(&descriptor, &info, values, 2) == CTEX_RESULT_SUCCESS) &&
            expect(near(values[0], 0.4) && near(values[1], 0.3));
    }
    descriptor.target = 99;
    values[0] = -2.0;
    if (passed) {
        passed = expect(ctex_paint_apply_eraser(&descriptor, &info, values, 2) ==
                        CTEX_RESULT_INVALID_ARGUMENT) &&
                 expect(ctex_get_last_diagnostic_code() == CTEX_DIAGNOSTIC_INVALID_PAINT_TOOL) &&
                 expect(values[0] == -2.0);
    }
    return passed;
}

static int fill_exposes_all_scopes_and_shades_atomically(void) {
    const ctex_paint_surface_texel texels[8] = {
        {{0, 0, 0}, {0, 0, 1}, {0, 0, 1}, {0.1, 0.1}, 0},
        {{0, 0, 0}, {0, 0, 1}, {0, 0, 1}, {0.2, 0.1}, 0},
        {{0, 0, 0}, {0, 0, 1}, {0, 0.5, 0.8660254037844386}, {1.1, 0.1}, 1},
        {{0, 0, 0}, {0, 0, 1}, {0, 0.5, 0.8660254037844386}, {1.2, 0.1}, 1},
        {{0, 0, 0}, {0, 0, 1}, {0, 1, 0}, {0.3, 0.2}, 2},
        {{0, 0, 0}, {0, 0, 1}, {0, 1, 0}, {0.4, 0.2}, 2},
        {{0, 0, 0}, {0, 0, 1}, {1, 0, 0}, {1.3, 0.2}, 3},
        {{0, 0, 0}, {0, 0, 1}, {1, 0, 0}, {1.4, 0.2}, 3}};
    const uint8_t coverage[8] = {1, 1, 1, 1, 1, 1, 1, 1};
    const uint32_t triangles[8] = {0, 0, 1, 1, 2, 2, 3, 3};
    const uint32_t islands[8] = {10, 10, 10, 10, 20, 20, 30, 30};
    const uint32_t adjacency0[1] = {1};
    const uint32_t adjacency1[2] = {0, 2};
    const uint32_t adjacency2[2] = {1, 3};
    const uint32_t adjacency3[1] = {2};
    const ctex_paint_fill_triangle_topology topology[4] = {
        {0, {0, 0, 1}, adjacency0, 1},
        {1, {0, 0.5, 0.8660254037844386}, adjacency1, 2},
        {2, {0, 1, 0}, adjacency2, 2},
        {3, {1, 0, 0}, adjacency3, 1}};
    const double selection[8] = {0, 1, 0, 0.5, 0, 1, 0, 0};
    const double screen_selection[8] = {1, 0, 1, 1, 1, 1, 1, 1};
    const double rejection[8] = {1, 1, 0.5, 1, 1, 1, 1, 1};
    const ctex_paint_mask_view screen_mask = {screen_selection, 8};
    const ctex_paint_mask_inputs_descriptor masks = {
        .size = CTEX_PAINT_MASK_INPUTS_DESCRIPTOR_CURRENT_SIZE, .screen_selection = &screen_mask};
    const ctex_vec4f empty[8] = {{0, 0, 0, 1}, {0, 0, 0, 1}, {0, 0, 0, 1}, {0, 0, 0, 1},
                                 {0, 0, 0, 1}, {0, 0, 0, 1}, {0, 0, 0, 1}, {0, 0, 0, 1}};
    const ctex_vec4f paint[8] = {{1, 0, 0, 1}, {1, 0, 0, 1}, {1, 0, 0, 1}, {1, 0, 0, 1},
                                 {1, 0, 0, 1}, {1, 0, 0, 1}, {1, 0, 0, 1}, {1, 0, 0, 1}};
    const ctex_paint_tool_channel_descriptor layer = {
        CTEX_PAINT_TOOL_CHANNEL_DESCRIPTOR_CURRENT_SIZE, "pbr.base_color", 3, empty, 8};
    const ctex_paint_tool_channel_descriptor material = {
        CTEX_PAINT_TOOL_CHANNEL_DESCRIPTOR_CURRENT_SIZE, "pbr.base_color", 3, paint, 8};
    const double expected[6][8] = {{1, 1, 1, 1, 1, 1, 1, 1}, {1, 1, 0, 0, 0, 0, 0, 0},
                                   {1, 1, 1, 1, 0, 0, 0, 0}, {1, 1, 1, 1, 0, 0, 0, 0},
                                   {1, 1, 0, 0, 1, 1, 0, 0}, {0, 1, 0, 0.5, 0, 1, 0, 0}};
    ctex_paint_fill_descriptor descriptor = {.size = CTEX_PAINT_FILL_DESCRIPTOR_CURRENT_SIZE,
                                             .width = 4,
                                             .height = 2,
                                             .has_picked_texel = 1,
                                             .picked_texel = 0,
                                             .maximum_angle_degrees = 45,
                                             .surface_texels = texels,
                                             .surface_texel_count = 8,
                                             .coverage = coverage,
                                             .coverage_count = 8,
                                             .triangle_identity = triangles,
                                             .triangle_identity_count = 8,
                                             .uv_island_identity = islands,
                                             .uv_island_identity_count = 8,
                                             .triangle_topology = topology,
                                             .triangle_topology_count = 4,
                                             .selection = selection,
                                             .selection_count = 8,
                                             .enabled_layer_snapshot = &layer,
                                             .enabled_layer_channel_count = 1,
                                             .material = &material,
                                             .material_channel_count = 1,
                                             .blend_mode = "normal"};
    ctex_paint_fill_info info = {.size = CTEX_PAINT_FILL_INFO_CURRENT_SIZE};
    double scope_values[8] = {0};
    uint32_t selected_triangles[4] = {99, 99, 99, 99};
    ctex_vec4f shaded[8] = {{0}};
    const ctex_paint_tool_channel_output channel_output = {
        CTEX_PAINT_TOOL_CHANNEL_OUTPUT_CURRENT_SIZE, shaded, 8};
    ctex_paint_fill_outputs outputs = {CTEX_PAINT_FILL_OUTPUTS_CURRENT_SIZE,
                                       scope_values,
                                       8,
                                       selected_triangles,
                                       4,
                                       &channel_output,
                                       1};
    int passed = 1;
    uint32_t scope = 0;
    for (scope = 0; passed && scope < 6; ++scope) {
        size_t index = 0;
        descriptor.scope = scope;
        passed = expect(ctex_paint_apply_fill(&descriptor, &info, &outputs) == CTEX_RESULT_SUCCESS);
        for (index = 0; passed && index < 8; ++index) {
            passed = expect(near(scope_values[index], expected[scope][index])) &&
                     expect(near_float(shaded[index].x, (float)expected[scope][index]));
        }
    }
    if (passed) {
        outputs.scope_value_capacity = 7;
        scope_values[0] = -1;
        shaded[0].x = -1;
        passed = expect(ctex_paint_apply_fill(&descriptor, &info, &outputs) ==
                        CTEX_RESULT_BUFFER_TOO_SMALL) &&
                 expect(scope_values[0] == -1 && shaded[0].x == -1);
    }
    if (passed) {
        outputs.scope_value_capacity = 8;
        descriptor.scope = CTEX_PAINT_FILL_WHOLE_SET;
        descriptor.masks = &masks;
        descriptor.rejection_acceptance = rejection;
        descriptor.rejection_acceptance_count = 8;
        passed =
            expect(ctex_paint_apply_fill(&descriptor, &info, &outputs) == CTEX_RESULT_SUCCESS) &&
            expect(near_float(shaded[0].x, 1.0f)) && expect(near_float(shaded[1].x, 0.0f)) &&
            expect(near_float(shaded[2].x, 0.5f));
    }
    if (passed) {
        descriptor.masks = NULL;
        descriptor.rejection_acceptance = NULL;
        descriptor.rejection_acceptance_count = 0;
        descriptor.scope = CTEX_PAINT_FILL_CONNECTED_BY_ANGLE;
        descriptor.maximum_angle_degrees = 200;
        passed = expect(ctex_paint_apply_fill(&descriptor, &info, NULL) == CTEX_RESULT_SUCCESS) &&
                 expect(info.maximum_angle_clamped == 1) &&
                 expect(near(info.resolved_maximum_angle_degrees, 180)) &&
                 expect(info.selected_triangle_count == 4);
    }
    return passed;
}

static int clone_maps_aligned_and_fixed_sources_and_refuses_cross_set(void) {
    const ctex_paint_surface_texel texels[4] = {{{0, 0, 0}, {0, 0, 1}, {0, 0, 1}, {0.125, 0.5}, 0},
                                                {{0, 0, 0}, {0, 0, 1}, {0, 0, 1}, {0.375, 0.5}, 1},
                                                {{0, 0, 0}, {0, 0, 1}, {0, 0, 1}, {0.625, 0.5}, 2},
                                                {{0, 0, 0}, {0, 0, 1}, {0, 0, 1}, {0.875, 0.5}, 3}};
    const uint8_t coverage[4] = {1, 1, 1, 1};
    const ctex_vec4f layer_pixels[4] = {{0, 0, 0, 1}, {0, 0, 0, 1}, {0, 0, 0, 1}, {0, 0, 0, 1}};
    const ctex_vec4f source_pixels[4] = {
        {0.1f, 0.1f, 0.1f, 1}, {0.2f, 0.2f, 0.2f, 1}, {0.3f, 0.3f, 0.3f, 1}, {0.4f, 0.4f, 0.4f, 1}};
    const ctex_paint_tool_channel_descriptor layer = {
        CTEX_PAINT_TOOL_CHANNEL_DESCRIPTOR_CURRENT_SIZE, "pbr.base_color", 3, layer_pixels, 4};
    const ctex_paint_tool_channel_descriptor source_snapshot = {
        CTEX_PAINT_TOOL_CHANNEL_DESCRIPTOR_CURRENT_SIZE, "pbr.base_color", 3, source_pixels, 4};
    const ctex_paint_deposition_sample deposition[4] = {{.strength = 1, .write = 1},
                                                        {.strength = 1, .write = 1},
                                                        {.strength = 1, .write = 1},
                                                        {.strength = 1, .write = 1}};
    ctex_paint_clone_source_descriptor source = {
        CTEX_PAINT_CLONE_SOURCE_DESCRIPTOR_CURRENT_SIZE, "set:body", {0.125, 0.5}};
    ctex_paint_clone_descriptor descriptor = {.size = CTEX_PAINT_CLONE_DESCRIPTOR_CURRENT_SIZE,
                                              .width = 4,
                                              .height = 1,
                                              .mode = CTEX_PAINT_CLONE_ALIGNED,
                                              .destination_texture_set_id = "set:body",
                                              .tile_origin = {0, 0},
                                              .destination_anchor_uv = {0.375, 0.5},
                                              .source = &source,
                                              .destination_surface_texels = texels,
                                              .destination_surface_texel_count = 4,
                                              .destination_coverage = coverage,
                                              .destination_coverage_count = 4,
                                              .enabled_layer_snapshot = &layer,
                                              .enabled_layer_channel_count = 1,
                                              .source_snapshot = &source_snapshot,
                                              .source_channel_count = 1,
                                              .deposition = deposition,
                                              .deposition_count = 4,
                                              .blend_mode = "normal"};
    ctex_paint_clone_info info = {.size = CTEX_PAINT_CLONE_INFO_CURRENT_SIZE};
    size_t sample_indices[4] = {99, 99, 99, 99};
    ctex_vec4f pixels[4] = {{-1, -1, -1, -1}, {-1, -1, -1, -1}, {-1, -1, -1, -1}, {-1, -1, -1, -1}};
    const ctex_paint_tool_channel_output channel_output = {
        CTEX_PAINT_TOOL_CHANNEL_OUTPUT_CURRENT_SIZE, pixels, 4};
    ctex_paint_clone_outputs outputs = {CTEX_PAINT_CLONE_OUTPUTS_CURRENT_SIZE, sample_indices, 4,
                                        &channel_output, 1};
    int passed = expect(ctex_paint_apply_clone(&descriptor, &info, NULL) == CTEX_RESULT_SUCCESS) &&
                 expect(info.required_source_sample_count == 4 && info.applied_channel_count == 1);
    if (passed) {
        passed =
            expect(ctex_paint_apply_clone(&descriptor, &info, &outputs) == CTEX_RESULT_SUCCESS) &&
            expect(sample_indices[0] == CTEX_PAINT_NO_CLONE_SAMPLE) &&
            expect(sample_indices[1] == 0 && sample_indices[2] == 1 && sample_indices[3] == 2) &&
            expect(near_float(pixels[0].x, 0.0f) && near_float(pixels[1].x, 0.1f) &&
                   near_float(pixels[2].x, 0.2f) && near_float(pixels[3].x, 0.3f));
    }
    if (passed) {
        descriptor.mode = CTEX_PAINT_CLONE_FIXED;
        source.uv = (ctex_vec2d){0.625, 0.5};
        passed =
            expect(ctex_paint_apply_clone(&descriptor, &info, &outputs) == CTEX_RESULT_SUCCESS) &&
            expect(sample_indices[0] == 2 && sample_indices[1] == 2 && sample_indices[2] == 2 &&
                   sample_indices[3] == 2) &&
            expect(near_float(pixels[0].x, 0.3f) && near_float(pixels[3].x, 0.3f));
    }
    if (passed) {
        outputs.source_sample_capacity = 3;
        sample_indices[0] = 77;
        pixels[0].x = -2;
        passed = expect(ctex_paint_apply_clone(&descriptor, &info, &outputs) ==
                        CTEX_RESULT_BUFFER_TOO_SMALL) &&
                 expect(sample_indices[0] == 77 && pixels[0].x == -2);
    }
    if (passed) {
        outputs.source_sample_capacity = 4;
        source.texture_set_id = "set:source";
        passed = expect(ctex_paint_apply_clone(&descriptor, &info, &outputs) ==
                        CTEX_RESULT_INVALID_ARGUMENT) &&
                 expect(ctex_get_last_diagnostic_code() == CTEX_DIAGNOSTIC_INVALID_PAINT_TOOL) &&
                 expect(strstr(ctex_get_last_diagnostic(), "set:source") != NULL) &&
                 expect(strstr(ctex_get_last_diagnostic(), "set:body") != NULL);
    }
    return passed;
}

static int blur_and_smear_filter_the_immutable_snapshot(void) {
    const ctex_stroke_frame frame = {
        {1, 0, 0},
        {0, 1, 0},
        {0, 0, 1},
    };
    const ctex_paint_surface_filter_sample horizontal0[2] = {{0, frame, 0, 0, 1},
                                                             {1, frame, 1, 0, 1}};
    const ctex_paint_surface_filter_sample horizontal1[3] = {
        {0, frame, -1, 0, 1}, {1, frame, 0, 0, 1}, {2, frame, 1, 0, 1}};
    const ctex_paint_surface_filter_sample horizontal2[2] = {{1, frame, -1, 0, 1},
                                                             {2, frame, 0, 0, 1}};
    const ctex_paint_surface_filter_sample vertical0 = {0, frame, 0, 0, 1};
    const ctex_paint_surface_filter_sample vertical1 = {1, frame, 0, 0, 1};
    const ctex_paint_surface_filter_sample vertical2 = {2, frame, 0, 0, 1};
    const ctex_paint_blur_neighborhood_descriptor neighborhoods[3] = {
        {CTEX_PAINT_BLUR_NEIGHBORHOOD_DESCRIPTOR_CURRENT_SIZE, frame, horizontal0, 2, &vertical0,
         1},
        {CTEX_PAINT_BLUR_NEIGHBORHOOD_DESCRIPTOR_CURRENT_SIZE, frame, horizontal1, 3, &vertical1,
         1},
        {CTEX_PAINT_BLUR_NEIGHBORHOOD_DESCRIPTOR_CURRENT_SIZE, frame, horizontal2, 2, &vertical2,
         1}};
    const ctex_vec4f snapshot_pixels[3] = {{0, 0, 0, 1}, {1, 1, 1, 1}, {0, 0, 0, 1}};
    const ctex_paint_tool_channel_descriptor snapshot = {
        CTEX_PAINT_TOOL_CHANNEL_DESCRIPTOR_CURRENT_SIZE, "pbr.roughness", 1, snapshot_pixels, 3};
    const ctex_paint_deposition_sample deposition[3] = {
        {.strength = 1, .write = 1}, {.strength = 1, .write = 1}, {.strength = 1, .write = 1}};
    ctex_paint_blur_descriptor blur = {CTEX_PAINT_BLUR_DESCRIPTOR_CURRENT_SIZE,
                                       3,
                                       1,
                                       1,
                                       &snapshot,
                                       1,
                                       deposition,
                                       3,
                                       "normal",
                                       neighborhoods,
                                       3};
    ctex_paint_blur_info blur_info = {.size = CTEX_PAINT_BLUR_INFO_CURRENT_SIZE};
    ctex_vec4f output_pixels[3] = {{-1, -1, -1, -1}, {-1, -1, -1, -1}, {-1, -1, -1, -1}};
    const ctex_paint_tool_channel_output output = {CTEX_PAINT_TOOL_CHANNEL_OUTPUT_CURRENT_SIZE,
                                                   output_pixels, 3};
    int passed =
        expect(ctex_paint_apply_blur(&blur, &blur_info, &output, 1) == CTEX_RESULT_SUCCESS) &&
        expect(near_float(output_pixels[0].x, 0.5f) &&
               near_float(output_pixels[1].x, 1.0f / 3.0f) && near_float(output_pixels[2].x, 0.5f));
    if (passed) {
        blur.radius = 0;
        passed = expect(ctex_paint_apply_blur(&blur, &blur_info, NULL, 0) == CTEX_RESULT_SUCCESS) &&
                 expect(blur_info.resolved_radius == 1 && blur_info.radius_clamped == 1);
    }

    const ctex_vec4f smear_pixels[3] = {{0, 0, 0, 1}, {0.4f, 0.4f, 0.4f, 1}, {0.8f, 0.8f, 0.8f, 1}};
    const ctex_paint_tool_channel_descriptor smear_snapshot = {
        CTEX_PAINT_TOOL_CHANNEL_DESCRIPTOR_CURRENT_SIZE, "pbr.base_color", 3, smear_pixels, 3};
    const ctex_paint_smear_mapping_descriptor mappings[3] = {
        {CTEX_PAINT_SMEAR_MAPPING_DESCRIPTOR_CURRENT_SIZE, frame, {0, frame, 0, 0, 1}},
        {CTEX_PAINT_SMEAR_MAPPING_DESCRIPTOR_CURRENT_SIZE, frame, {0, frame, -1, 0, 1}},
        {CTEX_PAINT_SMEAR_MAPPING_DESCRIPTOR_CURRENT_SIZE, frame, {1, frame, -1, 0, 1}}};
    ctex_paint_smear_descriptor smear = {CTEX_PAINT_SMEAR_DESCRIPTOR_CURRENT_SIZE,
                                         3,
                                         1,
                                         0.5,
                                         1,
                                         0,
                                         &smear_snapshot,
                                         1,
                                         deposition,
                                         3,
                                         "normal",
                                         mappings,
                                         3};
    ctex_paint_smear_info smear_info = {.size = CTEX_PAINT_SMEAR_INFO_CURRENT_SIZE};
    if (passed) {
        passed =
            expect(ctex_paint_apply_smear(&smear, &smear_info, &output, 1) ==
                   CTEX_RESULT_SUCCESS) &&
            expect(near_float(output_pixels[0].x, 0.0f) && near_float(output_pixels[1].x, 0.2f) &&
                   near_float(output_pixels[2].x, 0.6f));
    }
    if (passed) {
        smear.strength = 2;
        passed =
            expect(ctex_paint_apply_smear(&smear, &smear_info, NULL, 0) == CTEX_RESULT_SUCCESS) &&
            expect(near(smear_info.resolved_strength, 1) && smear_info.strength_clamped == 1);
    }
    return passed;
}

static int stencil_resolves_a_screen_anchored_invertible_mask(void) {
    const ctex_vec2d positions[3] = {{-0.25, 0}, {0.25, 0}, {0.75, 0}};
    const double opacity[2] = {0, 1};
    ctex_paint_stencil_descriptor descriptor = {CTEX_PAINT_STENCIL_DESCRIPTOR_CURRENT_SIZE,
                                                3,
                                                1,
                                                positions,
                                                3,
                                                2,
                                                1,
                                                opacity,
                                                2,
                                                {0, 0},
                                                0,
                                                {1, 1},
                                                0};
    ctex_paint_stencil_info info = {.size = CTEX_PAINT_STENCIL_INFO_CURRENT_SIZE};
    double values[3] = {-1, -1, -1};
    int passed = expect(ctex_paint_resolve_stencil_mask(&descriptor, &info, NULL, 0) ==
                        CTEX_RESULT_SUCCESS) &&
                 expect(info.required_mask_value_count == 3);
    if (passed) {
        passed = expect(ctex_paint_resolve_stencil_mask(&descriptor, &info, values, 3) ==
                        CTEX_RESULT_SUCCESS) &&
                 expect(near(values[0], 0) && near(values[1], 1) && near(values[2], 0));
    }
    if (passed) {
        descriptor.inverted = 1;
        passed = expect(ctex_paint_resolve_stencil_mask(&descriptor, &info, values, 3) ==
                        CTEX_RESULT_SUCCESS) &&
                 expect(near(values[0], 1) && near(values[1], 0) && near(values[2], 1));
    }
    if (passed) {
        descriptor.scale.x = 0;
        passed = expect(ctex_paint_resolve_stencil_mask(&descriptor, &info, NULL, 0) ==
                        CTEX_RESULT_SUCCESS) &&
                 expect(info.scale_x_clamped == 1 && info.resolved_scale.x > 0);
    }
    return passed;
}

static int decal_rasterizes_a_retained_editable_placement(void) {
    const ctex_paint_surface_texel surface[2] = {
        {{0, 0, 0}, {0, 0, 1}, {0, 0, 1}, {0.25, 0.5}, 0},
        {{0.25, -0.25, 0}, {0, 0, 1}, {0, 0, 1}, {0.75, 0.5}, 1}};
    const uint8_t coverage[2] = {1, 1};
    const ctex_vec4f layer_pixels[2] = {{0, 0, 0, 1}, {0, 0, 0, 1}};
    const ctex_vec4f material_pixels[2] = {{0.2f, 0, 0, 1}, {0.8f, 0, 0, 1}};
    const double opacity[2] = {0.5, 0.5};
    const ctex_paint_tool_channel_descriptor layer = {
        CTEX_PAINT_TOOL_CHANNEL_DESCRIPTOR_CURRENT_SIZE, "pbr.base_color", 3, layer_pixels, 2};
    const ctex_paint_tool_channel_descriptor material = {
        CTEX_PAINT_TOOL_CHANNEL_DESCRIPTOR_CURRENT_SIZE, "pbr.base_color", 3, material_pixels, 2};
    ctex_paint_decal_descriptor descriptor = {.size = CTEX_PAINT_DECAL_DESCRIPTOR_CURRENT_SIZE,
                                              .width = 2,
                                              .height = 1,
                                              .surface_texels = surface,
                                              .surface_texel_count = 2,
                                              .coverage = coverage,
                                              .coverage_count = 2,
                                              .placement = {{0, 0, 0}, {0, 0, 1}, {0, 1, {1, 1}}},
                                              .material_width = 2,
                                              .material_height = 1,
                                              .material = &material,
                                              .material_channel_count = 1,
                                              .material_opacity = opacity,
                                              .material_opacity_count = 2,
                                              .enabled_layer_snapshot = &layer,
                                              .enabled_layer_channel_count = 1,
                                              .blend_mode = "normal"};
    ctex_paint_decal_info info = {.size = CTEX_PAINT_DECAL_INFO_CURRENT_SIZE};
    size_t samples[2] = {99, 99};
    double strength[2] = {-1, -1};
    ctex_vec4f pixels[2] = {{-1, -1, -1, -1}, {-1, -1, -1, -1}};
    const ctex_paint_tool_channel_output channel = {CTEX_PAINT_TOOL_CHANNEL_OUTPUT_CURRENT_SIZE,
                                                    pixels, 2};
    const ctex_paint_decal_outputs outputs = {
        CTEX_PAINT_DECAL_OUTPUTS_CURRENT_SIZE, samples, 2, strength, 2, &channel, 1};
    int passed =
        expect(ctex_paint_rasterize_decal(&descriptor, &info, &outputs) == CTEX_RESULT_SUCCESS) &&
        expect(samples[0] == 1 && samples[1] == 1) &&
        expect(near(strength[0], 0.5) && near(strength[1], 0.5)) &&
        expect(near_float(pixels[1].x, 0.4f));
    if (passed) {
        descriptor.placement.transform.rotation_radians = 1.5707963267948966;
        passed = expect(ctex_paint_rasterize_decal(&descriptor, &info, &outputs) ==
                        CTEX_RESULT_SUCCESS) &&
                 expect(samples[0] == 1 && samples[1] == 0) &&
                 expect(near_float(pixels[1].x, 0.1f));
    }
    return passed;
}

static int projection_exposes_camera_planar_and_triplanar_mapping(void) {
    const ctex_paint_surface_texel surface = {
        {0.25, 0.75, 0.25}, {0.7071067811865476, 0.5, 0.5}, {0, 0, 1}, {0, 0}, 0};
    const uint8_t coverage = 1;
    const double visible_surface = 0.5;
    const ctex_vec4f layer_pixel = {0, 0, 0, 1};
    const ctex_vec4f material_pixels[4] = {
        {0.1f, 0, 0, 1}, {0.2f, 0, 0, 1}, {0.3f, 0, 0, 1}, {0.4f, 0, 0, 1}};
    const double opacity[4] = {1, 1, 1, 1};
    const ctex_paint_tool_channel_descriptor layer = {
        CTEX_PAINT_TOOL_CHANNEL_DESCRIPTOR_CURRENT_SIZE, "pbr.base_color", 3, &layer_pixel, 1};
    const ctex_paint_tool_channel_descriptor material = {
        CTEX_PAINT_TOOL_CHANNEL_DESCRIPTOR_CURRENT_SIZE, "pbr.base_color", 3, material_pixels, 4};
    ctex_paint_projection_descriptor descriptor = {
        .size = CTEX_PAINT_PROJECTION_DESCRIPTOR_CURRENT_SIZE,
        .width = 1,
        .height = 1,
        .surface_texels = &surface,
        .surface_texel_count = 1,
        .coverage = &coverage,
        .coverage_count = 1,
        .mode = CTEX_PAINT_PROJECTION_CAMERA,
        .camera_view_projection = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1},
        .camera_visible_surface = &visible_surface,
        .camera_visible_surface_count = 1,
        .planar_origin = {0, 0, 0},
        .planar_u_axis = {1, 0, 0},
        .planar_v_axis = {0, 1, 0},
        .planar_extent = {2, 2},
        .triplanar_scale = 1,
        .triplanar_offset = {0, 0},
        .material_width = 2,
        .material_height = 2,
        .material = &material,
        .material_channel_count = 1,
        .material_opacity = opacity,
        .material_opacity_count = 4,
        .enabled_layer_snapshot = &layer,
        .enabled_layer_channel_count = 1,
        .blend_mode = "normal"};
    ctex_paint_projection_info info = {.size = CTEX_PAINT_PROJECTION_INFO_CURRENT_SIZE};
    ctex_paint_projection_sample sample = {{99, 99, 99}, {-1, -1, -1}, 99};
    double strength = -1;
    ctex_vec4f pixel = {-1, -1, -1, -1};
    const ctex_paint_tool_channel_output channel = {CTEX_PAINT_TOOL_CHANNEL_OUTPUT_CURRENT_SIZE,
                                                    &pixel, 1};
    ctex_paint_projection_outputs outputs = {
        CTEX_PAINT_PROJECTION_OUTPUTS_CURRENT_SIZE, &sample, 0, &strength, 1, &channel, 1};
    int passed =
        expect(ctex_paint_apply_projection(&descriptor, &info, NULL) == CTEX_RESULT_SUCCESS) &&
        expect(info.required_sample_count == 1 && info.applied_channel_count == 1) &&
        expect(ctex_paint_apply_projection(&descriptor, &info, &outputs) ==
               CTEX_RESULT_BUFFER_TOO_SMALL) &&
        expect(sample.source_indices[0] == 99 && near(strength, -1) && near_float(pixel.x, -1));
    outputs.sample_capacity = 1;
    if (passed) {
        passed = expect(ctex_paint_apply_projection(&descriptor, &info, &outputs) ==
                        CTEX_RESULT_SUCCESS) &&
                 expect(sample.count == 1 && sample.source_indices[0] == 1 &&
                        near(sample.weights[0], 1)) &&
                 expect(near(strength, 0.5) && near_float(pixel.x, 0.1f));
    }
    descriptor.mode = CTEX_PAINT_PROJECTION_PLANAR;
    if (passed) {
        passed = expect(ctex_paint_apply_projection(&descriptor, &info, &outputs) ==
                        CTEX_RESULT_SUCCESS) &&
                 expect(info.resolved_mode == CTEX_PAINT_PROJECTION_PLANAR) &&
                 expect(sample.count == 1 && sample.source_indices[0] == 1) &&
                 expect(near(strength, 1) && near_float(pixel.x, 0.2f));
    }
    descriptor.mode = CTEX_PAINT_PROJECTION_TRIPLANAR;
    if (passed) {
        passed = expect(ctex_paint_apply_projection(&descriptor, &info, &outputs) ==
                        CTEX_RESULT_SUCCESS) &&
                 expect(sample.count == 3 && sample.source_indices[0] == 3 &&
                        sample.source_indices[1] == 2 && sample.source_indices[2] == 0) &&
                 expect(near(sample.weights[0], 0.5) && near(sample.weights[1], 0.25) &&
                        near(sample.weights[2], 0.25)) &&
                 expect(near(strength, 1) && near_float(pixel.x, 0.3f));
    }
    descriptor.triplanar_scale = 0;
    if (passed) {
        passed = expect(ctex_paint_apply_projection(&descriptor, &info, &outputs) ==
                        CTEX_RESULT_SUCCESS) &&
                 expect(info.triplanar_scale_clamped == 1 && info.resolved_triplanar_scale > 0);
    }
    return passed;
}

static int text_rasterizes_supplied_utf8_font_and_applies_a_decal(void) {
    const double cedilla_coverage[4] = {1, 1, 1, 1};
    const double b_coverage[2] = {1, 1};
    const ctex_paint_font_glyph_descriptor glyphs[2] = {
        {.size = CTEX_PAINT_FONT_GLYPH_DESCRIPTOR_CURRENT_SIZE,
         .codepoint = 0x00e7,
         .width = 2,
         .height = 2,
         .bearing_y = 2,
         .advance = 2,
         .coverage = cedilla_coverage,
         .coverage_count = 4},
        {.size = CTEX_PAINT_FONT_GLYPH_DESCRIPTOR_CURRENT_SIZE,
         .codepoint = 'B',
         .width = 1,
         .height = 2,
         .bearing_y = 2,
         .advance = 1,
         .coverage = b_coverage,
         .coverage_count = 2}};
    const ctex_paint_font_descriptor font = {.size = CTEX_PAINT_FONT_DESCRIPTOR_CURRENT_SIZE,
                                             .identity = "font:capi:v1",
                                             .pixels_per_em = 2,
                                             .ascent = 2,
                                             .descent = 0,
                                             .line_gap = 0,
                                             .glyphs = glyphs,
                                             .glyph_count = 2};
    const ctex_paint_surface_texel surface = {{0, 0, 0}, {0, 0, 1}, {0, 0, 1}, {0.5, 0.5}, 0};
    const uint8_t coverage = 1;
    const ctex_vec4f layer_pixel = {0, 0, 0, 1};
    const ctex_paint_tool_channel_descriptor layer = {
        CTEX_PAINT_TOOL_CHANNEL_DESCRIPTOR_CURRENT_SIZE, "pbr.base_color", 3, &layer_pixel, 1};
    const ctex_paint_text_material_value material = {
        CTEX_PAINT_TEXT_MATERIAL_VALUE_CURRENT_SIZE, "pbr.base_color", 3, {1, 0, 0, 1}};
    const char text[] = "\xc3\xa7\xc3\xa7\nB";
    ctex_paint_text_descriptor descriptor = {.size = CTEX_PAINT_TEXT_DESCRIPTOR_CURRENT_SIZE,
                                             .width = 1,
                                             .height = 1,
                                             .surface_texels = &surface,
                                             .surface_texel_count = 1,
                                             .coverage = &coverage,
                                             .coverage_count = 1,
                                             .font = &font,
                                             .utf8 = text,
                                             .utf8_size = sizeof(text) - 1,
                                             .tracking_em = 20,
                                             .alignment = CTEX_PAINT_TEXT_ALIGN_RIGHT,
                                             .text_size = 2,
                                             .placement = {{0, 0, 0}, {0, 0, 1}, {0, 1, {1, 1}}},
                                             .material = &material,
                                             .material_channel_count = 1,
                                             .enabled_layer_snapshot = &layer,
                                             .enabled_layer_channel_count = 1,
                                             .blend_mode = "normal"};
    ctex_paint_text_info info = {.size = CTEX_PAINT_TEXT_INFO_CURRENT_SIZE};
    int passed = expect(ctex_paint_apply_text(&descriptor, &info, NULL) == CTEX_RESULT_SUCCESS) &&
                 expect(info.tracking_clamped == 1 && near(info.resolved_tracking_em, 10) &&
                        info.required_codepoint_count == 4);
    descriptor.tracking_em = 0;
    uint32_t codepoints[4] = {99, 99, 99, 99};
    double raster_opacity[16] = {-1};
    size_t source_sample = 99;
    double strength = -1;
    ctex_vec4f pixel = {-1, -1, -1, -1};
    const ctex_paint_tool_channel_output channel = {CTEX_PAINT_TOOL_CHANNEL_OUTPUT_CURRENT_SIZE,
                                                    &pixel, 1};
    ctex_paint_text_outputs outputs = {.size = CTEX_PAINT_TEXT_OUTPUTS_CURRENT_SIZE,
                                       .codepoints = codepoints,
                                       .codepoint_capacity = 4,
                                       .raster_opacity = raster_opacity,
                                       .raster_opacity_capacity = 15,
                                       .source_sample_indices = &source_sample,
                                       .source_sample_capacity = 1,
                                       .strength = &strength,
                                       .strength_capacity = 1,
                                       .channels = &channel,
                                       .channel_count = 1};
    if (passed) {
        passed = expect(ctex_paint_apply_text(&descriptor, &info, &outputs) ==
                        CTEX_RESULT_BUFFER_TOO_SMALL) &&
                 expect(codepoints[0] == 99 && near(raster_opacity[0], -1) && source_sample == 99 &&
                        near(strength, -1) && near_float(pixel.x, -1));
    }
    outputs.raster_opacity_capacity = 16;
    if (passed) {
        passed =
            expect(ctex_paint_apply_text(&descriptor, &info, &outputs) == CTEX_RESULT_SUCCESS) &&
            expect(info.raster_width == 4 && info.raster_height == 4 && info.line_count == 2 &&
                   near(info.frame_scale.x, 4) && near(info.frame_scale.y, 4)) &&
            expect(codepoints[0] == 0x00e7 && codepoints[1] == 0x00e7 && codepoints[2] == '\n' &&
                   codepoints[3] == 'B') &&
            expect(near(raster_opacity[8], 0) && near(raster_opacity[11], 1)) &&
            expect(source_sample != CTEX_PAINT_NO_DECAL_SAMPLE && near(strength, 1) &&
                   near_float(pixel.x, 1));
    }
    const char invalid_utf8[] = "\xc0\x80";
    descriptor.utf8 = invalid_utf8;
    descriptor.utf8_size = sizeof(invalid_utf8) - 1;
    if (passed) {
        const uint32_t preserved_codepoint = codepoints[0];
        const float preserved_pixel = pixel.x;
        passed =
            expect(ctex_paint_apply_text(&descriptor, &info, &outputs) ==
                   CTEX_RESULT_INVALID_ARGUMENT) &&
            expect(codepoints[0] == preserved_codepoint && near_float(pixel.x, preserved_pixel));
    }
    return passed;
}

static int particles_replay_deterministically_and_deposit_mapped_contacts(void) {
    ctex_mesh* mesh = coverage_mesh();
    ctex_pick_index* index = NULL;
    ctex_mesh_info mesh_info = {.size = CTEX_MESH_INFO_CURRENT_SIZE};
    int passed =
        mesh != NULL && expect(ctex_mesh_get_info(mesh, &mesh_info) == CTEX_RESULT_SUCCESS) &&
        expect(ctex_pick_index_create(mesh, &index) == CTEX_RESULT_SUCCESS) && index != NULL;
    const ctex_paint_surface_texel surface[4] = {{{0, 0, 0}, {0, 0, 1}, {0, 0, 1}, {0, 0}, 1},
                                                 {{0, 0, 0}, {0, 0, 1}, {0, 0, 1}, {0, 0}, 1},
                                                 {{0, 0, 0}, {0, 0, 1}, {0, 0, 1}, {0, 0}, 1},
                                                 {{0, 0, 0}, {0, 0, 1}, {0, 0, 1}, {0, 0}, 1}};
    const uint8_t coverage[4] = {1, 1, 1, 1};
    const uint32_t triangles[4] = {1, 1, 1, 1};
    const ctex_pick_texture_set_binding_descriptor binding = {
        CTEX_PICK_TEXTURE_SET_BINDING_DESCRIPTOR_CURRENT_SIZE, 0, "uv0"};
    const ctex_vec4f layer_pixels[4] = {{0, 0, 0, 1}, {0, 0, 0, 1}, {0, 0, 0, 1}, {0, 0, 0, 1}};
    const ctex_vec4f material_pixels[4] = {{1, 0, 0, 1}, {1, 0, 0, 1}, {1, 0, 0, 1}, {1, 0, 0, 1}};
    const ctex_paint_tool_channel_descriptor layer = {
        CTEX_PAINT_TOOL_CHANNEL_DESCRIPTOR_CURRENT_SIZE, "pbr.base_color", 3, layer_pixels, 4};
    const ctex_paint_tool_channel_descriptor material = {
        CTEX_PAINT_TOOL_CHANNEL_DESCRIPTOR_CURRENT_SIZE, "pbr.base_color", 3, material_pixels, 4};
    ctex_paint_particle_descriptor descriptor = {
        .size = CTEX_PAINT_PARTICLE_DESCRIPTOR_CURRENT_SIZE,
        .width = 2,
        .height = 2,
        .tile_origin = {0, 0},
        .texture_set_id = "material/10:material:0/uv/3:uv0",
        .mesh_revision = mesh_info.revision,
        .surface_texels = surface,
        .surface_texel_count = 4,
        .coverage = coverage,
        .coverage_count = 4,
        .triangle_identity = triangles,
        .triangle_identity_count = 4,
        .texture_sets = &binding,
        .texture_set_count = 1,
        .emitter_position = {1, 0.5, 1},
        .emitter_direction = {0, 0, -1},
        .simulation = {.count = 2,
                       .lifetime_seconds = 1,
                       .initial_speed = 2,
                       .mass = 1,
                       .gravity = {0, 0, 0},
                       .friction = 0,
                       .restitution = 0,
                       .randomness = 0.1,
                       .seed = 42},
        .material = &material,
        .material_channel_count = 1,
        .enabled_layer_snapshot = &layer,
        .enabled_layer_channel_count = 1,
        .blend_mode = "normal"};
    ctex_paint_particle_info info = {.size = CTEX_PAINT_PARTICLE_INFO_CURRENT_SIZE};
    if (passed) {
        passed = expect(ctex_paint_apply_particles(index, &descriptor, &info, NULL) ==
                        CTEX_RESULT_SUCCESS) &&
                 expect(info.emitted_count == 2 && info.required_final_state_count == 2 &&
                        info.required_contact_count > 0 && info.mapped_contact_count > 0);
    }
    ctex_paint_particle_contact contacts[32] = {{0}};
    ctex_paint_particle_state states[2] = {0};
    char texture_set_ids[1024] = {0};
    double strength[4] = {-1, -1, -1, -1};
    ctex_vec4f pixels[4] = {{-1, -1, -1, -1}, {-1, -1, -1, -1}, {-1, -1, -1, -1}, {-1, -1, -1, -1}};
    const ctex_paint_tool_channel_output channel = {CTEX_PAINT_TOOL_CHANNEL_OUTPUT_CURRENT_SIZE,
                                                    pixels, 4};
    ctex_paint_particle_outputs outputs = {.size = CTEX_PAINT_PARTICLE_OUTPUTS_CURRENT_SIZE,
                                           .contacts = contacts,
                                           .contact_capacity = info.required_contact_count - 1,
                                           .final_states = states,
                                           .final_state_capacity = 2,
                                           .texture_set_ids = texture_set_ids,
                                           .texture_set_id_size = sizeof(texture_set_ids),
                                           .strength = strength,
                                           .strength_capacity = 4,
                                           .channels = &channel,
                                           .channel_count = 1};
    if (passed) {
        contacts[0].particle_ordinal = 99;
        passed = expect(ctex_paint_apply_particles(index, &descriptor, &info, &outputs) ==
                        CTEX_RESULT_BUFFER_TOO_SMALL) &&
                 expect(contacts[0].particle_ordinal == 99 && near(strength[0], -1) &&
                        near_float(pixels[0].x, -1));
    }
    outputs.contact_capacity = 32;
    outputs.texture_set_ids = NULL;
    outputs.texture_set_id_size = 0;
    contacts[0].particle_ordinal = 77;
    if (passed) {
        passed = expect(ctex_paint_apply_particles(index, &descriptor, &info, &outputs) ==
                        CTEX_RESULT_SUCCESS) &&
                 expect(contacts[0].particle_ordinal == 77 && near(strength[0], -1) &&
                        near_float(pixels[0].x, -1));
    }
    outputs.texture_set_ids = texture_set_ids;
    outputs.texture_set_id_size = sizeof(texture_set_ids);
    if (passed) {
        passed = expect(ctex_paint_apply_particles(index, &descriptor, &info, &outputs) ==
                        CTEX_RESULT_SUCCESS) &&
                 expect(strcmp(texture_set_ids + contacts[0].texture_set_id_offset,
                               descriptor.texture_set_id) == 0) &&
                 expect(contacts[0].mapped_texel < 4 && strength[contacts[0].mapped_texel] > 0 &&
                        near_float(pixels[contacts[0].mapped_texel].x,
                                   (float)strength[contacts[0].mapped_texel]));
    }
    const ctex_paint_particle_contact first_contact = contacts[0];
    const ctex_paint_particle_state first_state = states[0];
    if (passed) {
        memset(contacts, 0, sizeof(contacts));
        memset(states, 0, sizeof(states));
        passed = expect(ctex_paint_apply_particles(index, &descriptor, &info, &outputs) ==
                        CTEX_RESULT_SUCCESS) &&
                 expect(contacts[0].particle_ordinal == first_contact.particle_ordinal &&
                        near(contacts[0].time_seconds, first_contact.time_seconds) &&
                        near(contacts[0].position.x, first_contact.position.x) &&
                        near(contacts[0].impulse, first_contact.impulse) &&
                        contacts[0].mapped_texel == first_contact.mapped_texel) &&
                 expect(near(states[0].position.x, first_state.position.x) &&
                        near(states[0].velocity.z, first_state.velocity.z) &&
                        states[0].collision_count == first_state.collision_count);
    }
    descriptor.simulation.count = 0;
    if (passed) {
        passed = expect(ctex_paint_apply_particles(index, &descriptor, &info, NULL) ==
                        CTEX_RESULT_SUCCESS) &&
                 expect(info.count_clamped == 1 && info.resolved_settings.count == 1);
    }
    ctex_pick_index_destroy(index);
    ctex_mesh_destroy(mesh);
    return passed;
}

static int picker_reads_channels_and_optional_material_provenance(void) {
    const ctex_vec4f base_color[4] = {
        {0.10F, 0, 0, 1}, {0.11F, 0, 0, 1}, {0.12F, 0, 0, 1}, {0.13F, 0, 0, 1}};
    const ctex_vec4f roughness[4] = {
        {0.20F, 0, 0, 1}, {0.21F, 0, 0, 1}, {0.22F, 0, 0, 1}, {0.23F, 0, 0, 1}};
    const ctex_paint_tool_channel_descriptor channels[2] = {
        {CTEX_PAINT_TOOL_CHANNEL_DESCRIPTOR_CURRENT_SIZE, "pbr.base_color", 3, base_color, 4},
        {CTEX_PAINT_TOOL_CHANNEL_DESCRIPTOR_CURRENT_SIZE, "pbr.roughness", 1, roughness, 4}};
    const char* provenance[4] = {"material:paint", "material:edge", "material:dirt",
                                 "material:metal"};
    ctex_paint_picker_texture_view_descriptor view = {
        .size = CTEX_PAINT_PICKER_TEXTURE_VIEW_DESCRIPTOR_CURRENT_SIZE,
        .texture_set_id = "set:body",
        .tile_origin = {1, 0},
        .width = 2,
        .height = 2,
        .enabled_channels = channels,
        .enabled_channel_count = 2,
        .material_identities = provenance,
        .material_identity_count = 4};
    ctex_paint_picker_descriptor descriptor = {
        .size = CTEX_PAINT_PICKER_DESCRIPTOR_CURRENT_SIZE,
        .hit = {.has_hit = 1, .uv = {1.25F, 0.75F}, .udim_u = 1, .udim_v = 0, .udim_number = 1002},
        .hit_texture_set_id = "set:body",
        .texture_views = &view,
        .texture_view_count = 1};
    ctex_paint_picker_info info = {.size = CTEX_PAINT_PICKER_INFO_CURRENT_SIZE};
    int passed = expect(ctex_paint_pick_enabled_channels(&descriptor, &info, NULL, 0, NULL, 0) ==
                        CTEX_RESULT_SUCCESS) &&
                 expect(info.required_channel_count == 2 && info.required_string_size > 0 &&
                        info.texel == 0 && info.has_material_identity == 1);
    ctex_paint_picker_channel_value values[2] = {{.component_count = 99}, {0}};
    char strings[256] = {'x', '\0'};
    if (passed) {
        passed = expect(ctex_paint_pick_enabled_channels(&descriptor, &info, values, 2, NULL, 0) ==
                        CTEX_RESULT_SUCCESS) &&
                 expect(values[0].component_count == 99);
    }
    if (passed) {
        passed = expect(ctex_paint_pick_enabled_channels(&descriptor, &info, NULL, 0, strings,
                                                         sizeof(strings)) == CTEX_RESULT_SUCCESS) &&
                 expect(strings[0] == 'x');
    }
    if (passed) {
        passed = expect(ctex_paint_pick_enabled_channels(&descriptor, &info, values, 1, strings,
                                                         sizeof(strings)) ==
                        CTEX_RESULT_BUFFER_TOO_SMALL) &&
                 expect(values[0].component_count == 99 && strings[0] == 'x');
    }
    if (passed) {
        passed = expect(ctex_paint_pick_enabled_channels(&descriptor, &info, values, 2, strings,
                                                         sizeof(strings)) == CTEX_RESULT_SUCCESS) &&
                 expect(strcmp(strings + info.texture_set_id_offset, "set:body") == 0 &&
                        strcmp(strings + info.material_identity_offset, "material:paint") == 0) &&
                 expect(values[0].component_count == 3 && near_float(values[0].value.x, 0.10F) &&
                        strcmp(strings + values[0].semantic_id_offset, "pbr.base_color") == 0) &&
                 expect(values[1].component_count == 1 && near_float(values[1].value.x, 0.20F) &&
                        strcmp(strings + values[1].semantic_id_offset, "pbr.roughness") == 0);
    }
    view.material_identities = NULL;
    view.material_identity_count = 0;
    if (passed) {
        passed = expect(ctex_paint_pick_enabled_channels(&descriptor, &info, NULL, 0, NULL, 0) ==
                        CTEX_RESULT_SUCCESS) &&
                 expect(info.has_material_identity == 0 && info.material_identity_size == 0);
    }
    view.texture_set_id = "set:other";
    if (passed) {
        passed = expect(ctex_paint_pick_enabled_channels(&descriptor, &info, NULL, 0, NULL, 0) ==
                        CTEX_RESULT_INVALID_ARGUMENT);
    }
    view.texture_set_id = "set:body";
    const ctex_paint_picker_texture_view_descriptor overlapping[2] = {view, view};
    descriptor.texture_views = overlapping;
    descriptor.texture_view_count = 2;
    if (passed) {
        passed = expect(ctex_paint_pick_enabled_channels(&descriptor, &info, NULL, 0, NULL, 0) ==
                        CTEX_RESULT_INVALID_ARGUMENT);
    }
    return passed;
}

static int colour_id_selects_exact_regions_and_reports_empty_results(void) {
    const ctex_vec4f pixels[4] = {{1.0F, 0.0F, 0.0F, 1.0F},
                                  {0.98F, 0.01F, 0.0F, 0.25F},
                                  {0.0F, 1.0F, 0.0F, 1.0F},
                                  {0.95F, 0.05F, 0.0F, 1.0F}};
    ctex_paint_colour_id_descriptor descriptor = {
        .size = CTEX_PAINT_COLOUR_ID_DESCRIPTOR_CURRENT_SIZE,
        .width = 4,
        .height = 1,
        .pixels = pixels,
        .pixel_count = 4,
        .picked_colour = {1.0F, 0.0F, 0.0F, 0.0F},
        .tolerance = 0.03};
    ctex_paint_colour_id_info info = {.size = CTEX_PAINT_COLOUR_ID_INFO_CURRENT_SIZE};
    int passed =
        expect(ctex_paint_select_colour_id(&descriptor, &info, NULL, 0) == CTEX_RESULT_SUCCESS) &&
        expect(info.status == CTEX_PAINT_COLOUR_ID_SELECTION_MATCHED &&
               info.selected_texel_count == 2 && info.required_value_count == 4 &&
               info.tolerance_clamped == 0);
    double values[4] = {-1, -1, -1, -1};
    if (passed) {
        passed = expect(ctex_paint_select_colour_id(&descriptor, &info, values, 3) ==
                        CTEX_RESULT_BUFFER_TOO_SMALL) &&
                 expect(near(values[0], -1));
    }
    if (passed) {
        passed = expect(ctex_paint_select_colour_id(&descriptor, &info, values, 4) ==
                        CTEX_RESULT_SUCCESS) &&
                 expect(near(values[0], 1) && near(values[1], 1) && near(values[2], 0) &&
                        near(values[3], 0));
    }
    descriptor.picked_colour = (ctex_vec4f){0.9F, 0.0F, 0.0F, 1.0F};
    descriptor.tolerance = 0.0;
    if (passed) {
        passed = expect(ctex_paint_select_colour_id(&descriptor, &info, NULL, 0) ==
                        CTEX_RESULT_SUCCESS) &&
                 expect(info.status == CTEX_PAINT_COLOUR_ID_SELECTION_EMPTY &&
                        info.selected_texel_count == 0 && near(info.resolved_tolerance, 0));
    }
    descriptor.picked_colour = (ctex_vec4f){0.0F, 0.0F, 0.0F, 1.0F};
    descriptor.tolerance = 2.0;
    if (passed) {
        passed = expect(ctex_paint_select_colour_id(&descriptor, &info, NULL, 0) ==
                        CTEX_RESULT_SUCCESS) &&
                 expect(info.status == CTEX_PAINT_COLOUR_ID_SELECTION_MATCHED &&
                        info.selected_texel_count == 4 && info.tolerance_clamped == 1 &&
                        info.resolved_tolerance < 2.0);
    }
    return passed;
}

static int selection_exposes_screen_polygon_and_stored_mask_regions(void) {
    ctex_mesh* mesh = selection_mesh();
    ctex_pick_index* index = NULL;
    ctex_mesh_info mesh_info = {.size = CTEX_MESH_INFO_CURRENT_SIZE};
    int passed = mesh != NULL &&
                 expect(ctex_mesh_get_info(mesh, &mesh_info) == CTEX_RESULT_SUCCESS) &&
                 expect(ctex_pick_index_create(mesh, &index) == CTEX_RESULT_SUCCESS);
    const ctex_paint_surface_texel screen_texels[4] = {{.position = {-0.6, 0, 0}, .triangle = 0},
                                                       {.position = {0, 0, 0}, .triangle = 1},
                                                       {.position = {-0.2, 0, 0}, .triangle = 1},
                                                       {.position = {0.6, 0, 0}, .triangle = 2}};
    const uint8_t screen_coverage[4] = {1, 1, 1, 1};
    const uint32_t screen_triangles[4] = {0, 1, 1, 2};
    const uint32_t screen_islands[4] = {10, 10, 10, 20};
    ctex_paint_selection_surface_descriptor surface = {
        .size = CTEX_PAINT_SELECTION_SURFACE_DESCRIPTOR_CURRENT_SIZE,
        .width = 4,
        .height = 1,
        .texture_set_id = "object:selection:paint",
        .uv_set = "paint",
        .mesh_revision = mesh_info.revision,
        .surface_texels = screen_texels,
        .surface_texel_count = 4,
        .coverage = screen_coverage,
        .coverage_count = 4,
        .triangle_identity = screen_triangles,
        .triangle_identity_count = 4,
        .uv_island_identity = screen_islands,
        .uv_island_identity_count = 4};
    ctex_paint_screen_selection_descriptor screen = {
        .size = CTEX_PAINT_SCREEN_SELECTION_DESCRIPTOR_CURRENT_SIZE,
        .kind = CTEX_PAINT_SELECTION_SCREEN_RECTANGLE,
        .surface = &surface,
        .minimum = {48, 48},
        .maximum = {52, 52},
        .view = {.size = CTEX_PICK_SCREEN_VIEW_DESCRIPTOR_CURRENT_SIZE,
                 .viewport_width = 100,
                 .viewport_height = 100}};
    screen.view.view[0] = screen.view.view[5] = screen.view.view[10] = screen.view.view[15] = 1;
    screen.view.projection[0] = screen.view.projection[5] = screen.view.projection[10] =
        screen.view.projection[15] = 1;
    ctex_paint_selection_info info = {.size = CTEX_PAINT_SELECTION_INFO_CURRENT_SIZE};
    if (passed) {
        passed =
            expect(ctex_paint_select_screen(index, &screen, &info, NULL) == CTEX_RESULT_SUCCESS) &&
            expect(info.kind == CTEX_PAINT_SELECTION_SCREEN_RECTANGLE &&
                   info.selected_texel_count == 1 && info.selected_triangle_count == 1 &&
                   info.required_value_count == 4 && info.visited_nodes > 0);
    }
    double values[6] = {-1, -1, -1, -1, -1, -1};
    uint32_t selected_triangles[3] = {99, 99, 99};
    ctex_paint_selection_outputs outputs = {.size = CTEX_PAINT_SELECTION_OUTPUTS_CURRENT_SIZE,
                                            .values = values,
                                            .value_capacity = 3,
                                            .selected_triangle_ids = selected_triangles,
                                            .selected_triangle_capacity = 3};
    if (passed) {
        passed = expect(ctex_paint_select_screen(index, &screen, &info, &outputs) ==
                        CTEX_RESULT_BUFFER_TOO_SMALL) &&
                 expect(near(values[0], -1) && selected_triangles[0] == 99);
    }
    outputs.value_capacity = 6;
    if (passed) {
        passed = expect(ctex_paint_select_screen(index, &screen, &info, &outputs) ==
                        CTEX_RESULT_SUCCESS) &&
                 expect(near(values[0], 0) && near(values[1], 1) && near(values[2], 0) &&
                        near(values[3], 0) && selected_triangles[0] == 1);
    }
    const ctex_vec2f lasso[5] = {{5, 35}, {35, 35}, {25, 50}, {35, 65}, {5, 65}};
    screen.kind = CTEX_PAINT_SELECTION_SCREEN_LASSO;
    screen.lasso_points = lasso;
    screen.lasso_point_count = 5;
    if (passed) {
        passed = expect(ctex_paint_select_screen(index, &screen, &info, &outputs) ==
                        CTEX_RESULT_SUCCESS) &&
                 expect(info.kind == CTEX_PAINT_SELECTION_SCREEN_LASSO && near(values[0], 1) &&
                        near(values[1], 0) && near(values[2], 0) && near(values[3], 0));
    }
    surface.mesh_revision += 1;
    if (passed) {
        passed = expect(ctex_paint_select_screen(index, &screen, &info, NULL) ==
                        CTEX_RESULT_INVALID_ARGUMENT);
    }
    ctex_pick_index_destroy(index);
    ctex_mesh_destroy(mesh);

    const ctex_paint_surface_texel polygon_texels[6] = {{.triangle = 0}, {.triangle = 0},
                                                        {.triangle = 1}, {.triangle = 1},
                                                        {.triangle = 2}, {.triangle = 2}};
    const uint8_t polygon_coverage[6] = {1, 1, 1, 1, 1, 1};
    const uint32_t polygon_triangles[6] = {0, 0, 1, 1, 2, 2};
    const uint32_t polygon_islands[6] = {10, 10, 10, 10, 20, 20};
    surface.width = 6;
    surface.mesh_revision = 1;
    surface.surface_texels = polygon_texels;
    surface.surface_texel_count = 6;
    surface.coverage = polygon_coverage;
    surface.coverage_count = 6;
    surface.triangle_identity = polygon_triangles;
    surface.triangle_identity_count = 6;
    surface.uv_island_identity = polygon_islands;
    surface.uv_island_identity_count = 6;
    const uint32_t adjacent_zero[1] = {1};
    const uint32_t adjacent_one[2] = {0, 2};
    const uint32_t adjacent_two[1] = {1};
    const ctex_paint_fill_triangle_topology topology[3] = {
        {.triangle_identity = 0,
         .geometric_normal = {0, 0, 1},
         .adjacent_triangles = adjacent_zero,
         .adjacent_triangle_count = 1},
        {.triangle_identity = 1,
         .geometric_normal = {0, 0.5, 0.8660254037844386},
         .adjacent_triangles = adjacent_one,
         .adjacent_triangle_count = 2},
        {.triangle_identity = 2,
         .geometric_normal = {0, 1, 0},
         .adjacent_triangles = adjacent_two,
         .adjacent_triangle_count = 1}};
    ctex_paint_polygon_selection_descriptor polygon = {
        .size = CTEX_PAINT_POLYGON_SELECTION_DESCRIPTOR_CURRENT_SIZE,
        .kind = CTEX_PAINT_SELECTION_POLYGON_TRIANGLE,
        .surface = &surface,
        .picked_texel = 0,
        .maximum_angle_degrees = 45,
        .triangle_topology = topology,
        .triangle_topology_count = 3};
    if (passed) {
        passed =
            expect(ctex_paint_select_polygon(&polygon, &info, &outputs) == CTEX_RESULT_SUCCESS) &&
            expect(info.kind == CTEX_PAINT_SELECTION_POLYGON_TRIANGLE &&
                   info.selected_texel_count == 2 && near(values[0], 1) && near(values[1], 1) &&
                   near(values[2], 0) && selected_triangles[0] == 0);
    }
    polygon.kind = CTEX_PAINT_SELECTION_POLYGON_UV_ISLAND;
    if (passed) {
        passed =
            expect(ctex_paint_select_polygon(&polygon, &info, &outputs) == CTEX_RESULT_SUCCESS) &&
            expect(info.selected_texel_count == 4 && info.selected_triangle_count == 2 &&
                   near(values[3], 1) && near(values[4], 0));
    }
    polygon.kind = CTEX_PAINT_SELECTION_POLYGON_CONNECTED_BY_ANGLE;
    if (passed) {
        passed =
            expect(ctex_paint_select_polygon(&polygon, &info, &outputs) == CTEX_RESULT_SUCCESS) &&
            expect(info.selected_texel_count == 4 && info.maximum_angle_clamped == 0 &&
                   near(info.resolved_maximum_angle_degrees, 45));
    }
    polygon.maximum_angle_degrees = -1;
    if (passed) {
        passed =
            expect(ctex_paint_select_polygon(&polygon, &info, &outputs) == CTEX_RESULT_SUCCESS) &&
            expect(info.selected_texel_count == 2 && info.maximum_angle_clamped == 1 &&
                   near(info.resolved_maximum_angle_degrees, 0) && near(values[0], 1) &&
                   near(values[2], 0));
    }
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
                   material_coordinate_modes_are_exposed_per_texel() &&
                   uncovered_material_texels_have_no_projections() &&
                   depth_rejection_defaults_are_exposed() &&
                   angle_and_backface_rejection_are_exposed() &&
                   derived_symmetry_depth_policy_is_reported() &&
                   rejection_parameters_are_clamped_and_invalid_flags_are_transactional() &&
                   deposition_accumulates_and_discards_canonical_stamps() &&
                   paint_mask_classes_intersect_before_deposition() &&
                   snapshot_blending_uses_deposition_write_mask() &&
                   brush_applies_every_enabled_channel_atomically() &&
                   eraser_reduces_the_selected_target_atomically() &&
                   fill_exposes_all_scopes_and_shades_atomically() &&
                   clone_maps_aligned_and_fixed_sources_and_refuses_cross_set() &&
                   blur_and_smear_filter_the_immutable_snapshot() &&
                   stencil_resolves_a_screen_anchored_invertible_mask() &&
                   decal_rasterizes_a_retained_editable_placement() &&
                   projection_exposes_camera_planar_and_triplanar_mapping() &&
                   text_rasterizes_supplied_utf8_font_and_applies_a_decal() &&
                   particles_replay_deterministically_and_deposit_mapped_contacts() &&
                   picker_reads_channels_and_optional_material_provenance() &&
                   colour_id_selects_exact_regions_and_reports_empty_results() &&
                   selection_exposes_screen_polygon_and_stored_mask_regions() &&
                   invalid_inputs_are_stable_diagnostics()
               ? 0
               : 1;
}
