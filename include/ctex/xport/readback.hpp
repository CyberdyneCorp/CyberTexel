#ifndef CTEX_XPORT_READBACK_HPP
#define CTEX_XPORT_READBACK_HPP

#include <cstddef>
#include <cstdint>
#include <ctex/xport/delta.hpp>
#include <ctex/xport/format.hpp>
#include <ctex/xport/snapshot.hpp>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace ctex::xport {

enum class TileReadbackStatus : std::uint8_t { pending, complete, cancelled, failed };

enum class ChannelOrder : std::uint8_t { r, rg, rgb, rgba };
enum class ComponentByteOrder : std::uint8_t { native };
enum class TileContiguity : std::uint8_t { separate_buffers };

struct TileMemoryLayout {
    std::uint32_t width;
    std::uint32_t height;
    std::size_t row_pitch_bytes;
    std::size_t pixel_stride_bytes;
    ChannelOrder channel_order;
    image::ChannelType component_type;
    ComponentByteOrder component_byte_order;
    TileContiguity tile_contiguity;

    friend constexpr bool operator==(TileMemoryLayout, TileMemoryLayout) noexcept = default;

    [[nodiscard]] constexpr std::size_t byte_size() const noexcept {
        return row_pitch_bytes * height;
    }
};

[[nodiscard]] TileMemoryLayout tile_memory_layout(const doc::TextureChannels& channels,
                                                  std::string_view semantic_id,
                                                  image::TileCoordinate coordinate);
[[nodiscard]] TileMemoryLayout tile_memory_layout(const doc::TextureChannels& channels,
                                                  std::string_view semantic_id,
                                                  image::TileCoordinate coordinate,
                                                  const ReadbackFormatSelection& format);
[[nodiscard]] TileMemoryLayout tile_memory_layout(const PreviewResource& preview,
                                                  image::TileCoordinate coordinate);
[[nodiscard]] TileMemoryLayout tile_memory_layout(const PreviewResource& preview,
                                                  image::TileCoordinate coordinate,
                                                  const ReadbackFormatSelection& format);

struct TileReadbackDestination {
    TileVersion version;
    TileMemoryLayout layout;
    std::span<std::byte> output;
};

struct HostTileCompletion {
    TileVersion version;
    TileMemoryLayout layout;
    std::span<const std::byte> bytes;
};

class TileReadback {
public:
    TileReadback(TileReadback&&) noexcept = default;
    TileReadback& operator=(TileReadback&&) noexcept = default;
    TileReadback(const TileReadback&) = delete;
    TileReadback& operator=(const TileReadback&) = delete;

    [[nodiscard]] static TileReadback begin_cpu(
        const doc::TextureChannels& channels, std::string_view semantic_id,
        doc::ChannelRevisionCursor cursor, std::span<const TileReadbackDestination> destinations);
    [[nodiscard]] static TileReadback begin_cpu(
        const doc::TextureChannels& channels, std::string_view semantic_id,
        doc::ChannelRevisionCursor cursor, const ReadbackFormatSelection& format,
        std::span<const TileReadbackDestination> destinations);
    [[nodiscard]] static TileReadback begin_cpu(
        const SnapshotToken& snapshot, std::span<const TileReadbackDestination> destinations);
    [[nodiscard]] static TileReadback begin_cpu(
        const SnapshotToken& snapshot, const ReadbackFormatSelection& format,
        std::span<const TileReadbackDestination> destinations);
    [[nodiscard]] static TileReadback begin_host(
        std::span<const TileReadbackDestination> destinations);
    [[nodiscard]] static TileReadback begin_host(
        const ReadbackFormatSelection& format,
        std::span<const TileReadbackDestination> destinations);

    [[nodiscard]] TileReadbackStatus status() const noexcept { return status_; }
    [[nodiscard]] bool output_readable() const noexcept {
        return status_ == TileReadbackStatus::complete;
    }
    [[nodiscard]] const std::string& detail() const noexcept { return detail_; }
    [[nodiscard]] std::size_t tile_count() const noexcept { return destinations_.size(); }
    [[nodiscard]] const std::optional<ReadbackFormatSelection>& format_selection() const noexcept {
        return format_selection_;
    }

    [[nodiscard]] bool cancel() noexcept;
    [[nodiscard]] bool complete_host(std::span<const HostTileCompletion> completed_tiles);
    [[nodiscard]] bool fail_host(std::string detail);

private:
    TileReadback(std::vector<TileReadbackDestination> destinations, TileReadbackStatus status,
                 std::string detail, std::optional<ReadbackFormatSelection> format_selection = {});
    [[nodiscard]] static TileReadback failed(
        std::string detail, std::optional<ReadbackFormatSelection> format_selection = {});

    std::vector<TileReadbackDestination> destinations_;
    TileReadbackStatus status_{};
    std::string detail_;
    std::optional<ReadbackFormatSelection> format_selection_;
};

}  // namespace ctex::xport

#endif
