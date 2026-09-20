#ifndef CTEX_IMAGE_TILED_IMAGE_HPP
#define CTEX_IMAGE_TILED_IMAGE_HPP

#include <cstddef>
#include <cstdint>
#include <ctex/image/pixel_format.hpp>
#include <map>
#include <memory>
#include <memory_resource>
#include <span>
#include <utility>
#include <vector>

namespace ctex::image {

inline constexpr std::uint32_t default_tile_size = 64;
using Revision = std::uint64_t;
using RevisionEpoch = std::uint64_t;
using Generation = std::uint64_t;
using TileStorage = std::pmr::vector<std::byte>;
using TileStorageHandle = std::shared_ptr<const TileStorage>;

class TileStorageSnapshot {
public:
    TileStorageSnapshot() = default;
    [[nodiscard]] bool empty() const noexcept { return storage_ == nullptr; }
    [[nodiscard]] std::size_t size() const noexcept { return storage_ ? storage_->size() : 0; }
    [[nodiscard]] const void* identity() const noexcept { return storage_.get(); }
    [[nodiscard]] std::span<const std::byte> bytes() const noexcept {
        return storage_ ? std::span<const std::byte>(*storage_) : std::span<const std::byte>{};
    }

private:
    friend class TiledImage;
    explicit TileStorageSnapshot(std::shared_ptr<TileStorage> storage)
        : storage_(std::move(storage)) {}
    std::shared_ptr<TileStorage> storage_;
};

struct RevisionCursor {
    RevisionEpoch epoch{};
    Revision revision{};
    friend constexpr bool operator==(RevisionCursor, RevisionCursor) noexcept = default;
};

struct TileCoordinate {
    std::uint32_t x;
    std::uint32_t y;

    friend constexpr bool operator==(TileCoordinate, TileCoordinate) noexcept = default;
};

struct TileExtent {
    std::uint32_t width;
    std::uint32_t height;

    friend constexpr bool operator==(TileExtent, TileExtent) noexcept = default;
};

struct TileChangeSet {
    std::vector<TileCoordinate> coordinates;
    std::size_t indexed_tiles_visited{};
};

class TiledImage {
public:
    TiledImage(std::uint32_t width, std::uint32_t height, PixelFormat format,
               std::uint32_t tile_size = default_tile_size,
               std::span<const std::byte> clear_pixel = {},
               std::pmr::memory_resource* memory_resource = std::pmr::get_default_resource());
    TiledImage(const TiledImage& other);
    TiledImage& operator=(const TiledImage& other);
    TiledImage(TiledImage&& other) noexcept = default;
    TiledImage& operator=(TiledImage&& other) noexcept;

    [[nodiscard]] std::uint32_t width() const noexcept { return width_; }
    [[nodiscard]] std::uint32_t height() const noexcept { return height_; }
    [[nodiscard]] std::uint32_t tile_size() const noexcept { return tile_size_; }
    [[nodiscard]] std::uint32_t tile_columns() const noexcept { return tile_columns_; }
    [[nodiscard]] std::uint32_t tile_rows() const noexcept { return tile_rows_; }
    [[nodiscard]] PixelFormat format() const noexcept { return format_; }
    [[nodiscard]] std::size_t pixel_bytes() const noexcept { return pixel_bytes_; }
    [[nodiscard]] std::size_t tile_bytes() const noexcept { return tile_bytes_; }
    [[nodiscard]] std::span<const std::byte> clear_pixel() const noexcept { return clear_pixel_; }
    [[nodiscard]] std::size_t resident_pixel_bytes() const noexcept;
    [[nodiscard]] Revision revision() const noexcept { return revision_; }
    [[nodiscard]] RevisionCursor revision_cursor() const noexcept {
        return {revision_epoch_, revision_};
    }

    [[nodiscard]] TileExtent tile_extent(TileCoordinate tile) const;
    [[nodiscard]] Revision tile_revision(TileCoordinate tile) const;
    [[nodiscard]] Generation tile_generation(TileCoordinate tile) const;
    [[nodiscard]] bool is_tile_allocated(TileCoordinate tile) const;
    [[nodiscard]] TileStorageHandle pin_tile_storage(TileCoordinate tile) const;
    [[nodiscard]] TileStorageSnapshot snapshot_tile_storage(TileCoordinate tile) const;
    void prepare_tile_storage_exchanges(std::size_t maximum_new_allocations);
    [[nodiscard]] TileStorageSnapshot exchange_tile_storage(TileCoordinate tile,
                                                            TileStorageSnapshot replacement);
    [[nodiscard]] std::vector<TileCoordinate> allocated_tiles() const;
    [[nodiscard]] TileChangeSet changed_tiles_after(Revision revision) const;
    [[nodiscard]] bool is_tile_dirty(TileCoordinate tile) const;
    [[nodiscard]] std::vector<TileCoordinate> dirty_tiles() const;

    [[nodiscard]] std::span<const std::byte> read_pixel(std::uint32_t x, std::uint32_t y) const;
    void write_pixel(std::uint32_t x, std::uint32_t y, std::span<const std::byte> pixel);
    [[nodiscard]] bool can_clear() const noexcept;
    void clear();
    [[nodiscard]] RevisionCursor reset_revision_history();
    void clear_dirty() noexcept;

private:
    [[nodiscard]] std::size_t tile_index(TileCoordinate tile) const;
    [[nodiscard]] std::size_t pixel_offset(std::uint32_t x, std::uint32_t y) const noexcept;
    [[nodiscard]] TileStorage& allocate_tile(std::size_t index);
    void begin_new_revision_epoch() noexcept;

    std::uint32_t width_;
    std::uint32_t height_;
    std::uint32_t tile_size_;
    std::uint32_t tile_columns_;
    std::uint32_t tile_rows_;
    PixelFormat format_;
    std::size_t pixel_bytes_;
    std::size_t tile_bytes_;
    std::pmr::memory_resource* memory_resource_;
    std::pmr::vector<std::byte> clear_pixel_;
    std::pmr::vector<std::shared_ptr<TileStorage>> tiles_;
    std::pmr::vector<TileCoordinate> allocated_tiles_;
    std::pmr::vector<bool> dirty_;
    RevisionEpoch revision_epoch_{1};
    Revision revision_{};
    std::pmr::vector<Revision> tile_revisions_;
    std::pmr::vector<Generation> tile_generations_;
    std::pmr::map<Revision, TileCoordinate> changed_tiles_by_revision_;
};

}  // namespace ctex::image

#endif
