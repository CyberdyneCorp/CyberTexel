#include <ctex/capi.h>
#include <string.h>

int main(void) {
    const ctex_version version = ctex_get_version();
    if (version.major != 0 || version.minor != 1 || version.patch != 0) {
        return 1;
    }
    return strcmp(version.string, "0.1.0") == 0 ? 0 : 1;
}
