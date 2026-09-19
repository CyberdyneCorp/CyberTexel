#ifndef CTEX_IMAGE_TILED_IMAGE_HPP
#define CTEX_IMAGE_TILED_IMAGE_HPP

#include <cstddef>
#include <cstdint>
#include <ctex/image/pixel_format.hpp>
#include <span>
#include <vector>

namespace ctex::image {

inline constexpr std::uint32_t default_tile_size = 64;

struct TileCoordinate {
    std::uint32_t x;
    std::uint32_t y;

    friend constexpr bool operator==(TileCoordinate, TileCoordinate) noexcept = default;
};

struct TileExtent {
    std::uint32_t width;
    std::uint32_t height;
};

class TiledImage {
public:
    TiledImage(std::uint32_t width, std::uint32_t height, PixelFormat format,
               std::uint32_t tile_size = default_tile_size,
               std::span<const std::byte> clear_pixel = {});

    [[nodiscard]] std::uint32_t width() const noexcept { return width_; }
    [[nodiscard]] std::uint32_t height() const noexcept { return height_; }
    [[nodiscard]] std::uint32_t tile_size() const noexcept { return tile_size_; }
    [[nodiscard]] std::uint32_t tile_columns() const noexcept { return tile_columns_; }
    [[nodiscard]] std::uint32_t tile_rows() const noexcept { return tile_rows_; }
    [[nodiscard]] PixelFormat format() const noexcept { return format_; }
    [[nodiscard]] std::size_t pixel_bytes() const noexcept { return pixel_bytes_; }
    [[nodiscard]] std::size_t tile_bytes() const noexcept { return tile_bytes_; }
    [[nodiscard]] std::size_t resident_pixel_bytes() const noexcept;

    [[nodiscard]] TileExtent tile_extent(TileCoordinate tile) const;
    [[nodiscard]] bool is_tile_allocated(TileCoordinate tile) const;
    [[nodiscard]] bool is_tile_dirty(TileCoordinate tile) const;
    [[nodiscard]] std::vector<TileCoordinate> dirty_tiles() const;

    [[nodiscard]] std::span<const std::byte> read_pixel(std::uint32_t x, std::uint32_t y) const;
    void write_pixel(std::uint32_t x, std::uint32_t y, std::span<const std::byte> pixel);
    void clear_dirty() noexcept;

private:
    [[nodiscard]] std::size_t tile_index(TileCoordinate tile) const;
    [[nodiscard]] std::size_t pixel_offset(std::uint32_t x, std::uint32_t y) const noexcept;
    [[nodiscard]] std::vector<std::byte>& allocate_tile(std::size_t index);

    std::uint32_t width_;
    std::uint32_t height_;
    std::uint32_t tile_size_;
    std::uint32_t tile_columns_;
    std::uint32_t tile_rows_;
    PixelFormat format_;
    std::size_t pixel_bytes_;
    std::size_t tile_bytes_;
    std::vector<std::byte> clear_pixel_;
    std::vector<std::vector<std::byte>> tiles_;
    std::vector<bool> dirty_;
};

}  // namespace ctex::image

#endif
