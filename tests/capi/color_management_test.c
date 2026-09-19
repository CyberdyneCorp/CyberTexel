#include <ctex/capi.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

static int expect(int condition, const char* message) {
    if (!condition) {
        fprintf(stderr, "%s\n", message);
    }
    return condition;
}

static int near(double left, double right, double tolerance) {
    return fabs(left - right) <= tolerance;
}

static int working_space_and_transforms(void) {
    size_t name_size = 0;
    char name[32] = {0};
    ctex_rgb_color encoded = {0.1, 0.5, 1.25, CTEX_COLOR_SPACE_SRGB_REC709};
    ctex_rgb_color working = {0};
    ctex_rgb_color round_trip = {0};
    return expect(ctex_get_working_color_space() == CTEX_COLOR_SPACE_LINEAR_REC709,
                  "working colour-space enum changed") &&
           expect(ctex_color_space_get_name(CTEX_COLOR_SPACE_LINEAR_REC709, NULL, 0, &name_size) ==
                      CTEX_RESULT_SUCCESS,
                  "working colour-space name sizing failed") &&
           expect(ctex_color_space_get_name(CTEX_COLOR_SPACE_LINEAR_REC709, name, sizeof(name),
                                            &name_size) == CTEX_RESULT_SUCCESS &&
                      strcmp(name, "Linear Rec. 709") == 0,
                  "working colour-space name changed") &&
           expect(ctex_color_convert(&encoded, CTEX_COLOR_SPACE_LINEAR_REC709, &working) ==
                          CTEX_RESULT_SUCCESS &&
                      near(working.green, 0.21404114048223255, 1e-14) && working.blue > 1.0,
                  "sRGB input did not convert to linear Rec. 709") &&
           expect(ctex_color_convert(&working, CTEX_COLOR_SPACE_SRGB_REC709, &round_trip) ==
                          CTEX_RESULT_SUCCESS &&
                      near(round_trip.red, encoded.red, 1e-12) &&
                      near(round_trip.green, encoded.green, 1e-12) &&
                      near(round_trip.blue, encoded.blue, 1e-12),
                  "declared colour conversion did not round-trip");
}

static int channel_policies_and_automatic_input(void) {
    ctex_channel_color_policy roughness = {.size = CTEX_CHANNEL_COLOR_POLICY_CURRENT_SIZE};
    ctex_channel_color_policy normal = {.size = CTEX_CHANNEL_COLOR_POLICY_CURRENT_SIZE};
    ctex_bit_depth_warning normal_warning = {.size = CTEX_BIT_DEPTH_WARNING_CURRENT_SIZE};
    ctex_resolved_input_color_space photograph = {
        .size = CTEX_RESOLVED_INPUT_COLOR_SPACE_CURRENT_SIZE,
    };
    ctex_resolved_input_color_space data_map = {
        .size = CTEX_RESOLVED_INPUT_COLOR_SPACE_CURRENT_SIZE,
    };
    const ctex_rgb_color encoded_roughness = {0.5, 0.5, 0.5, CTEX_COLOR_SPACE_SRGB_REC709};
    ctex_rgb_color working_roughness = {0};
    return expect(ctex_channel_get_color_policy(CTEX_CHANNEL_SEMANTIC_ROUGHNESS, &roughness) ==
                          CTEX_RESULT_SUCCESS &&
                      roughness.color_valued == 0 && roughness.recommended_bit_depth == 8,
                  "roughness colour policy is incorrect") &&
           expect(ctex_channel_get_color_policy(CTEX_CHANNEL_SEMANTIC_NORMAL, &normal) ==
                          CTEX_RESULT_SUCCESS &&
                      normal.color_valued == 0 && normal.recommended_bit_depth == 16,
                  "normal bit-depth policy is incorrect") &&
           expect(ctex_channel_get_bit_depth_warning(CTEX_CHANNEL_SEMANTIC_NORMAL, 8,
                                                     &normal_warning) == CTEX_RESULT_SUCCESS &&
                      normal_warning.warning == 1 && normal_warning.selected_bit_depth == 8 &&
                      normal_warning.recommended_bit_depth == 16,
                  "8-bit normal warning is incorrect") &&
           expect(ctex_resolve_input_color_space(CTEX_INPUT_COLOR_SPACE_AUTOMATIC,
                                                 CTEX_CHANNEL_SEMANTIC_BASE_COLOR,
                                                 &photograph) == CTEX_RESULT_SUCCESS &&
                      photograph.inferred == 1 &&
                      photograph.color_space == CTEX_COLOR_SPACE_SRGB_REC709,
                  "automatic base-colour input was not resolved to sRGB") &&
           expect(ctex_resolve_input_color_space(CTEX_INPUT_COLOR_SPACE_AUTOMATIC,
                                                 CTEX_CHANNEL_SEMANTIC_ROUGHNESS,
                                                 &data_map) == CTEX_RESULT_SUCCESS &&
                      data_map.inferred == 1 &&
                      data_map.color_space == CTEX_COLOR_SPACE_LINEAR_REC709,
                  "automatic roughness input was not resolved to linear data") &&
           expect(ctex_color_input_to_working(&encoded_roughness, CTEX_CHANNEL_SEMANTIC_ROUGHNESS,
                                              &working_roughness) == CTEX_RESULT_SUCCESS &&
                      working_roughness.red == 0.5 && working_roughness.green == 0.5 &&
                      working_roughness.blue == 0.5,
                  "roughness received a colour transfer function");
}

