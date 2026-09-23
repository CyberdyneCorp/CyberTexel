#include <ctex/capi.h>

#include <cstddef>
#include <stdexcept>
#include <string>

namespace {

void* throwing_allocate(std::size_t, std::size_t, void*) {
    throw std::runtime_error("allocator callback exploded");
}

void ignored_deallocate(void*, std::size_t, std::size_t, void*) {}

}  // namespace

int main() {
    const ctex_allocator_descriptor allocator{
        CTEX_ALLOCATOR_DESCRIPTOR_CURRENT_SIZE,
        throwing_allocate,
        ignored_deallocate,
        nullptr,
    };
    if (ctex_set_allocator(&allocator) != CTEX_RESULT_SUCCESS) {
        return 1;
    }

    ctex_document* document = nullptr;
    const ctex_result result = ctex_document_create(&document);
    const ctex_diagnostic_code code = ctex_get_last_diagnostic_code();
    const std::string diagnostic = ctex_get_last_diagnostic();
    const bool caught = result == CTEX_RESULT_INTERNAL_ERROR && document == nullptr &&
                        code == CTEX_DIAGNOSTIC_UNEXPECTED_EXCEPTION &&
                        diagnostic.find("ctex_document_create") != std::string::npos &&
                        diagnostic.find("allocator callback exploded") != std::string::npos;

    if (ctex_set_allocator(nullptr) != CTEX_RESULT_SUCCESS) {
        return 2;
    }
    return caught ? 0 : 3;
}
