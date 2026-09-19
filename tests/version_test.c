#include <ctex/capi.h>
#include <string.h>

int main(void) {
    const ctex_version version = ctex_get_version();
    const ctex_version abi_version = ctex_get_abi_version();
    if (version.major != 0 || version.minor != 1 || version.patch != 0) {
        return 1;
    }
    if (abi_version.major != version.major || abi_version.minor != version.minor ||
        abi_version.patch != version.patch) {
        return 1;
    }
    return strcmp(version.string, "0.1.0") == 0 && strcmp(abi_version.string, version.string) == 0
               ? 0
               : 1;
}
