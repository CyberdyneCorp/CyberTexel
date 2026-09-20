#ifndef CTEX_XPORT_DELTA_HPP
#define CTEX_XPORT_DELTA_HPP

#include <cstdint>
#include <ctex/doc/channels.hpp>
#include <ctex/xport/preview.hpp>
#include <stdexcept>
#include <string_view>
#include <vector>

namespace ctex::xport {

enum class TileResidency : std::uint8_t { cpu, host_device };
enum class DeltaQueryDisposition : std::uint8_t { complete, full_resynchronization_required };

struct TileVersion {
    image::TileCoordinate coordinate;
    doc::TileRevision revision{};
    image::Generation generation{};
    TileResidency residency{};
    friend bool operator==(const TileVersion&, const TileVersion&) = default;
};

struct ChannelDelta {
    DeltaQueryDisposition disposition{};
    doc::ChannelRevisionCursor synchronized_cursor;
    doc::ChannelRevisionCursor current_cursor;
    std::vector<TileVersion> changed_tiles;
    std::size_t indexed_tiles_visited{};
};

class DeltaQueryError final : public std::invalid_argument {
public:
    using std::invalid_argument::invalid_argument;
};

[[nodiscard]] ChannelDelta query_channel_delta_metadata(
    const doc::TextureChannels& channels, std::string_view semantic_id,
    doc::ChannelRevisionCursor synchronized_cursor);
[[nodiscard]] ChannelDelta query_channel_delta_metadata(
    const image::TiledImage& image, doc::ChannelRevisionCursor synchronized_cursor);
[[nodiscard]] ChannelDelta query_channel_delta_metadata(
    const PreviewResource& preview, doc::ChannelRevisionCursor synchronized_cursor);

}  // namespace ctex::xport

#endif
