#include <algorithm>
#include <array>
#include <ctex/image/tiled_image.hpp>
#include <limits>
#include <stdexcept>

namespace ctex::image {
namespace {

std::size_t checked_multiply(std::size_t left, std::size_t right, const char* description) {
    if (right != 0 && left > std::numeric_limits<std::size_t>::max() / right) {
        throw std::overflow_error(description);
    }
    return left * right;
}

std::uint64_t row_major_key(TileCoordinate coordinate) noexcept {
    return (static_cast<std::uint64_t>(coordinate.y) << 32U) | coordinate.x;
}

void radix_sort_row_major(std::vector<TileCoordinate>& coordinates) {
    if (coordinates.size() < 2) {
        return;
    }
    std::vector<TileCoordinate> scratch(coordinates.size());
    for (unsigned shift = 0; shift < 64; shift += 8) {
        std::array<std::size_t, 256> offsets{};
        for (const TileCoordinate coordinate : coordinates) {
            ++offsets[(row_major_key(coordinate) >> shift) & 0xffU];
        }
        std::size_t next = 0;
        for (std::size_t& offset : offsets) {
            const std::size_t count = offset;
            offset = next;
            next += count;
        }
        for (const TileCoordinate coordinate : coordinates) {
            scratch[offsets[(row_major_key(coordinate) >> shift) & 0xffU]++] = coordinate;
        }
        coordinates.swap(scratch);
    }
}

}  // namespace

TiledImage::TiledImage(std::uint32_t width, std::uint32_t height, PixelFormat format,
                       std::uint32_t tile_size, std::span<const std::byte> clear_pixel)
    : width_(width),
      height_(height),
      tile_size_(tile_size),
      tile_columns_(0),
      tile_rows_(0),
      format_(format),
      pixel_bytes_(format.bytes_per_pixel()),
      tile_bytes_(0) {
    if (width == 0 || height == 0) {
        throw std::invalid_argument("image dimensions must be non-zero");
    }
    if (!format.is_valid()) {
        throw std::invalid_argument("pixel format must have one to four 8, 16 or 32-bit channels");
    }
    if (tile_size == 0) {
        throw std::invalid_argument("tile size must be non-zero");
    }
    if (!clear_pixel.empty() && clear_pixel.size() != pixel_bytes_) {
        throw std::invalid_argument("clear pixel size does not match the pixel format");
    }

    tile_columns_ = 1 + ((width - 1) / tile_size);
    tile_rows_ = 1 + ((height - 1) / tile_size);
    const std::size_t tile_pixels = checked_multiply(tile_size, tile_size, "tile area overflows");
    tile_bytes_ = checked_multiply(tile_pixels, pixel_bytes_, "tile byte size overflows");
    const std::size_t tile_count =
        checked_multiply(tile_columns_, tile_rows_, "image tile count overflows");

    clear_pixel_.assign(pixel_bytes_, std::byte{0});
    if (!clear_pixel.empty()) {
        std::copy(clear_pixel.begin(), clear_pixel.end(), clear_pixel_.begin());
    }
    tiles_.resize(tile_count);
    dirty_.resize(tile_count, false);
    tile_revisions_.resize(tile_count, 0);
    tile_generations_.resize(tile_count, 0);
}

std::size_t TiledImage::resident_pixel_bytes() const noexcept {
    return static_cast<std::size_t>(std::count_if(
               tiles_.begin(), tiles_.end(), [](const auto& tile) { return tile != nullptr; })) *
           tile_bytes_;
}

TileExtent TiledImage::tile_extent(TileCoordinate tile) const {
    static_cast<void>(tile_index(tile));
    const std::uint32_t origin_x = tile.x * tile_size_;
    const std::uint32_t origin_y = tile.y * tile_size_;
    return {
        std::min(tile_size_, width_ - origin_x),
        std::min(tile_size_, height_ - origin_y),
    };
}

Revision TiledImage::tile_revision(TileCoordinate tile) const {
    return tile_revisions_[tile_index(tile)];
}

Generation TiledImage::tile_generation(TileCoordinate tile) const {
    return tile_generations_[tile_index(tile)];
}

bool TiledImage::is_tile_allocated(TileCoordinate tile) const {
    return tiles_[tile_index(tile)] != nullptr;
}

TileStorageHandle TiledImage::pin_tile_storage(TileCoordinate tile) const {
    return tiles_[tile_index(tile)];
}

TileChangeSet TiledImage::changed_tiles_after(Revision revision) const {
    if (revision > revision_) {
        throw std::out_of_range("tile change query revision is newer than the image");
    }
    TileChangeSet result;
    if (revision == revision_) {
        return result;
    }
    for (auto entry = changed_tiles_by_revision_.upper_bound(revision);
         entry != changed_tiles_by_revision_.end(); ++entry) {
        result.coordinates.push_back(entry->second);
        ++result.indexed_tiles_visited;
    }
    radix_sort_row_major(result.coordinates);
    return result;
}

bool TiledImage::is_tile_dirty(TileCoordinate tile) const { return dirty_[tile_index(tile)]; }

std::vector<TileCoordinate> TiledImage::dirty_tiles() const {
    std::vector<TileCoordinate> result;
    for (std::uint32_t y = 0; y < tile_rows_; ++y) {
        for (std::uint32_t x = 0; x < tile_columns_; ++x) {
            const TileCoordinate coordinate{x, y};
            if (dirty_[tile_index(coordinate)]) {
                result.push_back(coordinate);
            }
        }
    }
    return result;
}

std::span<const std::byte> TiledImage::read_pixel(std::uint32_t x, std::uint32_t y) const {
    if (x >= width_ || y >= height_) {
        throw std::out_of_range("pixel coordinate is outside the image");
    }
    const TileCoordinate coordinate{x / tile_size_, y / tile_size_};
    const auto& tile = tiles_[tile_index(coordinate)];
    if (!tile) {
        return clear_pixel_;
    }
    return std::span<const std::byte>(*tile).subspan(pixel_offset(x, y), pixel_bytes_);
}

void TiledImage::write_pixel(std::uint32_t x, std::uint32_t y, std::span<const std::byte> pixel) {
    if (x >= width_ || y >= height_) {
        throw std::out_of_range("pixel coordinate is outside the image");
    }
    if (pixel.size() != pixel_bytes_) {
        throw std::invalid_argument("pixel size does not match the image format");
    }
    const std::span<const std::byte> existing = read_pixel(x, y);
    if (std::equal(pixel.begin(), pixel.end(), existing.begin(), existing.end())) {
        return;
    }
    const TileCoordinate coordinate{x / tile_size_, y / tile_size_};
    const std::size_t index = tile_index(coordinate);
    if (tile_generations_[index] == std::numeric_limits<Generation>::max()) {
        throw std::overflow_error("tile generation space is exhausted");
    }
    const bool revision_exhausted = revision_ == std::numeric_limits<Revision>::max();
    if (revision_exhausted && revision_epoch_ == std::numeric_limits<RevisionEpoch>::max()) {
        throw std::overflow_error("image revision epoch space is exhausted");
    }
    auto& tile = allocate_tile(index);
    if (revision_exhausted) {
        std::map<Revision, TileCoordinate> next_index;
        next_index.emplace(1, coordinate);
        begin_new_revision_epoch();
        changed_tiles_by_revision_.swap(next_index);
    } else {
        const Revision next_revision = revision_ + 1;
        const auto [unused, inserted] =
            changed_tiles_by_revision_.emplace(next_revision, coordinate);
        static_cast<void>(unused);
        if (!inserted) {
            throw std::logic_error("tile change index revision collision");
        }
        const Revision previous_revision = tile_revisions_[index];
        if (previous_revision != 0) {
            changed_tiles_by_revision_.erase(previous_revision);
        }
    }
    std::copy(pixel.begin(), pixel.end(), tile.begin() + pixel_offset(x, y));
    ++revision_;
    ++tile_generations_[index];
    tile_revisions_[index] = revision_;
    dirty_[index] = true;
}

RevisionCursor TiledImage::reset_revision_history() {
    if (revision_epoch_ == std::numeric_limits<RevisionEpoch>::max()) {
        throw std::overflow_error("image revision epoch space is exhausted");
    }
    begin_new_revision_epoch();
    return revision_cursor();
}

void TiledImage::clear_dirty() noexcept { std::fill(dirty_.begin(), dirty_.end(), false); }

std::size_t TiledImage::tile_index(TileCoordinate tile) const {
    if (tile.x >= tile_columns_ || tile.y >= tile_rows_) {
        throw std::out_of_range("tile coordinate is outside the image");
    }
    return static_cast<std::size_t>(tile.y) * tile_columns_ + tile.x;
}

std::size_t TiledImage::pixel_offset(std::uint32_t x, std::uint32_t y) const noexcept {
    const std::size_t local_x = x % tile_size_;
    const std::size_t local_y = y % tile_size_;
    return ((local_y * tile_size_) + local_x) * pixel_bytes_;
}

std::vector<std::byte>& TiledImage::allocate_tile(std::size_t index) {
    auto& tile = tiles_[index];
    if (tile && tile.unique()) {
        return *tile;
    }
    if (tile) {
        tile = std::make_shared<std::vector<std::byte>>(*tile);
        return *tile;
    }
    tile = std::make_shared<std::vector<std::byte>>(tile_bytes_);
    for (std::size_t offset = 0; offset < tile_bytes_; offset += pixel_bytes_) {
        std::copy(clear_pixel_.begin(), clear_pixel_.end(), tile->begin() + offset);
    }
    return *tile;
}

void TiledImage::begin_new_revision_epoch() noexcept {
    ++revision_epoch_;
    revision_ = 0;
    std::fill(tile_revisions_.begin(), tile_revisions_.end(), 0);
    changed_tiles_by_revision_.clear();
}

}  // namespace ctex::image
