#include <ctex/capi.h>
#include <math.h>
#include <stddef.h>
#include <stdint.h>

static int expect(int condition) { return condition ? 1 : 0; }

static int near(double actual, double expected) { return fabs(actual - expected) <= 1.0e-9; }

static ctex_stroke_frame regular_frame(void) {
    const ctex_stroke_frame frame = {
        .tangent = {1.0, 0.0, 0.0},
        .bitangent = {0.0, 1.0, 0.0},
        .normal = {0.0, 0.0, 1.0},
    };
    return frame;
}

static int scalar_and_mirrored_vector_filters_cross_the_seam(void) {
    const ctex_stroke_frame regular = regular_frame();
    const ctex_stroke_frame mirrored = {
        .tangent = {1.0, 0.0, 0.0},
        .bitangent = {0.0, -1.0, 0.0},
        .normal = {0.0, 0.0, 1.0},
    };
    const double scalar_values[4] = {2.0, 100.0, 6.0, 10.0};
    const ctex_vec3d vector_values[2] = {{0.0, 0.6, 0.8}, {0.0, -0.6, 0.8}};
    ctex_paint_surface_filter_sample samples[2] = {
        {.texel_index = 0, .tangent_frame = regular, .offset_x = 0, .offset_y = 0, .weight = 1.0},
        {.texel_index = 2, .tangent_frame = regular, .offset_x = 1, .offset_y = 0, .weight = 1.0},
    };
    ctex_paint_surface_filter_descriptor filter = {
        .size = CTEX_PAINT_SURFACE_FILTER_DESCRIPTOR_CURRENT_SIZE,
        .operation = CTEX_PAINT_SURFACE_FILTER_BLUR,
        .radius_x = 1,
        .radius_y = 0,
        .output_frame = regular,
        .samples = samples,
        .sample_count = 2,
    };
    double scalar = -1.0;
    ctex_vec3d vector = {-1.0, -1.0, -1.0};
    int passed = expect(ctex_paint_filter_surface_scalar(&filter, scalar_values, 4, &scalar) ==
                        CTEX_RESULT_SUCCESS) &&
                 expect(near(scalar, 4.0));

    samples[0].offset_x = -1;
    samples[0].weight = -1.0;
    samples[1].texel_index = 3;
    filter.operation = CTEX_PAINT_SURFACE_FILTER_DERIVATIVE;
    if (passed) {
        passed = expect(ctex_paint_filter_surface_scalar(&filter, scalar_values, 4, &scalar) ==
                        CTEX_RESULT_SUCCESS) &&
                 expect(near(scalar, 8.0));
    }

    samples[0].offset_x = 0;
    samples[0].weight = 0.5;
    samples[1].texel_index = 1;
    samples[1].tangent_frame = mirrored;
    samples[1].weight = 0.5;
    filter.operation = CTEX_PAINT_SURFACE_FILTER_MIP_GENERATION;
    if (passed) {
        passed = expect(ctex_paint_filter_surface_tangent_vector(&filter, vector_values, 2,
                                                                 &vector) == CTEX_RESULT_SUCCESS) &&
                 expect(near(vector.x, 0.0) && near(vector.y, 0.6) && near(vector.z, 0.8));
    }
    return passed;
}

