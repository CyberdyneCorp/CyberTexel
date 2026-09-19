#include <ctex/capi.h>
#include <stddef.h>
#include <string.h>

int cyber_sibling_probe(void);

static int result_codes_are_distinct(void) {
    const ctex_result values[] = {
        CTEX_RESULT_SUCCESS,          CTEX_RESULT_INVALID_ARGUMENT,
        CTEX_RESULT_MISSING_RESOURCE, CTEX_RESULT_UNSUPPORTED_OPERATION,
        CTEX_RESULT_OUT_OF_MEMORY,    CTEX_RESULT_OVER_BUDGET,
        CTEX_RESULT_CANCELLED,        CTEX_RESULT_INTERNAL_ERROR,
    };
    const size_t count = sizeof(values) / sizeof(values[0]);
    size_t left = 0;
    for (; left < count; ++left) {
        size_t right = left + 1;
        for (; right < count; ++right) {
            if (values[left] == values[right]) {
                return 0;
            }
        }
    }
    return CTEX_RESULT_SUCCESS == 0;
}

int main(void) {
    ctex_document* document = NULL;
    if (cyber_sibling_probe() != 73 || !result_codes_are_distinct()) {
        return 1;
    }
    if (ctex_document_create(&document) != CTEX_RESULT_SUCCESS || document == NULL) {
        return 2;
    }
    if (ctex_get_last_result() != CTEX_RESULT_SUCCESS ||
        strcmp(ctex_get_last_diagnostic(), "") != 0) {
        return 3;
    }
    ctex_document_destroy(document);

    if (ctex_document_create(NULL) != CTEX_RESULT_INVALID_ARGUMENT) {
        return 4;
    }
    if (ctex_get_last_result() != CTEX_RESULT_INVALID_ARGUMENT) {
        return 5;
    }
    if (strstr(ctex_get_last_diagnostic(), "ctex_document_create") == NULL ||
        strstr(ctex_get_last_diagnostic(), "out_document=null") == NULL) {
        return 6;
    }
    ctex_document_destroy(NULL);
    return 0;
}
