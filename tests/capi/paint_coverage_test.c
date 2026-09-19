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
                   deposition_accumulates_and_discards_canonical_stamps() &&
                   paint_mask_classes_intersect_before_deposition() &&
                   snapshot_blending_uses_deposition_write_mask() &&
                   invalid_inputs_are_stable_diagnostics()
               ? 0
               : 1;
}
