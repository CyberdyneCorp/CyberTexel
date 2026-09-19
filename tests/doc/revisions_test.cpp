#include <array>
#include <cstddef>
#include <cstdint>
#include <ctex/doc/channels.hpp>
#include <iostream>
#include <string_view>

namespace {

using ctex::doc::TextureChannels;
using ctex::image::TileCoordinate;

bool expect(bool condition, std::string_view message) {
    if (!condition) {
        std::cerr << message << '\n';
    }
    return condition;
}

bool revisions_advance_only_for_changed_tiles() {
    constexpr std::uint32_t texture_extent = 16384;
    TextureChannels channels(texture_extent, texture_extent, 8,
                             ctex::doc::metallic_roughness_channels());
    channels.enable("pbr.base_color");
    channels.enable("pbr.opacity");
    const auto& base_color = channels.pixels("pbr.base_color");
    const std::array default_base_color{std::byte{128}, std::byte{128}, std::byte{128}};
    channels.pixels("pbr.base_color").write_pixel(0, 0, default_base_color);
    bool passed = true;
    passed &= expect(channels.channel_revision("pbr.base_color") == 0 &&
                         channels.channel_revision("pbr.opacity") == 0 &&
                         channels.tile_revision("pbr.base_color", {255, 255}) == 0 &&
                         channels.resident_pixel_bytes() == 0,
                     "reading initial channel revisions allocated or changed pixels");

    const std::array changed_tiles{
        TileCoordinate{0, 0},     TileCoordinate{1, 3},     TileCoordinate{2, 7},
        TileCoordinate{4, 15},    TileCoordinate{8, 31},    TileCoordinate{16, 63},
        TileCoordinate{32, 95},   TileCoordinate{64, 127},  TileCoordinate{96, 159},
        TileCoordinate{128, 191}, TileCoordinate{192, 223}, TileCoordinate{255, 255},
    };
    std::array<std::byte, 3> last_value{};
    for (std::size_t index = 0; index < changed_tiles.size(); ++index) {
        last_value = {std::byte{static_cast<unsigned char>(index + 1)}, std::byte{3}, std::byte{5}};
        const TileCoordinate tile = changed_tiles[index];
        channels.pixels("pbr.base_color")
            .write_pixel(tile.x * ctex::image::default_tile_size,
                         tile.y * ctex::image::default_tile_size, last_value);
    }

    std::size_t advanced_tiles = 0;
    for (std::uint32_t y = 0; y < base_color.tile_rows(); ++y) {
        for (std::uint32_t x = 0; x < base_color.tile_columns(); ++x) {
            advanced_tiles += channels.tile_revision("pbr.base_color", {x, y}) != 0 ? 1 : 0;
        }
    }
    passed &= expect(channels.channel_revision("pbr.base_color") == changed_tiles.size() &&
                         advanced_tiles == changed_tiles.size() &&
                         channels.channel_revision("pbr.opacity") == 0,
                     "a twelve-tile change did not advance exactly twelve tile revisions");

    const TileCoordinate last_tile = changed_tiles.back();
    const auto revision_before_noop = channels.channel_revision("pbr.base_color");
    channels.pixels("pbr.base_color")
        .write_pixel(last_tile.x * ctex::image::default_tile_size,
                     last_tile.y * ctex::image::default_tile_size, last_value);
    passed &=
        expect(channels.channel_revision("pbr.base_color") == revision_before_noop &&
                   channels.tile_revision("pbr.base_color", last_tile) == revision_before_noop,
               "an identical write advanced revision metadata");

    const std::array changed_again{std::byte{21}, std::byte{34}, std::byte{55}};
    channels.pixels("pbr.base_color")
        .write_pixel(last_tile.x * ctex::image::default_tile_size,
                     last_tile.y * ctex::image::default_tile_size, changed_again);
    passed &=
        expect(channels.channel_revision("pbr.base_color") == revision_before_noop + 1 &&
                   channels.tile_revision("pbr.base_color", last_tile) == revision_before_noop + 1,
               "a subsequent content change did not publish its channel revision on the tile");
    return passed;
}

}  // namespace

int main() { return revisions_advance_only_for_changed_tiles() ? 0 : 1; }
