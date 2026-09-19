#include <ctex/capi.h>
#include <stddef.h>
#include <stdint.h>

static int expect(int condition) { return condition ? 1 : 0; }

static int gradient_is_extrapolated_through_the_gutter(void) {
    const double source[6] = {-20.0, -20.0, 2.0, 3.0, 4.0, -20.0};
    const uint8_t coverage[6] = {0, 0, 1, 1, 1, 0};
    const double expected[6] = {0.0, 1.0, 2.0, 3.0, 4.0, 5.0};
    ctex_paint_seam_dilation_descriptor dilation;
    ctex_paint_seam_dilation_info info = {.size = CTEX_PAINT_SEAM_DILATION_INFO_CURRENT_SIZE};
    double output[6] = {-9.0, -9.0, -9.0, -9.0, -9.0, -9.0};
    size_t count = 0;
    int passed = expect(ctex_paint_seam_dilation_init(&dilation) == CTEX_RESULT_SUCCESS) &&
                 expect(dilation.radius == 2);
    dilation.width = 6;
    dilation.height = 1;
    dilation.component_count = 1;
    dilation.pixels = source;
    dilation.pixel_count = 6;
    dilation.coverage = coverage;
    dilation.coverage_count = 6;
    if (passed) {
        passed = expect(ctex_paint_dilate_uv_seams(&dilation, &info, NULL, 0, &count) ==
                        CTEX_RESULT_SUCCESS) &&
                 expect(count == 6 && info.required_pixel_count == 6 &&
                        info.dilated_texel_count == 3 && info.zero_gradient_texel_count == 0 &&
                        info.resolved_radius == 2 && info.radius_clamped == 0);
    }
    info.dilated_texel_count = 99;
    if (passed) {
        passed = expect(ctex_paint_dilate_uv_seams(&dilation, &info, output, 5, &count) ==
                        CTEX_RESULT_BUFFER_TOO_SMALL) &&
                 expect(count == 6 && info.dilated_texel_count == 99 && output[0] == -9.0);
    }
    if (passed) {
        passed = expect(ctex_paint_dilate_uv_seams(&dilation, &info, output, 6, &count) ==
                        CTEX_RESULT_SUCCESS);
    }
    for (size_t index = 0; passed && index < 6; ++index) {
        passed = expect(output[index] == expected[index]);
    }
    return passed;
}

static int zero_radius_and_clamped_thin_island_are_reported(void) {
    const double source[3] = {-1.0, 7.0, -1.0};
    const uint8_t coverage[3] = {0, 1, 0};
    ctex_paint_seam_dilation_descriptor dilation;
    ctex_paint_seam_dilation_info info = {.size = CTEX_PAINT_SEAM_DILATION_INFO_CURRENT_SIZE};
    double output[3] = {0.0, 0.0, 0.0};
    size_t count = 0;
    int passed = expect(ctex_paint_seam_dilation_init(&dilation) == CTEX_RESULT_SUCCESS);
    dilation.width = 3;
    dilation.height = 1;
    dilation.component_count = 1;
    dilation.pixels = source;
    dilation.pixel_count = 3;
    dilation.coverage = coverage;
    dilation.coverage_count = 3;
    dilation.radius = 0;
    if (passed) {
        passed = expect(ctex_paint_dilate_uv_seams(&dilation, &info, output, 3, &count) ==
                        CTEX_RESULT_SUCCESS) &&
                 expect(output[0] == -1.0 && output[1] == 7.0 && output[2] == -1.0 &&
                        info.dilated_texel_count == 0 && info.resolved_radius == 0);
    }
    dilation.radius = UINT32_MAX;
    if (passed) {
        passed = expect(ctex_paint_dilate_uv_seams(&dilation, &info, output, 3, &count) ==
                        CTEX_RESULT_SUCCESS) &&
                 expect(output[0] == 7.0 && output[1] == 7.0 && output[2] == 7.0 &&
                        info.dilated_texel_count == 2 && info.zero_gradient_texel_count == 2 &&
                        info.resolved_radius < UINT32_MAX && info.radius_clamped == 1);
    }
    return passed;
}

static int invalid_coverage_is_transactional(void) {
    const double source[3] = {-1.0, 7.0, -1.0};
    const uint8_t coverage[3] = {0, 2, 0};
    ctex_paint_seam_dilation_descriptor dilation;
    ctex_paint_seam_dilation_info info = {
        .size = CTEX_PAINT_SEAM_DILATION_INFO_CURRENT_SIZE,
        .dilated_texel_count = 9,
    };
    double output[3] = {9.0, 9.0, 9.0};
    size_t count = 9;
    int passed = expect(ctex_paint_seam_dilation_init(&dilation) == CTEX_RESULT_SUCCESS);
    dilation.width = 3;
    dilation.height = 1;
    dilation.component_count = 1;
    dilation.pixels = source;
    dilation.pixel_count = 3;
    dilation.coverage = coverage;
    dilation.coverage_count = 3;
    if (passed) {
        passed =
            expect(ctex_paint_dilate_uv_seams(&dilation, &info, output, 3, &count) ==
                   CTEX_RESULT_INVALID_ARGUMENT) &&
            expect(ctex_get_last_diagnostic_code() == CTEX_DIAGNOSTIC_INVALID_PAINT_DILATION) &&
            expect(count == 3 && info.dilated_texel_count == 9 && output[0] == 9.0 &&
                   output[1] == 9.0 && output[2] == 9.0);
    }
    return passed;
}

static int oversized_raster_is_rejected_before_reading_inputs(void) {
    ctex_paint_seam_dilation_descriptor dilation;
    ctex_paint_seam_dilation_info info = {
        .size = CTEX_PAINT_SEAM_DILATION_INFO_CURRENT_SIZE,
        .dilated_texel_count = 9,
    };
    double output = 9.0;
    size_t count = 9;
    int passed = expect(ctex_paint_seam_dilation_init(&dilation) == CTEX_RESULT_SUCCESS);
    dilation.width = (uint32_t)CTEX_MAX_PAINT_TILE_TEXEL_COUNT + 1;
    dilation.height = 1;
    dilation.component_count = 1;
    if (passed) {
        passed = expect(ctex_paint_dilate_uv_seams(&dilation, &info, &output, 1, &count) ==
                        CTEX_RESULT_OVER_BUDGET) &&
                 expect(ctex_get_last_diagnostic_code() == CTEX_DIAGNOSTIC_PAINT_LIMIT_EXCEEDED) &&
                 expect(count == 9 && info.dilated_texel_count == 9 && output == 9.0);
    }
    return passed;
}

int main(void) {
    return gradient_is_extrapolated_through_the_gutter() &&
                   zero_radius_and_clamped_thin_island_are_reported() &&
                   invalid_coverage_is_transactional() &&
                   oversized_raster_is_rejected_before_reading_inputs()
               ? 0
               : 1;
}