static int island_padding_reports_and_preserves_gutters(void) {
    const uint32_t islands[7] = {CTEX_NO_UV_ISLAND, 10, CTEX_NO_UV_ISLAND, CTEX_NO_UV_ISLAND,
                                 CTEX_NO_UV_ISLAND, 20, CTEX_NO_UV_ISLAND};
    const double source[7] = {-1.0, 10.0, -1.0, -1.0, -1.0, 20.0, -1.0};
    const ctex_paint_island_padding_descriptor padding = {
        .size = CTEX_PAINT_ISLAND_PADDING_DESCRIPTOR_CURRENT_SIZE,
        .width = 7,
        .height = 1,
        .component_count = 1,
        .radius_x = 1,
        .radius_y = 0,
        .requested_mip_levels = 2,
        .island_identity = islands,
        .island_identity_count = 7,
        .pixels = source,
        .pixel_count = 7,
    };
    ctex_paint_island_padding_info info = {
        .size = CTEX_PAINT_ISLAND_PADDING_INFO_CURRENT_SIZE,
    };
    uint32_t ownership[7] = {99, 99, 99, 99, 99, 99, 99};
    ctex_paint_unsupported_mip_level unsupported[1] = {{99, 99, 99, 99}};
    uint32_t affected[2] = {99, 99};
    double output[7] = {-9.0, -9.0, -9.0, -9.0, -9.0, -9.0, -9.0};
    size_t ownership_count = 0;
    size_t unsupported_count = 0;
    size_t affected_count = 0;
    size_t pixel_count = 0;
    int passed = expect(ctex_paint_plan_island_padding(&padding, &info, NULL, 0, &ownership_count,
                                                       NULL, 0, &unsupported_count, NULL, 0,
                                                       &affected_count) == CTEX_RESULT_SUCCESS) &&
                 expect(ownership_count == 7 && unsupported_count == 1 && affected_count == 2 &&
                        info.padding_radius == 3 && info.required_pixel_count == 7);

    info.padded_texel_count = 99;
    if (passed) {
        passed = expect(ctex_paint_plan_island_padding(
                            &padding, &info, ownership, 7, &ownership_count, unsupported, 1,
                            &unsupported_count, affected, 1,
                            &affected_count) == CTEX_RESULT_BUFFER_TOO_SMALL) &&
                 expect(info.padded_texel_count == 99 && ownership[0] == 99 &&
                        unsupported[0].mip_level == 99 && affected[0] == 99);
    }
    if (passed) {
        passed =
            expect(ctex_paint_plan_island_padding(&padding, &info, ownership, 7, &ownership_count,
                                                  unsupported, 1, &unsupported_count, affected, 2,
                                                  &affected_count) == CTEX_RESULT_SUCCESS) &&
            expect(unsupported[0].mip_level == 1 && unsupported[0].required_gutter_radius == 3 &&
                   unsupported[0].affected_island_offset == 0 &&
                   unsupported[0].affected_island_count == 2 && affected[0] == 10 &&
                   affected[1] == 20 && ownership[3] == CTEX_NO_UV_ISLAND);
    }
    if (passed) {
        passed = expect(ctex_paint_apply_island_padding(&padding, &info, NULL, 0, &pixel_count) ==
                        CTEX_RESULT_SUCCESS) &&
                 expect(pixel_count == 7 && info.padded_texel_count == 4);
    }
    info.padded_texel_count = 99;
    if (passed) {
        passed = expect(ctex_paint_apply_island_padding(&padding, &info, output, 6, &pixel_count) ==
                        CTEX_RESULT_BUFFER_TOO_SMALL) &&
                 expect(info.padded_texel_count == 99 && output[0] == -9.0);
    }
    if (passed) {
        passed = expect(ctex_paint_apply_island_padding(&padding, &info, output, 7, &pixel_count) ==
                        CTEX_RESULT_SUCCESS) &&
                 expect(output[0] == 10.0 && output[1] == 10.0 && output[2] == 10.0 &&
                        output[3] == -1.0 && output[4] == 20.0 && output[5] == 20.0 &&
                        output[6] == 20.0 && info.padded_texel_count == 4);
    }
    return passed;
}

static int invalid_filter_and_oversized_padding_are_transactional(void) {
    const double value = 1.0;
    const ctex_paint_surface_filter_sample sample = {
        .texel_index = 0,
        .tangent_frame = {{0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}},
        .weight = 1.0,
    };
    const ctex_paint_surface_filter_descriptor filter = {
        .size = CTEX_PAINT_SURFACE_FILTER_DESCRIPTOR_CURRENT_SIZE,
        .operation = CTEX_PAINT_SURFACE_FILTER_BLUR,
        .output_frame = regular_frame(),
        .samples = &sample,
        .sample_count = 1,
    };
    ctex_paint_island_padding_descriptor padding = {
        .size = CTEX_PAINT_ISLAND_PADDING_DESCRIPTOR_CURRENT_SIZE,
        .width = (uint32_t)CTEX_MAX_PAINT_TILE_TEXEL_COUNT + 1,
        .height = 1,
        .component_count = 1,
        .requested_mip_levels = 1,
    };
    ctex_paint_island_padding_info info = {
        .size = CTEX_PAINT_ISLAND_PADDING_INFO_CURRENT_SIZE,
        .padded_texel_count = 99,
    };
    double result = 99.0;
    size_t pixel_count = 99;
    int passed = expect(ctex_paint_filter_surface_scalar(&filter, &value, 1, &result) ==
                        CTEX_RESULT_INVALID_ARGUMENT) &&
                 expect(ctex_get_last_diagnostic_code() == CTEX_DIAGNOSTIC_INVALID_PAINT_FILTER) &&
                 expect(result == 99.0);
    if (passed) {
        passed = expect(ctex_paint_filter_surface_scalar(&filter, &value,
                                                         CTEX_MAX_PAINT_TILE_TEXEL_COUNT + 1,
                                                         &result) == CTEX_RESULT_OVER_BUDGET) &&
                 expect(ctex_get_last_diagnostic_code() == CTEX_DIAGNOSTIC_PAINT_LIMIT_EXCEEDED) &&
                 expect(result == 99.0);
    }
    if (passed) {
        passed = expect(ctex_paint_apply_island_padding(&padding, &info, &result, 1,
                                                        &pixel_count) == CTEX_RESULT_OVER_BUDGET) &&
                 expect(ctex_get_last_diagnostic_code() == CTEX_DIAGNOSTIC_PAINT_LIMIT_EXCEEDED) &&
                 expect(info.padded_texel_count == 99 && pixel_count == 99 && result == 99.0);
    }
    return passed;
}

int main(void) {
    return scalar_and_mirrored_vector_filters_cross_the_seam() &&
                   island_padding_reports_and_preserves_gutters() &&
                   invalid_filter_and_oversized_padding_are_transactional()
               ? 0
               : 1;
}