static int precision_and_dithering(void) {
    double contributions[100] = {0};
    double accumulated = 0.0;
    uint8_t first[16] = {0};
    uint8_t second[16] = {0};
    uint8_t undithered = 0;
    size_t index = 0;
    for (index = 0; index < 100; ++index) {
        contributions[index] = 0.001;
    }
    if (!expect(
            ctex_accumulate_height(contributions, 100, 8, &accumulated) == CTEX_RESULT_SUCCESS &&
                accumulated > 0.09 && accumulated < 0.11,
            "height was not accumulated before quantization")) {
        return 0;
    }
    for (index = 0; index < 16; ++index) {
        const uint32_t x = (uint32_t)(index % 4);
        const uint32_t y = (uint32_t)(index / 4);
        if (ctex_quantize_unorm8(0.5, x, y, 1, &first[index]) != CTEX_RESULT_SUCCESS ||
            ctex_quantize_unorm8(0.5, x, y, 1, &second[index]) != CTEX_RESULT_SUCCESS) {
            return expect(0, "ordered dither call failed");
        }
    }
    return expect(memcmp(first, second, sizeof(first)) == 0,
                  "ordered dither was not deterministic") &&
           expect(first[0] != first[1], "ordered dither did not vary the quantized gradient") &&
           expect(ctex_quantize_unorm8(0.5, 0, 0, 0, &undithered) == CTEX_RESULT_SUCCESS &&
                      undithered == 128,
                  "host could not disable dithering");
}

static int preview_lut_is_explicit(void) {
    static const char cube_source[] =
        "LUT_3D_SIZE 2\n"
        "1 1 1\n"
        "0 1 1\n"
        "1 0 1\n"
        "0 0 1\n"
        "1 1 0\n"
        "0 1 0\n"
        "1 0 0\n"
        "0 0 0\n";
    ctex_cube_lut* lut = NULL;
    const ctex_rgb_color authored = {0.25, 0.5, 0.75, CTEX_COLOR_SPACE_LINEAR_REC709};
    ctex_rgb_color preview = {0};
    if (!expect(ctex_cube_lut_create(cube_source, sizeof(cube_source) - 1, &lut) ==
                        CTEX_RESULT_SUCCESS &&
                    lut != NULL,
                ".cube LUT creation failed")) {
        ctex_cube_lut_destroy(lut);
        return 0;
    }
    const int passed =
        expect(ctex_cube_lut_apply_preview(lut, &authored, &preview) == CTEX_RESULT_SUCCESS,
               "preview LUT application failed") &&
        expect(near(preview.red, 0.75, 1e-12) && near(preview.green, 0.5, 1e-12) &&
                   near(preview.blue, 0.25, 1e-12) &&
                   preview.color_space == CTEX_COLOR_SPACE_LINEAR_REC709,
               "preview LUT returned the wrong colour") &&
        expect(authored.red == 0.25 && authored.green == 0.5 && authored.blue == 0.75,
               "preview LUT modified authored export colour");
    ctex_cube_lut_destroy(lut);
    return passed;
}

static int unsupported_space_is_named(void) {
    ctex_rgb_color input = {0.0, 0.0, 0.0, 99};
    ctex_rgb_color output = {0};
    return expect(ctex_color_convert(&input, CTEX_COLOR_SPACE_LINEAR_REC709, &output) ==
                          CTEX_RESULT_UNSUPPORTED_OPERATION &&
                      ctex_get_last_diagnostic_code() == CTEX_DIAGNOSTIC_UNSUPPORTED_COLOR_SPACE,
                  "unsupported colour space was not refused by stable code");
}

static int malformed_lut_is_named(void) {
    static const char malformed[] = "LUT_3D_SIZE 2\n0 0 0\n";
    ctex_cube_lut* lut = (ctex_cube_lut*)(uintptr_t)1;
    return expect(ctex_cube_lut_create(malformed, sizeof(malformed) - 1, &lut) ==
                          CTEX_RESULT_INVALID_ARGUMENT &&
                      lut == NULL &&
                      ctex_get_last_diagnostic_code() == CTEX_DIAGNOSTIC_INVALID_CUBE_LUT,
                  "malformed .cube LUT was not refused by stable code");
}

int main(void) {
    return working_space_and_transforms() && channel_policies_and_automatic_input() &&
                   precision_and_dithering() && preview_lut_is_explicit() &&
                   unsupported_space_is_named() && malformed_lut_is_named()
               ? 0
               : 1;
}
