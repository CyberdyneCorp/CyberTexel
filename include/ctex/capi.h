#ifndef CTEX_CAPI_H
#define CTEX_CAPI_H

#include <stdint.h>

#if defined(_WIN32) && defined(CTEX_SHARED)
#if defined(CTEX_BUILDING_LIBRARY)
#define CTEX_API __declspec(dllexport)
#else
#define CTEX_API __declspec(dllimport)
#endif
#elif defined(__GNUC__) || defined(__clang__)
#define CTEX_API __attribute__((visibility("default")))
#else
#define CTEX_API
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef enum ctex_result {
    CTEX_RESULT_SUCCESS = 0,
    CTEX_RESULT_INVALID_ARGUMENT = 1,
    CTEX_RESULT_MISSING_RESOURCE = 2,
    CTEX_RESULT_UNSUPPORTED_OPERATION = 3,
    CTEX_RESULT_OUT_OF_MEMORY = 4,
    CTEX_RESULT_OVER_BUDGET = 5,
    CTEX_RESULT_CANCELLED = 6,
    CTEX_RESULT_INTERNAL_ERROR = 7
} ctex_result;

typedef struct ctex_document ctex_document;

typedef struct ctex_version {
    uint32_t major;
    uint32_t minor;
    uint32_t patch;
    const char* string;
} ctex_version;

CTEX_API ctex_version ctex_get_version(void);

CTEX_API ctex_result ctex_document_create(ctex_document** out_document);
CTEX_API void ctex_document_destroy(ctex_document* document);

/*
 * Diagnostics are local to the calling thread. The returned pointer is owned by
 * CyberTexel and remains valid until the next fallible C API call on that thread.
 */
CTEX_API ctex_result ctex_get_last_result(void);
CTEX_API const char* ctex_get_last_diagnostic(void);

#ifdef __cplusplus
}
#endif

#endif
