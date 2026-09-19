#include <algorithm>
#include <array>
#include <cstddef>
#include <ctex/xport/readback.hpp>
#include <ctex/xport/snapshot.hpp>
#include <iostream>
#include <string_view>
#include <vector>

namespace {

using ctex::doc::TextureChannels;
using ctex::xport::SnapshotPool;
using ctex::xport::SnapshotQueryStatus;
using ctex::xport::TileReadback;
using ctex::xport::TileReadbackDestination;
using ctex::xport::TileReadbackStatus;

bool expect(bool condition, std::string_view message) {
    if (!condition) {
        std::cerr << message << '\n';
    }
    return condition;
}

bool edits_after_query_do_not_tear_the_snapshot() {
    TextureChannels channels(64, 64, 8, ctex::doc::metallic_roughness_channels());
    channels.enable("pbr.base_color");
    const auto synchronized = channels.channel_revision_cursor("pbr.base_color");
    const std::array revision_one{std::byte{1}, std::byte{2}, std::byte{3}};
    const std::array revision_two{std::byte{5}, std::byte{8}, std::byte{13}};
    channels.pixels("pbr.base_color").write_pixel(0, 0, revision_one);
    const std::size_t tile_bytes = channels.pixels("pbr.base_color").tile_bytes();
    SnapshotPool pool(tile_bytes * 2);
    auto first_query =
        ctex::xport::query_channel_delta(pool, channels, "pbr.base_color", synchronized);
    if (!expect(first_query.admitted(), "revision-one snapshot was not admitted")) {
        return false;
    }
    auto& first = *first_query.synchronized;
    const auto revision_one_cursor = first.delta.current_cursor;

    channels.pixels("pbr.base_color").write_pixel(0, 0, revision_two);
    const auto layout = ctex::xport::tile_memory_layout(channels, "pbr.base_color", {0, 0});
    std::vector<std::byte> first_output(layout.byte_size(), std::byte{0x7f});
    const std::array first_destinations{
        TileReadbackDestination{first.delta.changed_tiles.front(), layout, first_output},
    };
    TileReadback first_readback = TileReadback::begin_cpu(first.snapshot, first_destinations);

    auto second_query =
        ctex::xport::query_channel_delta(pool, channels, "pbr.base_color", revision_one_cursor);
    if (!expect(second_query.admitted(), "revision-two snapshot was not admitted")) {
        return false;
    }
    auto& second = *second_query.synchronized;
    std::vector<std::byte> second_output(layout.byte_size(), std::byte{0x7f});
    const std::array second_destinations{
        TileReadbackDestination{second.delta.changed_tiles.front(), layout, second_output},
    };
    TileReadback second_readback = TileReadback::begin_cpu(second.snapshot, second_destinations);

    bool passed = true;
    passed &= expect(first_readback.status() == TileReadbackStatus::complete &&
                         std::equal(revision_one.begin(), revision_one.end(), first_output.begin()),
                     "revision-one snapshot was torn by the later edit");
    passed &=
        expect(second_readback.status() == TileReadbackStatus::complete &&
                   std::equal(revision_two.begin(), revision_two.end(), second_output.begin()),
               "the following delta did not contain revision two");
    passed &= expect(pool.memory_report().pinned_bytes == tile_bytes * 2 &&
                         pool.memory_report().active_snapshots == 2,
                     "snapshot memory report omitted pinned versions");

    first.snapshot.release();
    passed &= expect(!first.snapshot.active() && pool.memory_report().pinned_bytes == tile_bytes &&
                         pool.memory_report().active_snapshots == 1,
                     "explicit snapshot release did not reclaim revision one");
    second.snapshot.release();
    passed &=
        expect(pool.memory_report().pinned_bytes == 0 && pool.memory_report().active_snapshots == 0,
               "all released snapshots remained accounted as pinned");
    return passed;
}

bool budget_counts_unique_versions_and_refuses_before_exceeding() {
    TextureChannels channels(64, 64, 8, ctex::doc::metallic_roughness_channels());
    channels.enable("pbr.base_color");
    const auto initial = channels.channel_revision_cursor("pbr.base_color");
    const std::array revision_one{std::byte{1}, std::byte{1}, std::byte{1}};
    const std::array revision_two{std::byte{2}, std::byte{2}, std::byte{2}};
    const std::array revision_three{std::byte{3}, std::byte{3}, std::byte{3}};
    channels.pixels("pbr.base_color").write_pixel(0, 0, revision_one);
    const std::size_t tile_bytes = channels.pixels("pbr.base_color").tile_bytes();
    SnapshotPool pool(tile_bytes * 2);

    auto first = ctex::xport::query_channel_delta(pool, channels, "pbr.base_color", initial);
    auto duplicate = ctex::xport::query_channel_delta(pool, channels, "pbr.base_color", initial);
    if (!expect(first.admitted() && duplicate.admitted() &&
                    duplicate.additional_pinned_bytes == 0 &&
                    pool.memory_report().pinned_bytes == tile_bytes &&
                    pool.memory_report().pinned_allocations == 1,
                "shared snapshot allocation was counted more than once")) {
        return false;
    }
    const auto revision_one_cursor = first.synchronized->delta.current_cursor;
    duplicate.synchronized->snapshot.release();

    channels.pixels("pbr.base_color").write_pixel(0, 0, revision_two);
    auto second =
        ctex::xport::query_channel_delta(pool, channels, "pbr.base_color", revision_one_cursor);
    if (!expect(second.admitted(), "revision-two budget snapshot was not admitted")) {
        return false;
    }
    const auto revision_two_cursor = second.synchronized->delta.current_cursor;
    channels.pixels("pbr.base_color").write_pixel(0, 0, revision_three);
    auto refused =
        ctex::xport::query_channel_delta(pool, channels, "pbr.base_color", revision_two_cursor);
    bool passed = true;
    passed &= expect(second.admitted() && refused.status == SnapshotQueryStatus::over_budget &&
                         !refused.synchronized.has_value() &&
                         refused.additional_pinned_bytes == tile_bytes &&
                         pool.memory_report().pinned_bytes == tile_bytes * 2 &&
                         pool.memory_report().active_snapshots == 2,
                     "snapshot admission crossed or hid the configured ceiling");

    first.synchronized->snapshot.release();
    auto retry =
        ctex::xport::query_channel_delta(pool, channels, "pbr.base_color", revision_two_cursor);
    if (!expect(retry.admitted() && pool.memory_report().pinned_bytes == tile_bytes * 2,
                "released snapshot capacity was not reusable")) {
        return false;
    }
    second.synchronized->snapshot.release();
    retry.synchronized->snapshot.release();
    return passed &&
           expect(pool.memory_report().pinned_bytes == 0, "budget test leaked pinned allocations");
}

bool release_invalidates_readback_and_destruction_is_safe() {
    TextureChannels channels(64, 64, 8, ctex::doc::metallic_roughness_channels());
    channels.enable("pbr.base_color");
    const auto initial = channels.channel_revision_cursor("pbr.base_color");
    const std::array pixel{std::byte{7}, std::byte{8}, std::byte{9}};
    channels.pixels("pbr.base_color").write_pixel(0, 0, pixel);
    SnapshotPool pool(channels.pixels("pbr.base_color").tile_bytes());
    {
        auto query = ctex::xport::query_channel_delta(pool, channels, "pbr.base_color", initial);
        if (!expect(query.admitted(), "release test snapshot was not admitted")) {
            return false;
        }
        const auto version = query.synchronized->delta.changed_tiles.front();
        query.synchronized->snapshot.release();
        const auto layout = ctex::xport::tile_memory_layout(channels, "pbr.base_color", {0, 0});
        std::vector<std::byte> output(layout.byte_size(), std::byte{0x6d});
        const std::array destinations{TileReadbackDestination{version, layout, output}};
        TileReadback readback = TileReadback::begin_cpu(query.synchronized->snapshot, destinations);
        if (!expect(readback.status() == TileReadbackStatus::failed &&
                        output.front() == std::byte{0x6d},
                    "released snapshot allowed pixel publication")) {
            return false;
        }
    }
    {
        auto implicit = ctex::xport::query_channel_delta(pool, channels, "pbr.base_color", initial);
        if (!expect(implicit.admitted(), "destructor test snapshot was not admitted")) {
            return false;
        }
    }
    return expect(
        pool.memory_report().pinned_bytes == 0 && pool.memory_report().active_snapshots == 0,
        "snapshot destruction left pool accounting behind");
}

bool unchanged_query_fits_a_zero_byte_budget() {
    TextureChannels channels(16384, 16384, 8, ctex::doc::metallic_roughness_channels());
    channels.enable("pbr.base_color");
    const auto cursor = channels.channel_revision_cursor("pbr.base_color");
    SnapshotPool pool(0);
    auto query = ctex::xport::query_channel_delta(pool, channels, "pbr.base_color", cursor);
    const bool passed =
        expect(query.admitted() && query.synchronized->delta.changed_tiles.empty() &&
                   query.synchronized->snapshot.retained_bytes() == 0 &&
                   pool.memory_report().pinned_bytes == 0,
               "unchanged query consumed or required snapshot bytes");
    if (query.admitted()) {
        query.synchronized->snapshot.release();
    }
    return passed;
}

}  // namespace

int main() {
    return edits_after_query_do_not_tear_the_snapshot() &&
                   budget_counts_unique_versions_and_refuses_before_exceeding() &&
                   release_invalidates_readback_and_destruction_is_safe() &&
                   unchanged_query_fits_a_zero_byte_budget()
               ? 0
               : 1;
}
