#include <ctex/capi.h>
#include <ctex/version.h>

extern "C" ctex_version ctex_get_version(void) {
    return {
        CTEX_VERSION_MAJOR,
        CTEX_VERSION_MINOR,
        CTEX_VERSION_PATCH,
        CTEX_VERSION_STRING,
    };
}
