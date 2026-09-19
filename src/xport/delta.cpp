#include <ctex/xport/delta.hpp>
#include <string>

namespace ctex::xport {

ChannelDelta query_channel_delta(const doc::TextureChannels& channels, std::string_view semantic_id,
                                 doc::ChannelRevision synchronized_revision) {
    const image::TiledImage& image = channels.pixels(semantic_id);
    const doc::ChannelRevision current_revision = image.revision();
    if (synchronized_revision > current_revision) {
        throw DeltaQueryError("synchronized revision " + std::to_string(synchronized_revision) +
                              " is newer than current revision " +
                              std::to_string(current_revision));
    }

    ChannelDelta result{
        .synchronized_revision = synchronized_revision,
        .current_revision = current_revision,
        .changed_tiles = {},
    };
    if (synchronized_revision == current_revision) {
        return result;
    }

    for (std::uint32_t y = 0; y < image.tile_rows(); ++y) {
        for (std::uint32_t x = 0; x < image.tile_columns(); ++x) {
            const image::TileCoordinate coordinate{x, y};
            const doc::TileRevision revision = image.tile_revision(coordinate);
            if (revision <= synchronized_revision) {
                continue;
            }
            result.changed_tiles.push_back({
                .coordinate = coordinate,
                .revision = revision,
                .generation = image.tile_generation(coordinate),
                .residency = TileResidency::cpu,
            });
        }
    }
    return result;
}

}  // namespace ctex::xport
