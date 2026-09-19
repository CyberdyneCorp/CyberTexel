#ifndef CTEX_MAPS_PIXEL_BUFFER_HPP
#define CTEX_MAPS_PIXEL_BUFFER_HPP

#include <cstddef>
#include <cstdint>
#include <ctex/image/pixel_format.hpp>

namespace ctex::maps {

struct MeshMapPixelBufferView {
    std::uint32_t width{};
    std::uint32_t height{};
    image::PixelFormat format{};
    std::size_t row_stride_bytes{};
    const void* pixels{};
    std::size_t pixel_bytes{};
};

}  // namespace ctex::maps

#endif
