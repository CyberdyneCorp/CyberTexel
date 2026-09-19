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
    const ChannelDelta delta = ctex::xport::query_channel_delta(channels, "pbr.base_color", 0);
    return expect(delta.synchronized_revision == 0 && delta.current_revision == 0 &&
                      delta.changed_tiles.empty() && channels.resident_pixel_bytes() == 0,
                  "an unchanged delta query allocated or returned pixels");
}

bool twenty_operations_are_complete_and_coalesced() {
    TextureChannels channels(16384, 16384, 8, ctex::doc::metallic_roughness_channels());
    channels.enable("pbr.base_color");
    constexpr std::size_t changed_tile_count = 12;
    constexpr std::size_t operation_count = 20;
    std::array<TileCoordinate, changed_tile_count> changed_tiles{};
    for (std::size_t index = 0; index < changed_tiles.size(); ++index) {
        changed_tiles[index] = {static_cast<std::uint32_t>(index),
                                static_cast<std::uint32_t>(index)};
    }

    for (std::size_t operation = 0; operation < operation_count; ++operation) {
        const TileCoordinate tile = changed_tiles[operation % changed_tiles.size()];
        const std::array value{std::byte{static_cast<unsigned char>(operation + 1)}, std::byte{3},
                               std::byte{5}};
        channels.pixels("pbr.base_color")
            .write_pixel(tile.x * ctex::image::default_tile_size,
                         tile.y * ctex::image::default_tile_size, value);
    }

    const std::size_t resident_before_query = channels.resident_pixel_bytes();
    const ChannelDelta complete = ctex::xport::query_channel_delta(channels, "pbr.base_color", 0);
    bool passed = true;
    passed &= expect(complete.synchronized_revision == 0 &&
                         complete.current_revision == operation_count &&
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
        ctex::xport::query_channel_delta(channels, "pbr.base_color", changed_tile_count);
    passed &= expect(recent.changed_tiles.size() == operation_count - changed_tile_count,
                     "delta since the caller-held revision omitted or repeated a changed tile");
    for (const auto& version : recent.changed_tiles) {
        passed &= expect(version.revision > changed_tile_count && version.generation == 2,
                         "recent delta returned a stale tile version");
    }

    const ChannelDelta current =
        ctex::xport::query_channel_delta(channels, "pbr.base_color", operation_count);
    passed &= expect(current.changed_tiles.empty(),
                     "querying from the current revision returned false changes");
    return passed;
}

bool future_revision_is_refused() {
    TextureChannels channels(4, 4, 8, ctex::doc::metallic_roughness_channels());
    channels.enable("pbr.base_color");
    try {
        static_cast<void>(ctex::xport::query_channel_delta(channels, "pbr.base_color", 1));
    } catch (const ctex::xport::DeltaQueryError&) {
        return true;
    } catch (const std::exception&) {
    }
    return expect(false, "a future synchronized revision was accepted");
}

}  // namespace

int main() {
    return empty_query_moves_no_pixels() && twenty_operations_are_complete_and_coalesced() &&
                   future_revision_is_refused()
               ? 0
               : 1;
}
