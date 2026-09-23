#include <ctex/capi.h>
#include <ctex/version.h>

#include <stdio.h>

int main(void) {
    const ctex_version version = ctex_get_version();
    if (version.major != CTEX_VERSION_MAJOR || version.minor != CTEX_VERSION_MINOR ||
        version.patch != CTEX_VERSION_PATCH || version.string == NULL) {
        return 1;
    }
    printf("CyberTexel %s\n", version.string);
    return 0;
}
