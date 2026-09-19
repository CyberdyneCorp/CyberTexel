#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <ctex/xport/delta.hpp>
#include <exception>
#include <iostream>
#include <string_view>

namespace {

using ctex::doc::TextureChannels;
using ctex::image::TileCoordinate;
using ctex::xport::ChannelDelta;
using ctex::xport::DeltaQueryDisposition;
using ctex::xport::TileResidency;

bool expect(bool condition, std::string_view message) {
    if (!condition) {
        std::cerr << message << '\n';
    }
    return condition;
}

bool empty_query_moves_no_pixels() {
    TextureChannels channels(16384, 16384, 8, ctex::doc::metallic_roughness_channels());
    channels.enable("pbr.base_color");
    const auto initial_cursor = channels.channel_revision_cursor("pbr.base_color");
    const ChannelDelta delta =
        ctex::xport::query_channel_delta_metadata(channels, "pbr.base_color", initial_cursor);
    return expect(delta.disposition == DeltaQueryDisposition::complete &&
                      delta.synchronized_cursor == initial_cursor &&
                      delta.current_cursor == initial_cursor && delta.changed_tiles.empty() &&
                      channels.resident_pixel_bytes() == 0,
                  "an unchanged delta query allocated or returned pixels");
}

bool twenty_operations_are_complete_and_coalesced() {
    TextureChannels channels(16384, 16384, 8, ctex::doc::metallic_roughness_channels());
    channels.enable("pbr.base_color");
    const auto initial_cursor = channels.channel_revision_cursor("pbr.base_color");
    constexpr std::size_t changed_tile_count = 12;
    constexpr std::size_t operation_count = 20;
    std::array<TileCoordinate, changed_tile_count> changed_tiles{};
    for (std::size_t index = 0; index < changed_tiles.size(); ++index) {
        changed_tiles[index] = {static_cast<std::uint32_t>(index),
                                static_cast<std::uint32_t>(index)};
    }

    ctex::doc::ChannelRevisionCursor cursor_after_twelve{};
    for (std::size_t operation = 0; operation < operation_count; ++operation) {
        const TileCoordinate tile = changed_tiles[operation % changed_tiles.size()];
        const std::array value{std::byte{static_cast<unsigned char>(operation + 1)}, std::byte{3},
                               std::byte{5}};
        channels.pixels("pbr.base_color")
            .write_pixel(tile.x * ctex::image::default_tile_size,
                         tile.y * ctex::image::default_tile_size, value);
        if (operation + 1 == changed_tile_count) {
            cursor_after_twelve = channels.channel_revision_cursor("pbr.base_color");
        }
    }

    const std::size_t resident_before_query = channels.resident_pixel_bytes();
    const ChannelDelta complete =
        ctex::xport::query_channel_delta_metadata(channels, "pbr.base_color", initial_cursor);
    bool passed = true;
    passed &= expect(complete.disposition == DeltaQueryDisposition::complete &&
                         complete.synchronized_cursor == initial_cursor &&
                         complete.current_cursor.revision == operation_count &&
                         complete.changed_tiles.size() == changed_tile_count,
                     "twenty operations did not coalesce to the complete changed-tile union");
    for (std::size_t index = 0; index < complete.changed_tiles.size(); ++index) {
        const auto& version = complete.changed_tiles[index];
        const bool changed_twice = index < operation_count - changed_tile_count;
        const auto expected_revision = static_cast<ctex::doc::TileRevision>(
            changed_twice ? changed_tile_count + index + 1 : index + 1);
        passed &= expect(
            version.coordinate == changed_tiles[index] && version.revision == expected_revision &&
                version.generation == (changed_twice ? 2 : 1) &&
                version.residency == TileResidency::cpu,
            "a coalesced delta entry lost its coordinate, revision, generation or residency");
    }
    passed &= expect(channels.resident_pixel_bytes() == resident_before_query,
                     "delta query changed resident pixel storage");

    const ChannelDelta recent =
        ctex::xport::query_channel_delta_metadata(channels, "pbr.base_color", cursor_after_twelve);
    passed &= expect(recent.changed_tiles.size() == operation_count - changed_tile_count,
                     "delta since the caller-held revision omitted or repeated a changed tile");
    for (const auto& version : recent.changed_tiles) {
        passed &= expect(version.revision > changed_tile_count && version.generation == 2,
                         "recent delta returned a stale tile version");
    }

    const auto current_cursor = channels.channel_revision_cursor("pbr.base_color");
    const ChannelDelta current =
        ctex::xport::query_channel_delta_metadata(channels, "pbr.base_color", current_cursor);
    passed &= expect(current.changed_tiles.empty(),
                     "querying from the current revision returned false changes");
    return passed;
}

bool future_revision_is_refused() {
    TextureChannels channels(4, 4, 8, ctex::doc::metallic_roughness_channels());
    channels.enable("pbr.base_color");
    auto future = channels.channel_revision_cursor("pbr.base_color");
    ++future.revision;
    try {
        static_cast<void>(
            ctex::xport::query_channel_delta_metadata(channels, "pbr.base_color", future));
    } catch (const ctex::xport::DeltaQueryError&) {
        return true;
    } catch (const std::exception&) {
    }
    return expect(false, "a future synchronized revision was accepted");
}

bool stale_cursor_requires_full_resynchronization() {
    TextureChannels channels(64, 64, 8, ctex::doc::metallic_roughness_channels());
    channels.enable("pbr.base_color");
    const std::array first_value{std::byte{1}, std::byte{2}, std::byte{3}};
    channels.pixels("pbr.base_color").write_pixel(0, 0, first_value);
    const auto stale_cursor = channels.channel_revision_cursor("pbr.base_color");
    const std::size_t resident_bytes = channels.resident_pixel_bytes();
    std::array<std::byte, 3> pixels_before_reset{};
    const auto pixels = channels.pixels("pbr.base_color").read_pixel(0, 0);
    std::copy(pixels.begin(), pixels.end(), pixels_before_reset.begin());

    const auto reset_cursor = channels.reset_revision_history("pbr.base_color");
    const ChannelDelta stale =
        ctex::xport::query_channel_delta_metadata(channels, "pbr.base_color", stale_cursor);
    const ChannelDelta unknown = ctex::xport::query_channel_delta_metadata(
        channels, "pbr.base_color", ctex::doc::ChannelRevisionCursor{});
    bool passed = true;
    passed &=
        expect(reset_cursor.epoch == stale_cursor.epoch + 1 && reset_cursor.revision == 0 &&
                   stale.disposition == DeltaQueryDisposition::full_resynchronization_required &&
                   stale.current_cursor == reset_cursor && stale.changed_tiles.empty() &&
                   unknown.disposition == DeltaQueryDisposition::full_resynchronization_required,
               "a cursor from an unrelated revision epoch returned a partial delta");
    const auto pixels_after_reset = channels.pixels("pbr.base_color").read_pixel(0, 0);
    passed &= expect(std::equal(pixels_before_reset.begin(), pixels_before_reset.end(),
                                pixels_after_reset.begin(), pixels_after_reset.end()) &&
                         channels.resident_pixel_bytes() == resident_bytes &&
                         channels.tile_revision("pbr.base_color", {0, 0}) == 0,
                     "resetting revision history changed pixels or retained stale tile revisions");

    const std::array second_value{std::byte{5}, std::byte{8}, std::byte{13}};
    channels.pixels("pbr.base_color").write_pixel(0, 0, second_value);
    const ChannelDelta after_reset =
        ctex::xport::query_channel_delta_metadata(channels, "pbr.base_color", reset_cursor);
    passed &= expect(after_reset.disposition == DeltaQueryDisposition::complete &&
                         after_reset.changed_tiles.size() == 1 &&
                         after_reset.changed_tiles.front().revision == 1 &&
                         after_reset.changed_tiles.front().generation == 2,
                     "the new revision epoch did not resume complete delta tracking");
    return passed;
}

}  // namespace

int main() {
    return empty_query_moves_no_pixels() && twenty_operations_are_complete_and_coalesced() &&
                   future_revision_is_refused() && stale_cursor_requires_full_resynchronization()
               ? 0
               : 1;
}
