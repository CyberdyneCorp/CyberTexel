#ifndef CTEX_CAPI_INTERNAL_HPP
#define CTEX_CAPI_INTERNAL_HPP

#include <ctex/capi.h>

#include <ctex/doc/document.hpp>
#include <ctex/image/cube_lut.hpp>
#include <memory_resource>

struct ctex_allocator_state {
    ctex_allocate_callback allocate{};
    ctex_deallocate_callback deallocate{};
    void* user_data{};
};

class ctex_host_memory_resource final : public std::pmr::memory_resource {
public:
    explicit ctex_host_memory_resource(ctex_allocator_state allocator) : allocator_(allocator) {}

private:
    void* do_allocate(std::size_t bytes, std::size_t alignment) override;
    void do_deallocate(void* allocation, std::size_t bytes, std::size_t alignment) override;
    [[nodiscard]] bool do_is_equal(const std::pmr::memory_resource& other) const noexcept override;

    ctex_allocator_state allocator_;
};

struct ctex_document {
    explicit ctex_document(ctex_allocator_state allocator_value);

    ctex_allocator_state allocator;
    ctex_host_memory_resource memory_resource;
    ctex::doc::TextureDocument value;
};

struct ctex_cube_lut {
    ctex_cube_lut(ctex_allocator_state allocator_value, std::string_view source);

    ctex_allocator_state allocator;
    ctex_host_memory_resource memory_resource;
    ctex::image::CubeLut value;
};

#endif
