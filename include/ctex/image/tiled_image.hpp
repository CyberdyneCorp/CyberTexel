#ifndef CTEX_IMAGE_TILED_IMAGE_HPP
#define CTEX_IMAGE_TILED_IMAGE_HPP

#include <cstddef>
#include <cstdint>
#include <ctex/image/pixel_format.hpp>
#include <map>
#include <memory>
#include <memory_resource>
#include <span>
#include <string_view>
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

struct TileBackingKey {
    std::uint64_t namespace_identity{};
    TileCoordinate coordinate{};
    Generation generation{};
    friend constexpr bool operator==(TileBackingKey, TileBackingKey) noexcept = default;
};

class TileBackingStore {
public:
    virtual ~TileBackingStore() = default;
    [[nodiscard]] virtual bool store(TileBackingKey key, std::span<const std::byte> bytes) = 0;
    [[nodiscard]] virtual bool load(TileBackingKey key, std::span<std::byte> bytes) = 0;
    virtual void discard(TileBackingKey key) noexcept = 0;
    virtual void release_namespace(std::uint64_t namespace_identity) noexcept = 0;
};

enum class TileEvictionStatus : std::uint8_t {
    evicted,
    sparse,
    already_evicted,
    pinned,
    no_backing_store,
    backing_store_failed,
};

struct TileEvictionReport {
    TileEvictionStatus status{};
    std::size_t resident_bytes_released{};
    std::size_t backing_bytes_written{};
};

class TiledImage {
public:
    TiledImage(std::uint32_t width, std::uint32_t height, PixelFormat format,
               std::uint32_t tile_size = default_tile_size,
               std::span<const std::byte> clear_pixel = {},
               std::pmr::memory_resource* memory_resource = std::pmr::get_default_resource());
    TiledImage(const TiledImage& other);
    TiledImage& operator=(const TiledImage& other);
    TiledImage(TiledImage&& other) noexcept;
    TiledImage& operator=(TiledImage&& other) noexcept;
    ~TiledImage();

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
    [[nodiscard]] std::size_t backed_pixel_bytes() const noexcept;
    [[nodiscard]] Revision revision() const noexcept { return revision_; }
    [[nodiscard]] RevisionCursor revision_cursor() const noexcept {
        return {revision_epoch_, revision_};
    }

    [[nodiscard]] TileExtent tile_extent(TileCoordinate tile) const;
    [[nodiscard]] Revision tile_revision(TileCoordinate tile) const;
    [[nodiscard]] Generation tile_generation(TileCoordinate tile) const;
    [[nodiscard]] bool is_tile_allocated(TileCoordinate tile) const;
    [[nodiscard]] bool is_tile_resident(TileCoordinate tile) const;
    [[nodiscard]] bool is_tile_backed(TileCoordinate tile) const;
    void set_backing_store(std::shared_ptr<TileBackingStore> backing_store);
    [[nodiscard]] TileEvictionReport evict_tile(TileCoordinate tile);
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
    [[nodiscard]] std::shared_ptr<TileStorage> ensure_resident(std::size_t index) const;
    [[nodiscard]] TileBackingKey backing_key(std::size_t index) const noexcept;
    void discard_backing(std::size_t index) noexcept;
    void begin_new_revision_epoch() noexcept;

    struct BackingNamespace {};

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
    mutable std::pmr::vector<std::shared_ptr<TileStorage>> tiles_;
    mutable std::pmr::vector<bool> backed_tiles_;
    std::pmr::vector<TileCoordinate> allocated_tiles_;
    std::pmr::vector<bool> dirty_;
    RevisionEpoch revision_epoch_{1};
    Revision revision_{};
    std::pmr::vector<Revision> tile_revisions_;
    std::pmr::vector<Generation> tile_generations_;
    std::pmr::map<Revision, TileCoordinate> changed_tiles_by_revision_;
    std::shared_ptr<TileBackingStore> backing_store_;
    std::unique_ptr<BackingNamespace> backing_namespace_;
};

[[nodiscard]] std::string_view tile_eviction_status_name(TileEvictionStatus status) noexcept;

}  // namespace ctex::image

#endif
