#ifndef CTEX_MAPS_PIXEL_BUFFER_COPY_HPP
#define CTEX_MAPS_PIXEL_BUFFER_COPY_HPP

#include <ctex/image/tiled_image.hpp>
#include <ctex/maps/pixel_buffer.hpp>
#include <memory>

namespace ctex::maps::detail {

[[nodiscard]] std::shared_ptr<image::TiledImage> copy_mesh_map_pixel_buffer(
    const MeshMapPixelBufferView& source);

}  // namespace ctex::maps::detail

#endif
