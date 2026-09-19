#include <algorithm>
#include <ctex/xport/snapshot.hpp>
#include <limits>
#include <mutex>
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace ctex::xport {

struct SnapshotPool::State {
    struct Allocation {
        std::size_t bytes;
        std::size_t references;
    };

    explicit State(std::size_t limit) : budget_bytes(limit) {}

    bool acquire(std::span<const image::TileStorageHandle> storage, std::size_t& additional_bytes) {
        std::vector<const void*> acquired;
        acquired.reserve(storage.size());
        std::scoped_lock lock(mutex);
        std::unordered_set<const void*> request_allocations;
        additional_bytes = 0;
        for (const auto& allocation : storage) {
            const void* identity = allocation.get();
            if (!request_allocations.insert(identity).second || allocations.contains(identity)) {
                continue;
            }
            if (allocation->size() > std::numeric_limits<std::size_t>::max() - additional_bytes) {
                throw std::overflow_error("snapshot pinned byte count overflows");
            }
            additional_bytes += allocation->size();
        }
        if (additional_bytes > budget_bytes - std::min(budget_bytes, pinned_bytes)) {
            return false;
        }
        try {
            for (const auto& allocation : storage) {
                const void* identity = allocation.get();
                auto [entry, inserted] = allocations.try_emplace(
                    identity, Allocation{.bytes = allocation->size(), .references = 0});
                if (inserted) {
                    pinned_bytes += allocation->size();
                }
                ++entry->second.references;
                acquired.push_back(identity);
            }
        } catch (...) {
            for (const void* identity : acquired) {
                auto found = allocations.find(identity);
                if (--found->second.references == 0) {
                    pinned_bytes -= found->second.bytes;
                    allocations.erase(found);
                }
            }
            throw;
        }
        ++active_snapshots;
        return true;
    }

    void release(std::span<const image::TileStorageHandle> storage) noexcept {
        std::scoped_lock lock(mutex);
        for (const auto& allocation : storage) {
            const auto found = allocations.find(allocation.get());
            if (found == allocations.end()) {
                continue;
            }
            if (--found->second.references == 0) {
                pinned_bytes -= found->second.bytes;
                allocations.erase(found);
            }
        }
        if (active_snapshots != 0) {
            --active_snapshots;
        }
    }

    [[nodiscard]] SnapshotMemoryReport report() const noexcept {
        std::scoped_lock lock(mutex);
        return {
            .budget_bytes = budget_bytes,
            .pinned_bytes = pinned_bytes,
            .active_snapshots = active_snapshots,
            .pinned_allocations = allocations.size(),
        };
    }

    std::size_t budget_bytes;
    mutable std::mutex mutex;
    std::size_t pinned_bytes{};
    std::size_t active_snapshots{};
    std::unordered_map<const void*, Allocation> allocations;
};

struct SnapshotToken::Impl {
    struct PinnedTile {
        TileVersion version;
        image::TileExtent extent;
        image::TileStorageHandle storage;
    };

    ~Impl() { release(); }

    void release() noexcept {
        if (!registered) {
            return;
        }
        pool->release(storage);
        registered = false;
        tiles.clear();
        versions.clear();
        storage.clear();
    }

    std::shared_ptr<SnapshotPool::State> pool;
    doc::ChannelRevisionCursor cursor;
    image::PixelFormat format;
    std::uint32_t tile_size{};
    std::vector<PinnedTile> tiles;
    std::vector<TileVersion> versions;
    std::vector<image::TileStorageHandle> storage;
    std::size_t retained_bytes{};
    bool registered{};
};

SnapshotToken::SnapshotToken(std::unique_ptr<Impl> impl) noexcept : impl_(std::move(impl)) {}
SnapshotToken::SnapshotToken(SnapshotToken&&) noexcept = default;
SnapshotToken& SnapshotToken::operator=(SnapshotToken&&) noexcept = default;
SnapshotToken::~SnapshotToken() = default;

bool SnapshotToken::active() const noexcept { return impl_ && impl_->registered; }

doc::ChannelRevisionCursor SnapshotToken::cursor() const noexcept {
    return active() ? impl_->cursor : doc::ChannelRevisionCursor{};
}

image::PixelFormat SnapshotToken::source_format() const noexcept {
    return active() ? impl_->format : image::PixelFormat{};
}

std::span<const TileVersion> SnapshotToken::versions() const noexcept {
    return active() ? std::span<const TileVersion>(impl_->versions)
                    : std::span<const TileVersion>{};
}

std::size_t SnapshotToken::retained_bytes() const noexcept {
    return active() ? impl_->retained_bytes : 0;
}

void SnapshotToken::release() noexcept {
    if (impl_) {
        impl_->release();
    }
}

SnapshotToken::PinnedTileAccess SnapshotToken::access(const TileVersion& version) const {
    if (!active()) {
        throw std::logic_error("snapshot token is released");
    }
    const auto found = std::find_if(impl_->tiles.begin(), impl_->tiles.end(),
                                    [&](const auto& tile) { return tile.version == version; });
    if (found == impl_->tiles.end()) {
        throw std::invalid_argument("tile version is not pinned by this snapshot");
    }
    return {
        .extent = found->extent,
        .tile_size = impl_->tile_size,
        .format = impl_->format,
        .bytes = *found->storage,
    };
}

SnapshotPool::SnapshotPool(std::size_t budget_bytes)
    : state_(std::make_shared<State>(budget_bytes)) {}

SnapshotMemoryReport SnapshotPool::memory_report() const noexcept { return state_->report(); }

SnapshotQueryResult query_channel_delta(SnapshotPool& pool, const doc::TextureChannels& channels,
                                        std::string_view semantic_id,
                                        doc::ChannelRevisionCursor synchronized_cursor) {
    ChannelDelta delta = query_channel_delta_metadata(channels, semantic_id, synchronized_cursor);
    const image::TiledImage& image = channels.pixels(semantic_id);
    auto impl = std::make_unique<SnapshotToken::Impl>();
    impl->pool = pool.state_;
    impl->cursor = delta.current_cursor;
    impl->format = image.format();
    impl->tile_size = image.tile_size();
    impl->tiles.reserve(delta.changed_tiles.size());
    impl->versions = delta.changed_tiles;

    impl->storage.reserve(delta.changed_tiles.size());
    for (const TileVersion& version : delta.changed_tiles) {
        image::TileStorageHandle allocation = image.pin_tile_storage(version.coordinate);
        if (!allocation) {
            throw std::logic_error("changed tile has no storage to pin");
        }
        if (allocation->size() > std::numeric_limits<std::size_t>::max() - impl->retained_bytes) {
            throw std::overflow_error("snapshot retained byte count overflows");
        }
        impl->retained_bytes += allocation->size();
        impl->storage.push_back(allocation);
        impl->tiles.push_back({
            .version = version,
            .extent = image.tile_extent(version.coordinate),
            .storage = std::move(allocation),
        });
    }
    if (image.revision_cursor() != delta.current_cursor) {
        throw DeltaQueryError("channel changed while its snapshot was being captured");
    }

    std::size_t additional_bytes = 0;
    if (!pool.state_->acquire(impl->storage, additional_bytes)) {
        const SnapshotMemoryReport report = pool.memory_report();
        return {
            .status = SnapshotQueryStatus::over_budget,
            .synchronized = std::nullopt,
            .additional_pinned_bytes = additional_bytes,
            .detail = "snapshot requires " + std::to_string(additional_bytes) +
                      " additional pinned bytes with " + std::to_string(report.pinned_bytes) +
                      " already pinned under a " + std::to_string(report.budget_bytes) +
                      " byte budget",
        };
    }
    impl->registered = true;
    return {
        .status = SnapshotQueryStatus::admitted,
        .synchronized = SnapshotDelta{std::move(delta), SnapshotToken(std::move(impl))},
        .additional_pinned_bytes = additional_bytes,
        .detail = {},
    };
}

}  // namespace ctex::xport
