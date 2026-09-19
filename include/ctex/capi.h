#ifndef CTEX_CAPI_H
#define CTEX_CAPI_H

#include <stddef.h>
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
    CTEX_RESULT_INTERNAL_ERROR = 7,
    CTEX_RESULT_BUFFER_TOO_SMALL = 8
} ctex_result;

typedef struct ctex_document ctex_document;

typedef enum ctex_partition_source_kind {
    CTEX_PARTITION_SOURCE_MATERIAL = 0,
    CTEX_PARTITION_SOURCE_OBJECT = 1,
    CTEX_PARTITION_SOURCE_SUBMESH = 2,
    CTEX_PARTITION_SOURCE_EXPLICIT_FACES = 3
} ctex_partition_source_kind;

typedef struct ctex_texture_set_descriptor {
    uint32_t size;
    const char* display_name;
    uint32_t partition_kind;
    const char* partition_key;
    const char* uv_set;
    uint32_t width;
    uint32_t height;
    uint8_t default_bit_depth;
} ctex_texture_set_descriptor;

#define CTEX_TEXTURE_SET_DESCRIPTOR_V1_SIZE \
    ((uint32_t)offsetof(ctex_texture_set_descriptor, default_bit_depth))
#define CTEX_TEXTURE_SET_DESCRIPTOR_CURRENT_SIZE ((uint32_t)sizeof(ctex_texture_set_descriptor))

typedef struct ctex_version {
    uint32_t major;
    uint32_t minor;
    uint32_t patch;
    const char* string;
} ctex_version;

CTEX_API ctex_version ctex_get_version(void);

CTEX_API ctex_result ctex_document_create(ctex_document** out_document);
CTEX_API void ctex_document_destroy(ctex_document* document);
CTEX_API ctex_result ctex_document_create_texture_set(
    ctex_document* document, const ctex_texture_set_descriptor* descriptor);
CTEX_API ctex_result ctex_document_get_texture_set_ids(const ctex_document* document, char* buffer,
                                                       size_t buffer_size,
                                                       size_t* out_required_size,
                                                       size_t* out_count);

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
