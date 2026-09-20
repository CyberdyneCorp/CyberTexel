#ifndef CTEX_XPORT_SNAPSHOT_HPP
#define CTEX_XPORT_SNAPSHOT_HPP

#include <cstddef>
#include <cstdint>
#include <ctex/xport/delta.hpp>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>

namespace ctex::xport {

class TileReadback;
class SnapshotPool;
struct SnapshotQueryResult;

struct SnapshotMemoryReport {
    std::size_t budget_bytes{};
    std::size_t pinned_bytes{};
    std::size_t active_snapshots{};
    std::size_t pinned_allocations{};
};

class SnapshotToken {
public:
    SnapshotToken(SnapshotToken&&) noexcept;
    SnapshotToken& operator=(SnapshotToken&&) noexcept;
    SnapshotToken(const SnapshotToken&) = delete;
    SnapshotToken& operator=(const SnapshotToken&) = delete;
    ~SnapshotToken();

    [[nodiscard]] bool active() const noexcept;
    [[nodiscard]] doc::ChannelRevisionCursor cursor() const noexcept;
    [[nodiscard]] image::PixelFormat source_format() const noexcept;
    [[nodiscard]] std::span<const TileVersion> versions() const noexcept;
    [[nodiscard]] std::size_t retained_bytes() const noexcept;
    void release() noexcept;

private:
    struct PinnedTileAccess {
        image::TileExtent extent;
        std::uint32_t tile_size;
        image::PixelFormat format;
        std::span<const std::byte> bytes;
    };
    struct Impl;

    explicit SnapshotToken(std::unique_ptr<Impl> impl) noexcept;
    [[nodiscard]] PinnedTileAccess access(const TileVersion& version) const;

    std::unique_ptr<Impl> impl_;

    friend class TileReadback;
    friend class SnapshotPool;
    friend SnapshotQueryResult query_channel_delta(SnapshotPool&, const doc::TextureChannels&,
                                                   std::string_view, doc::ChannelRevisionCursor);
    friend SnapshotQueryResult query_channel_delta(SnapshotPool&, const image::TiledImage&,
                                                   doc::ChannelRevisionCursor);
    friend SnapshotQueryResult query_channel_delta(SnapshotPool&, const PreviewResource&,
                                                   doc::ChannelRevisionCursor);
};

class SnapshotPool {
public:
    explicit SnapshotPool(std::size_t budget_bytes);
    SnapshotPool(SnapshotPool&&) noexcept = default;
    SnapshotPool& operator=(SnapshotPool&&) noexcept = default;
    SnapshotPool(const SnapshotPool&) = delete;
    SnapshotPool& operator=(const SnapshotPool&) = delete;

    [[nodiscard]] SnapshotMemoryReport memory_report() const noexcept;

private:
    struct State;
    [[nodiscard]] SnapshotQueryResult capture(const image::TiledImage& image, ChannelDelta delta);
    std::shared_ptr<State> state_;

    friend struct SnapshotToken::Impl;
    friend SnapshotQueryResult query_channel_delta(SnapshotPool&, const doc::TextureChannels&,
                                                   std::string_view, doc::ChannelRevisionCursor);
    friend SnapshotQueryResult query_channel_delta(SnapshotPool&, const image::TiledImage&,
                                                   doc::ChannelRevisionCursor);
    friend SnapshotQueryResult query_channel_delta(SnapshotPool&, const PreviewResource&,
                                                   doc::ChannelRevisionCursor);
};

struct SnapshotDelta {
    ChannelDelta delta;
    SnapshotToken snapshot;
};

enum class SnapshotQueryStatus : std::uint8_t { admitted, over_budget };

struct SnapshotQueryResult {
    SnapshotQueryStatus status;
    std::optional<SnapshotDelta> synchronized;
    std::size_t additional_pinned_bytes{};
    std::string detail;

    [[nodiscard]] constexpr bool admitted() const noexcept {
        return status == SnapshotQueryStatus::admitted && synchronized.has_value();
    }
};

[[nodiscard]] SnapshotQueryResult query_channel_delta(
    SnapshotPool& pool, const doc::TextureChannels& channels, std::string_view semantic_id,
    doc::ChannelRevisionCursor synchronized_cursor);
[[nodiscard]] SnapshotQueryResult query_channel_delta(
    SnapshotPool& pool, const image::TiledImage& image,
    doc::ChannelRevisionCursor synchronized_cursor);
[[nodiscard]] SnapshotQueryResult query_channel_delta(
    SnapshotPool& pool, const PreviewResource& preview,
    doc::ChannelRevisionCursor synchronized_cursor);

}  // namespace ctex::xport

#endif
