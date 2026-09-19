#include "pixel_buffer_copy.hpp"

#include <limits>
#include <span>
#include <stdexcept>

namespace ctex::maps::detail {
namespace {

std::size_t checked_row_bytes(const MeshMapPixelBufferView& source) {
    const std::size_t pixel_size = source.format.bytes_per_pixel();
    if (!source.format.is_valid() || source.width == 0 || source.height == 0 || pixel_size == 0 ||
        source.width > std::numeric_limits<std::size_t>::max() / pixel_size) {
        throw std::invalid_argument("mesh-map pixel buffer has an invalid format or extent");
    }
    return static_cast<std::size_t>(source.width) * pixel_size;
}

}  // namespace

std::shared_ptr<image::TiledImage> copy_mesh_map_pixel_buffer(
    const MeshMapPixelBufferView& source) {
    const std::size_t row_bytes = checked_row_bytes(source);
    if (source.row_stride_bytes < row_bytes ||
        source.height - 1 >
            (std::numeric_limits<std::size_t>::max() - row_bytes) / source.row_stride_bytes) {
        throw std::invalid_argument("mesh-map pixel buffer has an invalid row stride");
    }
    const std::size_t required_bytes =
        static_cast<std::size_t>(source.height - 1) * source.row_stride_bytes + row_bytes;
    if (source.pixels == nullptr || source.pixel_bytes < required_bytes) {
        throw std::invalid_argument("mesh-map pixel buffer is incomplete");
    }

    auto result = std::make_shared<image::TiledImage>(source.width, source.height, source.format);
    const auto* bytes = static_cast<const std::byte*>(source.pixels);
    const std::size_t pixel_size = source.format.bytes_per_pixel();
    for (std::uint32_t y = 0; y < source.height; ++y) {
        for (std::uint32_t x = 0; x < source.width; ++x) {
            const std::size_t offset = static_cast<std::size_t>(y) * source.row_stride_bytes +
                                       static_cast<std::size_t>(x) * pixel_size;
            result->write_pixel(x, y, std::span(bytes + offset, pixel_size));
        }
    }
    return result;
}

}  // namespace ctex::maps::detail
