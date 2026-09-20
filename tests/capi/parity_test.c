#include <ctex/capi.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int expect(int condition, const char* message) {
    if (!condition) {
        fprintf(stderr, "%s\n", message);
    }
    return condition;
}

static int tolerance(uint32_t value_class, uint32_t filtered, double absolute, double relative) {
    ctex_parity_tolerance_info info = {.size = CTEX_PARITY_TOLERANCE_INFO_CURRENT_SIZE};
    return expect(
        ctex_executor_parity_get_tolerance(value_class, filtered, &info) == CTEX_RESULT_SUCCESS &&
            info.absolute == absolute && info.relative == relative,
        "declared parity tolerance is incorrect");
}

static int numeric_tolerances_are_public(void) {
    return tolerance(CTEX_PARITY_UNORM8, 0, 1.0 / 255.0, 0.0) &&
           tolerance(CTEX_PARITY_UNORM8, 1, 2.0 / 255.0, 0.0) &&
           tolerance(CTEX_PARITY_UNORM16, 0, 1.0 / 65535.0, 0.0) &&
           tolerance(CTEX_PARITY_UNORM16, 1, 2.0 / 65535.0, 0.0) &&
           tolerance(CTEX_PARITY_FLOATING_POINT, 0, 1.0e-6, 1.0e-5) &&
           tolerance(CTEX_PARITY_FLOATING_POINT, 1, 5.0e-6, 5.0e-5);
}

static int compare(const double* reference, const double* measured, size_t count,
                   uint32_t value_class, uint32_t filtered, ctex_parity_comparison_info* out_info,
                   char** out_message) {
    ctex_parity_comparison_info info = {.size = CTEX_PARITY_COMPARISON_INFO_CURRENT_SIZE};
    if (ctex_executor_compare_parity(reference, measured, count, value_class, filtered, &info, NULL,
                                     0) != CTEX_RESULT_SUCCESS) {
        return 0;
    }
    char* message = (char*)malloc(info.required_message_size);
    if (message == NULL ||
        ctex_executor_compare_parity(reference, measured, count, value_class, filtered, &info,
                                     message, info.required_message_size) != CTEX_RESULT_SUCCESS) {
        free(message);
        return 0;
    }
    *out_info = info;
    *out_message = message;
    return 1;
}

static int boundary_and_failure_details_are_reported(void) {
    const double step = 1.0 / 255.0;
    const double reference[] = {0.25, 0.75};
    const double at_boundary[] = {0.25 + step, 0.75 - step};
    const double outside[] = {0.25, 0.75 + step * 1.01};
    ctex_parity_comparison_info info = {0};
    char* message = NULL;
    int passed =
        expect(compare(reference, at_boundary, 2, CTEX_PARITY_UNORM8, 0, &info, &message) &&
                   info.matches == 1 && info.compared_value_count == 2 && info.has_failure == 0 &&
                   strstr(message, "within declared") != NULL,
               "values on the parity boundary did not match");
    free(message);
    message = NULL;
    passed =
        expect(compare(reference, outside, 2, CTEX_PARITY_UNORM8, 0, &info, &message) &&
                   info.matches == 0 && info.has_failure == 1 && info.failure_value_index == 1 &&
                   info.failure_absolute_deviation > info.failure_allowed_deviation &&
                   strstr(message, "value 1") != NULL,
               "parity mismatch omitted its first failure details") &&
        passed;
    free(message);
    return passed;
}

static int filtered_and_floating_bounds_are_applied(void) {
    const double unorm_reference[] = {0.5};
    const double unorm_measured[] = {0.5 + 1.5 / 255.0};
    ctex_parity_comparison_info info = {0};
    char* message = NULL;
    int passed = expect(
        compare(unorm_reference, unorm_measured, 1, CTEX_PARITY_UNORM8, 0, &info, &message) &&
            info.matches == 0,
        "unfiltered parity accepted a filtered-only deviation");
    free(message);
    message = NULL;
    passed = expect(compare(unorm_reference, unorm_measured, 1, CTEX_PARITY_UNORM8, 1, &info,
                            &message) &&
                        info.matches == 1,
                    "filtered parity did not use its wider bound") &&
             passed;
    free(message);

    const double float_reference[] = {0.0, 1000.0};
    const double float_measured[] = {0.9e-6, 1000.009};
    message = NULL;
    passed = expect(compare(float_reference, float_measured, 2, CTEX_PARITY_FLOATING_POINT, 0,
                            &info, &message) &&
                        info.matches == 1 && info.maximum_absolute_deviation > 0.008,
                    "floating parity did not combine absolute and relative terms") &&
             passed;
    free(message);
    return passed;
}

static int invalid_and_non_finite_inputs_do_not_hide_failures(void) {
    const double nan_value[] = {NAN};
    ctex_parity_comparison_info info = {.size = CTEX_PARITY_COMPARISON_INFO_CURRENT_SIZE};
    int passed = expect(
        ctex_executor_compare_parity(nan_value, nan_value, 1, CTEX_PARITY_FLOATING_POINT, 0, &info,
                                     NULL, 0) == CTEX_RESULT_SUCCESS &&
            info.matches == 0 && info.has_failure == 1 && isinf(info.maximum_absolute_deviation),
        "non-finite parity values were treated as matching");
    info.size = CTEX_PARITY_COMPARISON_INFO_CURRENT_SIZE;
    passed = expect(ctex_executor_compare_parity(NULL, NULL, 0, CTEX_PARITY_UNORM8, 0, &info, NULL,
                                                 0) == CTEX_RESULT_INVALID_ARGUMENT,
                    "empty parity input was accepted") &&
             passed;
    ctex_parity_tolerance_info tolerance_info = {
        .size = CTEX_PARITY_TOLERANCE_INFO_CURRENT_SIZE,
    };
    passed = expect(ctex_executor_parity_get_tolerance(99, 0, &tolerance_info) ==
                            CTEX_RESULT_INVALID_ARGUMENT &&
                        ctex_get_last_diagnostic_code() == CTEX_DIAGNOSTIC_INVALID_ENUM_VALUE,
                    "unknown parity value class was accepted") &&
             passed;
    return passed;
}

int main(void) {
    return numeric_tolerances_are_public() && boundary_and_failure_details_are_reported() &&
                   filtered_and_floating_bounds_are_applied() &&
                   invalid_and_non_finite_inputs_do_not_hide_failures()
               ? 0
               : 1;
}
