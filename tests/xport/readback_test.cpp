#include <algorithm>
#include <array>
#include <cstddef>
#include <ctex/xport/readback.hpp>
#include <iostream>
#include <span>
#include <stdexcept>
#include <string_view>
#include <type_traits>
#include <vector>

namespace {

using ctex::doc::TextureChannels;
using ctex::image::TileCoordinate;
using ctex::xport::ChannelOrder;
using ctex::xport::ComponentByteOrder;
using ctex::xport::HostTileCompletion;
using ctex::xport::TileContiguity;
using ctex::xport::TileMemoryLayout;
using ctex::xport::TileReadback;
using ctex::xport::TileReadbackDestination;
using ctex::xport::TileReadbackStatus;
using ctex::xport::TileResidency;
using ctex::xport::TileVersion;

static_assert(!std::is_copy_constructible_v<TileReadback>);
static_assert(std::is_move_constructible_v<TileReadback>);

bool expect(bool condition, std::string_view message) {
    if (!condition) {
        std::cerr << message << '\n';
    }
    return condition;
}

const TileVersion& version_at(const std::vector<TileVersion>& versions, TileCoordinate coordinate) {
    const auto found = std::find_if(versions.begin(), versions.end(), [&](const auto& version) {
        return version.coordinate == coordinate;
    });
    if (found == versions.end()) {
        throw std::logic_error("test delta did not contain the requested tile");
    }
    return *found;
}

bool cpu_readback_copies_only_named_tiles() {
    TextureChannels channels(70, 70, 8, ctex::doc::metallic_roughness_channels());
    channels.enable("pbr.base_color");
    const auto initial_cursor = channels.channel_revision_cursor("pbr.base_color");
    const std::array first{std::byte{1}, std::byte{2}, std::byte{3}};
    const std::array unrequested{std::byte{5}, std::byte{8}, std::byte{13}};
    const std::array edge{std::byte{21}, std::byte{34}, std::byte{55}};
    channels.pixels("pbr.base_color").write_pixel(1, 2, first);
    channels.pixels("pbr.base_color").write_pixel(64, 0, unrequested);
    channels.pixels("pbr.base_color").write_pixel(64, 64, edge);
    const auto delta = ctex::xport::query_channel_delta(channels, "pbr.base_color", initial_cursor);

    std::vector<std::byte> first_output(64 * 64 * 3, std::byte{0x7f});
    std::vector<std::byte> edge_output(6 * 6 * 3, std::byte{0x7f});
    const std::array destinations{
        TileReadbackDestination{version_at(delta.changed_tiles, {0, 0}),
                                ctex::xport::tile_memory_layout(channels, "pbr.base_color", {0, 0}),
                                first_output},
        TileReadbackDestination{version_at(delta.changed_tiles, {1, 1}),
                                ctex::xport::tile_memory_layout(channels, "pbr.base_color", {1, 1}),
                                edge_output},
    };
    TileReadback readback =
        TileReadback::begin_cpu(channels, "pbr.base_color", delta.current_cursor, destinations);

    const std::size_t first_offset = ((2 * 64) + 1) * 3;
    return expect(readback.status() == TileReadbackStatus::complete && readback.output_readable() &&
                      readback.tile_count() == 2,
                  "CPU tile readback did not complete through the asynchronous state model") &&
           expect(std::equal(first.begin(), first.end(), first_output.begin() + first_offset),
                  "CPU tile readback returned the wrong interior pixel") &&
           expect(std::equal(edge.begin(), edge.end(), edge_output.begin()),
                  "CPU tile readback returned the wrong edge-tile pixel");
}

bool stale_cpu_readback_does_not_publish() {
    TextureChannels channels(64, 64, 8, ctex::doc::metallic_roughness_channels());
    channels.enable("pbr.base_color");
    const auto initial_cursor = channels.channel_revision_cursor("pbr.base_color");
    const std::array first{std::byte{1}, std::byte{2}, std::byte{3}};
    channels.pixels("pbr.base_color").write_pixel(0, 0, first);
    const auto delta = ctex::xport::query_channel_delta(channels, "pbr.base_color", initial_cursor);
    const std::array second{std::byte{5}, std::byte{8}, std::byte{13}};
    channels.pixels("pbr.base_color").write_pixel(0, 0, second);

    std::vector<std::byte> output(64 * 64 * 3, std::byte{0x6d});
    const std::array destinations{
        TileReadbackDestination{delta.changed_tiles.front(),
                                ctex::xport::tile_memory_layout(channels, "pbr.base_color", {0, 0}),
                                output},
    };
    TileReadback readback =
        TileReadback::begin_cpu(channels, "pbr.base_color", delta.current_cursor, destinations);
    return expect(readback.status() == TileReadbackStatus::failed && !readback.output_readable() &&
                      std::all_of(output.begin(), output.end(),
                                  [](std::byte value) { return value == std::byte{0x6d}; }),
                  "stale CPU readback published bytes into the caller buffer");
}

TileVersion host_version(TileCoordinate coordinate, ctex::image::Generation generation) {
    return {
        .coordinate = coordinate,
        .revision = generation,
        .generation = generation,
        .residency = TileResidency::host_device,
    };
}

TileMemoryLayout host_layout(std::uint32_t width, std::uint32_t height) {
    return {
        .width = width,
        .height = height,
        .row_pitch_bytes = width,
        .pixel_stride_bytes = 1,
        .channel_order = ChannelOrder::r,
        .component_type = ctex::image::ChannelType::uint8_unorm,
        .component_byte_order = ComponentByteOrder::native,
        .tile_contiguity = TileContiguity::separate_buffers,
    };
}

bool host_completion_controls_buffer_visibility() {
    std::array<std::byte, 4> first_output{std::byte{0x7f}, std::byte{0x7f}, std::byte{0x7f},
                                          std::byte{0x7f}};
    std::array<std::byte, 2> second_output{std::byte{0x7f}, std::byte{0x7f}};
    const std::array destinations{
        TileReadbackDestination{host_version({2, 3}, 7), host_layout(2, 2), first_output},
        TileReadbackDestination{host_version({4, 5}, 11), host_layout(2, 1), second_output},
    };
    TileReadback readback = TileReadback::begin_host(destinations);
    bool passed = true;
    passed &= expect(readback.status() == TileReadbackStatus::pending &&
                         !readback.output_readable() && first_output.front() == std::byte{0x7f},
                     "pending host readback exposed output bytes");

    const std::array first_payload{std::byte{1}, std::byte{2}, std::byte{3}, std::byte{4}};
    const std::array second_payload{std::byte{5}, std::byte{6}};
    const std::array completed{
        HostTileCompletion{host_version({2, 3}, 7), host_layout(2, 2), first_payload},
        HostTileCompletion{host_version({4, 5}, 11), host_layout(2, 1), second_payload},
    };
    passed &= expect(
        readback.complete_host(completed) && readback.status() == TileReadbackStatus::complete &&
            readback.output_readable() &&
            std::equal(first_payload.begin(), first_payload.end(), first_output.begin()) &&
            std::equal(second_payload.begin(), second_payload.end(), second_output.begin()),
        "successful host completion did not publish the exact payload");
    return passed;
}

bool cancelled_failed_and_mismatched_completions_publish_nothing() {
    const TileVersion version = host_version({1, 1}, 3);
    const std::array payload{std::byte{1}, std::byte{2}, std::byte{3}};

    std::array<std::byte, 3> cancelled_output{std::byte{0x4a}, std::byte{0x4a}, std::byte{0x4a}};
    const std::array cancelled_destination{
        TileReadbackDestination{version, host_layout(3, 1), cancelled_output},
    };
    TileReadback cancelled = TileReadback::begin_host(cancelled_destination);
    const std::array completion{HostTileCompletion{version, host_layout(3, 1), payload}};
    bool passed = expect(cancelled.cancel() && !cancelled.complete_host(completion) &&
                             cancelled.status() == TileReadbackStatus::cancelled &&
                             cancelled_output.front() == std::byte{0x4a},
                         "late completion published a cancelled host readback");

    std::array<std::byte, 3> failed_output{std::byte{0x5b}, std::byte{0x5b}, std::byte{0x5b}};
    const std::array failed_destination{
        TileReadbackDestination{version, host_layout(3, 1), failed_output}};
    TileReadback failed = TileReadback::begin_host(failed_destination);
    passed &=
        expect(failed.fail_host("device lost") && failed.status() == TileReadbackStatus::failed &&
                   !failed.output_readable() && failed.detail() == "device lost" &&
                   failed_output.front() == std::byte{0x5b},
               "failed host readback changed its caller buffer");

    std::array<std::byte, 3> mismatch_output{std::byte{0x6c}, std::byte{0x6c}, std::byte{0x6c}};
    const std::array mismatch_destination{
        TileReadbackDestination{version, host_layout(3, 1), mismatch_output}};
    TileReadback mismatch = TileReadback::begin_host(mismatch_destination);
    const std::array wrong_completion{
        HostTileCompletion{host_version({9, 9}, 3), host_layout(3, 1), payload}};
    passed &= expect(!mismatch.complete_host(wrong_completion) &&
                         mismatch.status() == TileReadbackStatus::failed &&
                         mismatch_output.front() == std::byte{0x6c},
                     "mismatched host completion published output bytes");
    return passed;
}

}  // namespace

int main() {
    return cpu_readback_copies_only_named_tiles() && stale_cpu_readback_does_not_publish() &&
                   host_completion_controls_buffer_visibility() &&
                   cancelled_failed_and_mismatched_completions_publish_nothing()
               ? 0
               : 1;
}
