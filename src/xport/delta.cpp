#include <ctex/xport/delta.hpp>
#include <string>

namespace ctex::xport {

ChannelDelta query_channel_delta_metadata(const doc::TextureChannels& channels,
                                          std::string_view semantic_id,
                                          doc::ChannelRevisionCursor synchronized_cursor) {
    const image::TiledImage& image = channels.pixels(semantic_id);
    const doc::ChannelRevisionCursor current_cursor = image.revision_cursor();
    if (synchronized_cursor.epoch != current_cursor.epoch) {
        return {
            .disposition = DeltaQueryDisposition::full_resynchronization_required,
            .synchronized_cursor = synchronized_cursor,
            .current_cursor = current_cursor,
            .changed_tiles = {},
        };
    }
    if (synchronized_cursor.revision > current_cursor.revision) {
        throw DeltaQueryError(
            "synchronized revision " + std::to_string(synchronized_cursor.revision) +
            " is newer than current revision " + std::to_string(current_cursor.revision));
    }

    ChannelDelta result{
        .disposition = DeltaQueryDisposition::complete,
        .synchronized_cursor = synchronized_cursor,
        .current_cursor = current_cursor,
        .changed_tiles = {},
    };
    if (synchronized_cursor == current_cursor) {
        return result;
    }

    for (std::uint32_t y = 0; y < image.tile_rows(); ++y) {
        for (std::uint32_t x = 0; x < image.tile_columns(); ++x) {
            const image::TileCoordinate coordinate{x, y};
            const doc::TileRevision revision = image.tile_revision(coordinate);
            if (revision <= synchronized_cursor.revision) {
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
