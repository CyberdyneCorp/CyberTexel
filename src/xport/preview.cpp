#include <ctex/xport/preview.hpp>
#include <stdexcept>
#include <utility>

namespace ctex::xport {

PreviewResource::PreviewResource(std::string channel_semantic, std::uint32_t width,
                                 std::uint32_t height, image::PixelFormat format,
                                 std::uint32_t tile_size, std::span<const std::byte> clear_pixel)
    : channel_semantic_(std::move(channel_semantic)),
      pixels_(width, height, format, tile_size, clear_pixel) {
    if (channel_semantic_.empty()) {
        throw std::invalid_argument("preview resource requires a channel semantic");
    }
}

void PreviewResource::write_pixel(std::uint32_t x, std::uint32_t y,
                                  std::span<const std::byte> pixel) {
    pixels_.write_pixel(x, y, pixel);
}

image::RevisionCursor PreviewResource::reset_revision_history() {
    return pixels_.reset_revision_history();
}

}  // namespace ctex::xport
