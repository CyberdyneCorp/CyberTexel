#ifndef CTEX_XPORT_READBACK_HPP
#define CTEX_XPORT_READBACK_HPP

#include <cstddef>
#include <ctex/xport/delta.hpp>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace ctex::xport {

enum class TileReadbackStatus : std::uint8_t { pending, complete, cancelled, failed };

struct TileReadbackDestination {
    TileVersion version;
    std::span<std::byte> output;
};

struct HostTileCompletion {
    TileVersion version;
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
    [[nodiscard]] static TileReadback begin_host(
        std::span<const TileReadbackDestination> destinations);

    [[nodiscard]] TileReadbackStatus status() const noexcept { return status_; }
    [[nodiscard]] bool output_readable() const noexcept {
        return status_ == TileReadbackStatus::complete;
    }
    [[nodiscard]] const std::string& detail() const noexcept { return detail_; }
    [[nodiscard]] std::size_t tile_count() const noexcept { return destinations_.size(); }

    [[nodiscard]] bool cancel() noexcept;
    [[nodiscard]] bool complete_host(std::span<const HostTileCompletion> completed_tiles);
    [[nodiscard]] bool fail_host(std::string detail);

private:
    TileReadback(std::vector<TileReadbackDestination> destinations, TileReadbackStatus status,
                 std::string detail);
    [[nodiscard]] static TileReadback failed(std::string detail);

    std::vector<TileReadbackDestination> destinations_;
    TileReadbackStatus status_{};
    std::string detail_;
};

}  // namespace ctex::xport

#endif
