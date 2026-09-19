#ifndef CTEX_CAPI_INTERNAL_HPP
#define CTEX_CAPI_INTERNAL_HPP

#include <ctex/capi.h>

#include <ctex/doc/document.hpp>

struct ctex_allocator_state {
    ctex_allocate_callback allocate{};
    ctex_deallocate_callback deallocate{};
    void* user_data{};
};

struct ctex_document {
    explicit ctex_document(ctex_allocator_state allocator_value) : allocator(allocator_value) {}

    ctex_allocator_state allocator;
    ctex::doc::TextureDocument value;
};

#endif
