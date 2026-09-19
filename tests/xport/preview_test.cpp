#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <ctex/xport/readback.hpp>
#include <iostream>
#include <string_view>
#include <vector>

namespace {

using ctex::image::ChannelType;
using ctex::image::PixelFormat;
using ctex::xport::PreviewResource;
using ctex::xport::ReadbackConversionPolicy;
using ctex::xport::SnapshotPool;
using ctex::xport::TileReadback;
using ctex::xport::TileReadbackDestination;
using ctex::xport::TileReadbackStatus;

bool expect(bool condition, std::string_view message) {
    if (!condition) {
        std::cerr << message << '\n';
    }
    return condition;
}

std::array<std::byte, 2> uint16_bytes(std::uint16_t value) {
    std::array<std::byte, 2> bytes{};
    std::memcpy(bytes.data(), &value, sizeof(value));
    return bytes;
}

bool preview_is_isolated_and_uses_the_transport_types() {
    ctex::doc::TextureChannels committed(2, 1, 8, ctex::doc::metallic_roughness_channels());
    committed.enable("pbr.height", 16);
    const auto committed_cursor = committed.channel_revision_cursor("pbr.height");
    PreviewResource preview("pbr.height", 2, 1, PixelFormat{ChannelType::uint16_unorm, 1});
    const auto initial = preview.revision_cursor();
    preview.write_pixel(0, 0, uint16_bytes(32768));
    preview.write_pixel(1, 0, uint16_bytes(65535));

    const std::array accepted{PixelFormat{ChannelType::uint8_unorm, 1}};
    const auto format = ctex::xport::negotiate_readback_format(
        preview.format(), accepted, ReadbackConversionPolicy::allow_conversion);
    SnapshotPool pool(preview.pixels().tile_bytes());
    auto query = ctex::xport::query_channel_delta(pool, preview, initial);
    if (!expect(query.admitted() && format.compatible(),
                "preview delta or readback format was not admitted")) {
        return false;
    }
    auto& synchronized = *query.synchronized;
    const auto layout = ctex::xport::tile_memory_layout(preview, {0, 0}, *format.selection);
    std::vector<std::byte> output(layout.byte_size(), std::byte{0x7f});
    const std::array destinations{
        TileReadbackDestination{synchronized.delta.changed_tiles.front(), layout, output},
    };
    TileReadback readback =
        TileReadback::begin_cpu(synchronized.snapshot, *format.selection, destinations);

    const auto committed_pixel = committed.pixels("pbr.height").read_pixel(0, 0);
    const bool passed =
        expect(preview.channel_semantic() == "pbr.height" &&
                   synchronized.delta.changed_tiles.size() == 1 &&
                   synchronized.delta.indexed_tiles_visited == 1 &&
                   synchronized.snapshot.cursor() == synchronized.delta.current_cursor,
               "preview did not expose normal delta and snapshot records") &&
        expect(readback.status() == TileReadbackStatus::complete && output.size() == 2 &&
                   output[0] == std::byte{128} && output[1] == std::byte{255},
               "preview snapshot did not use negotiated asynchronous readback") &&
        expect(committed.channel_revision_cursor("pbr.height") == committed_cursor &&
                   std::all_of(committed_pixel.begin(), committed_pixel.end(),
                               [](std::byte value) { return value == std::byte{0}; }),
               "editing the preview modified committed document pixels");
    synchronized.snapshot.release();
    return passed;
}

bool edits_after_query_appear_only_in_the_following_preview_delta() {
    PreviewResource preview("pbr.base_color", 1, 1, PixelFormat{ChannelType::uint8_unorm, 3});
    const auto initial = preview.revision_cursor();
    const std::array first_pixel{std::byte{1}, std::byte{2}, std::byte{3}};
    const std::array second_pixel{std::byte{5}, std::byte{8}, std::byte{13}};
    preview.write_pixel(0, 0, first_pixel);
    SnapshotPool pool(preview.pixels().tile_bytes() * 2);
    auto first_query = ctex::xport::query_channel_delta(pool, preview, initial);
    if (!expect(first_query.admitted(), "first preview snapshot was not admitted")) {
        return false;
    }
    auto& first = *first_query.synchronized;
    const auto first_cursor = first.delta.current_cursor;
    preview.write_pixel(0, 0, second_pixel);
    const auto layout = ctex::xport::tile_memory_layout(preview, {0, 0});
    std::vector<std::byte> first_output(layout.byte_size());
    const std::array first_destinations{
        TileReadbackDestination{first.delta.changed_tiles.front(), layout, first_output},
    };
    TileReadback first_readback = TileReadback::begin_cpu(first.snapshot, first_destinations);

    auto second_query = ctex::xport::query_channel_delta(pool, preview, first_cursor);
    if (!expect(second_query.admitted(), "following preview snapshot was not admitted")) {
        return false;
    }
    auto& second = *second_query.synchronized;
    std::vector<std::byte> second_output(layout.byte_size());
    const std::array second_destinations{
        TileReadbackDestination{second.delta.changed_tiles.front(), layout, second_output},
    };
    TileReadback second_readback = TileReadback::begin_cpu(second.snapshot, second_destinations);

    const bool passed =
        expect(first_readback.status() == TileReadbackStatus::complete &&
                   std::equal(first_pixel.begin(), first_pixel.end(), first_output.begin()),
               "later preview editing tore the queried snapshot") &&
        expect(second_readback.status() == TileReadbackStatus::complete &&
                   std::equal(second_pixel.begin(), second_pixel.end(), second_output.begin()),
               "the following preview delta omitted the later edit");
    first.snapshot.release();
    second.snapshot.release();
    return passed;
}

bool unchanged_preview_uses_no_snapshot_budget() {
    PreviewResource preview("pbr.roughness", 16384, 16384,
                            PixelFormat{ChannelType::uint8_unorm, 1});
    SnapshotPool pool(0);
    auto query = ctex::xport::query_channel_delta(pool, preview, preview.revision_cursor());
    const bool passed =
        expect(query.admitted() && query.synchronized->delta.changed_tiles.empty() &&
                   query.synchronized->delta.indexed_tiles_visited == 0 &&
                   query.synchronized->snapshot.retained_bytes() == 0 &&
                   pool.memory_report().pinned_bytes == 0,
               "unchanged preview scanned pixels or consumed snapshot budget");
    if (query.admitted()) {
        query.synchronized->snapshot.release();
    }
    return passed;
}

}  // namespace

int main() {
    return preview_is_isolated_and_uses_the_transport_types() &&
                   edits_after_query_appear_only_in_the_following_preview_delta() &&
                   unchanged_preview_uses_no_snapshot_budget()
               ? 0
               : 1;
}
