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

typedef struct ctex_version {
    uint32_t major;
    uint32_t minor;
    uint32_t patch;
    const char* string;
} ctex_version;

CTEX_API ctex_version ctex_get_version(void);

#ifdef __cplusplus
}
#endif

#endif
