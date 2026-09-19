#ifndef CTEX_XPORT_PREVIEW_HPP
#define CTEX_XPORT_PREVIEW_HPP

#include <cstddef>
#include <cstdint>
#include <ctex/image/tiled_image.hpp>
#include <span>
#include <string>
#include <string_view>

namespace ctex::xport {

class PreviewResource {
public:
    PreviewResource(std::string channel_semantic, std::uint32_t width, std::uint32_t height,
                    image::PixelFormat format, std::uint32_t tile_size = image::default_tile_size,
                    std::span<const std::byte> clear_pixel = {});
    PreviewResource(PreviewResource&&) noexcept = default;
    PreviewResource& operator=(PreviewResource&&) noexcept = default;
    PreviewResource(const PreviewResource&) = delete;
    PreviewResource& operator=(const PreviewResource&) = delete;

    [[nodiscard]] std::string_view channel_semantic() const noexcept { return channel_semantic_; }
    [[nodiscard]] std::uint32_t width() const noexcept { return pixels_.width(); }
    [[nodiscard]] std::uint32_t height() const noexcept { return pixels_.height(); }
    [[nodiscard]] image::PixelFormat format() const noexcept { return pixels_.format(); }
    [[nodiscard]] image::RevisionCursor revision_cursor() const noexcept {
        return pixels_.revision_cursor();
    }
    [[nodiscard]] const image::TiledImage& pixels() const noexcept { return pixels_; }

    void write_pixel(std::uint32_t x, std::uint32_t y, std::span<const std::byte> pixel);
    [[nodiscard]] image::RevisionCursor reset_revision_history();

private:
    std::string channel_semantic_;
    image::TiledImage pixels_;
};

}  // namespace ctex::xport

#endif
