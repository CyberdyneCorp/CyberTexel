#include <ctex/xport/delta.hpp>
#include <string>

namespace ctex::xport {
namespace {

ChannelDelta query_image_delta(const image::TiledImage& image,
                               doc::ChannelRevisionCursor synchronized_cursor) {
    const doc::ChannelRevisionCursor current_cursor = image.revision_cursor();
    if (synchronized_cursor.epoch != current_cursor.epoch) {
        return {
            .disposition = DeltaQueryDisposition::full_resynchronization_required,
            .synchronized_cursor = synchronized_cursor,
            .current_cursor = current_cursor,
            .changed_tiles = {},
            .indexed_tiles_visited = 0,
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
        .indexed_tiles_visited = 0,
    };
    if (synchronized_cursor == current_cursor) {
        return result;
    }

    const image::TileChangeSet changes = image.changed_tiles_after(synchronized_cursor.revision);
    result.indexed_tiles_visited = changes.indexed_tiles_visited;
    result.changed_tiles.reserve(changes.coordinates.size());
    for (const image::TileCoordinate coordinate : changes.coordinates) {
        result.changed_tiles.push_back({
            .coordinate = coordinate,
            .revision = image.tile_revision(coordinate),
            .generation = image.tile_generation(coordinate),
            .residency = TileResidency::cpu,
        });
    }
    return result;
}

}  // namespace

ChannelDelta query_channel_delta_metadata(const doc::TextureChannels& channels,
                                          std::string_view semantic_id,
                                          doc::ChannelRevisionCursor synchronized_cursor) {
    return query_image_delta(channels.pixels(semantic_id), synchronized_cursor);
}

ChannelDelta query_channel_delta_metadata(const PreviewResource& preview,
                                          doc::ChannelRevisionCursor synchronized_cursor) {
    return query_image_delta(preview.pixels(), synchronized_cursor);
}

}  // namespace ctex::xport
