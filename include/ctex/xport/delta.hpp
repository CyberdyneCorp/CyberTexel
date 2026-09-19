#ifndef CTEX_XPORT_DELTA_HPP
#define CTEX_XPORT_DELTA_HPP

#include <cstdint>
#include <ctex/doc/channels.hpp>
#include <stdexcept>
#include <string_view>
#include <vector>

namespace ctex::xport {

enum class TileResidency : std::uint8_t { cpu, host_device };

struct TileVersion {
    image::TileCoordinate coordinate;
    doc::TileRevision revision{};
    image::Generation generation{};
    TileResidency residency{};
    friend bool operator==(const TileVersion&, const TileVersion&) = default;
};

struct ChannelDelta {
    doc::ChannelRevision synchronized_revision{};
    doc::ChannelRevision current_revision{};
    std::vector<TileVersion> changed_tiles;
};

class DeltaQueryError final : public std::invalid_argument {
public:
    using std::invalid_argument::invalid_argument;
};

[[nodiscard]] ChannelDelta query_channel_delta(const doc::TextureChannels& channels,
                                               std::string_view semantic_id,
                                               doc::ChannelRevision synchronized_revision);

}  // namespace ctex::xport

#endif
