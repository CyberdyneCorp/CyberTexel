#include <array>
#include <cstddef>
#include <ctex/doc/document.hpp>
#include <iostream>
#include <string_view>
#include <vector>

namespace {

using namespace ctex::doc;
using ctex::image::TileCoordinate;

bool expect(bool condition, std::string_view message) {
    if (!condition) {
        std::cerr << message << '\n';
    }
    return condition;
}

template <typename Callable>
bool expect_error(Callable&& callable, TileHistoryErrorCode code, std::string_view message) {
    try {
        callable();
    } catch (const TileHistoryError& error) {
        return expect(error.code() == code, message);
    } catch (...) {
    }
    return expect(false, message);
}

TextureSet texture_set(std::uint32_t width = 128, std::uint32_t height = 64) {
    TextureSet result({.display_name = "Material",
                       .partition_kind = PartitionSourceKind::material,
                       .partition_key = "material",
                       .uv_set = "uv0",
                       .width = width,
                       .height = height,
                       .default_bit_depth = 8});
    result.channels().enable("pbr.base_color");
    return result;
}

TileHistoryTarget target(std::uint32_t x, std::uint32_t y = 0) {
    return {.semantic_id = "pbr.base_color", .coordinate = {x, y}};
}

bool undo_and_redo_exchange_exact_storage_owners() {
    TextureSet set = texture_set();
    auto& image = set.channels().pixels("pbr.base_color");
    const std::array before{std::byte{10}, std::byte{20}, std::byte{30}};
    const std::array after{std::byte{40}, std::byte{50}, std::byte{60}};
    image.write_pixel(0, 0, before);
    const void* before_identity = image.snapshot_tile_storage({0, 0}).identity();
    set.configure_tile_history_budget(image.tile_bytes());
    const std::array targets{target(0)};
    auto capture = set.begin_tile_history_step("stroke", targets);
    image.write_pixel(0, 0, after);
    const void* after_identity = image.snapshot_tile_storage({0, 0}).identity();
    const TileHistoryCommitResult committed = set.commit_tile_history_step(std::move(capture));
    const TileHistoryRestoreResult undone = set.undo_tiles();
    const bool undo_ok =
        expect(committed.committed && committed.tile_count == 1 &&
                   image.snapshot_tile_storage({0, 0}).identity() == before_identity &&
                   std::equal(before.begin(), before.end(), image.read_pixel(0, 0).begin()) &&
                   undone.exchanged_storage_count == 1 && undone.copied_pixel_bytes == 0,
               "undo did not exchange the exact pre-edit tile owner without copying");
    const TileHistoryRestoreResult redone = set.redo_tiles();
    return undo_ok &&
           expect(image.snapshot_tile_storage({0, 0}).identity() == after_identity &&
                      std::equal(after.begin(), after.end(), image.read_pixel(0, 0).begin()) &&
                      redone.exchanged_storage_count == 1 && redone.copied_pixel_bytes == 0 &&
                      set.tile_history_budget_report().undo_steps == 1 &&
                      set.tile_history_budget_report().redo_steps == 0,
                  "redo did not symmetrically exchange the post-edit tile owner");
}

bool commit_retains_only_declared_tiles_that_changed() {
    TextureSet set = texture_set();
    auto& image = set.channels().pixels("pbr.base_color");
    const std::size_t tile_bytes = image.tile_bytes();
    set.configure_tile_history_budget(tile_bytes * 2);
    const std::array targets{target(0), target(1)};
    auto capture = set.begin_tile_history_step("partial-stroke", targets);
    image.write_pixel(0, 0, std::array{std::byte{1}, std::byte{2}, std::byte{3}});
    const TileHistoryCommitResult committed = set.commit_tile_history_step(std::move(capture));
    const TileHistoryBudgetReport report = set.tile_history_budget_report(tile_bytes);
    return expect(committed.tile_count == 1 && committed.retained_bytes == tile_bytes &&
                      report.retained_bytes == tile_bytes && report.available_bytes == tile_bytes &&
                      report.additional_steps_at_proposed_size == 1,
                  "history retained unchanged declared tiles or misreported its budget");
}

bool large_canvas_history_is_proportional_to_dirty_tiles() {
    TextureSet set = texture_set(16'384, 16'384);
    auto& image = set.channels().pixels("pbr.base_color");
    set.configure_tile_history_budget(image.tile_bytes());
    const std::array targets{target(128, 128)};
    auto capture = set.begin_tile_history_step("small-16k-stroke", targets);
    image.write_pixel(8'192, 8'192, std::array{std::byte{9}, std::byte{8}, std::byte{7}});
    const TileHistoryCommitResult committed = set.commit_tile_history_step(std::move(capture));
    const std::size_t full_channel_bytes =
        static_cast<std::size_t>(image.width()) * image.height() * image.pixel_bytes();
    return expect(committed.tile_count == 1 && committed.retained_bytes == image.tile_bytes() &&
                      committed.retained_bytes < full_channel_bytes / 10'000,
                  "small 16K edit retained storage proportional to the whole channel");
}

bool ceiling_discards_the_oldest_step() {
    TextureSet set = texture_set();
    auto& image = set.channels().pixels("pbr.base_color");
    set.configure_tile_history_budget(image.tile_bytes());
    const std::array first_target{target(0)};
    auto first = set.begin_tile_history_step("first", first_target);
    image.write_pixel(0, 0, std::array{std::byte{1}, std::byte{1}, std::byte{1}});
    static_cast<void>(set.commit_tile_history_step(std::move(first)));
    const std::array second_target{target(1)};
    auto second = set.begin_tile_history_step("second", second_target);
    image.write_pixel(64, 0, std::array{std::byte{2}, std::byte{2}, std::byte{2}});
    const TileHistoryCommitResult committed = set.commit_tile_history_step(std::move(second));
    const TileHistoryRestoreResult undone = set.undo_tiles();
    return expect(committed.committed && committed.step_identifier == "second" &&
                      committed.retained_bytes == image.tile_bytes() &&
                      set.tile_history_budget_report().undo_steps == 0 &&
                      set.tile_history_budget_report().redo_steps == 1 &&
                      undone.step_identifier == "second" && !image.is_tile_allocated({1, 0}) &&
                      image.is_tile_allocated({0, 0}),
                  "history ceiling did not discard only the oldest step");
}

bool budget_and_capture_refusals_happen_before_edits() {
    TextureSet set = texture_set();
    auto& image = set.channels().pixels("pbr.base_color");
    set.configure_tile_history_budget(image.tile_bytes() - 1);
    const std::array one_target{target(0)};
    const bool budget = expect_error(
        [&] { static_cast<void>(set.begin_tile_history_step("too-large", one_target)); },
        TileHistoryErrorCode::over_budget,
        "history admitted a step larger than its declared budget");
    set.configure_tile_history_budget(image.tile_bytes());
    const std::array duplicates{target(0), target(0)};
    const bool duplicate = expect_error(
        [&] { static_cast<void>(set.begin_tile_history_step("duplicates", duplicates)); },
        TileHistoryErrorCode::invalid_capture,
        "history accepted the same channel tile twice in one step");
    return budget && duplicate &&
           expect(image.tile_generation({0, 0}) == 0 &&
                      set.tile_history_budget_report().undo_steps == 0,
                  "capture refusal changed pixels or history state");
}

bool unchanged_capture_and_new_edit_manage_redo_explicitly() {
    TextureSet set = texture_set();
    auto& image = set.channels().pixels("pbr.base_color");
    set.configure_tile_history_budget(image.tile_bytes());
    const std::array targets{target(0)};
    auto unchanged = set.begin_tile_history_step("unchanged", targets);
    const TileHistoryCommitResult no_step = set.commit_tile_history_step(std::move(unchanged));
    auto first = set.begin_tile_history_step("first", targets);
    image.write_pixel(0, 0, std::array{std::byte{1}, std::byte{1}, std::byte{1}});
    static_cast<void>(set.commit_tile_history_step(std::move(first)));
    static_cast<void>(set.undo_tiles());
    auto replacement = set.begin_tile_history_step("replacement", targets);
    image.write_pixel(0, 0, std::array{std::byte{2}, std::byte{2}, std::byte{2}});
    static_cast<void>(set.commit_tile_history_step(std::move(replacement)));
    return expect(!no_step.committed && no_step.tile_count == 0,
                  "unchanged capture created a history step") &&
           expect_error([&] { static_cast<void>(set.redo_tiles()); }, TileHistoryErrorCode::no_redo,
                        "new committed edit did not invalidate redo history");
}

bool external_tile_change_and_budget_shrink_are_refused() {
    TextureSet set = texture_set();
    auto& image = set.channels().pixels("pbr.base_color");
    set.configure_tile_history_budget(image.tile_bytes());
    const std::array targets{target(0)};
    auto capture = set.begin_tile_history_step("tracked", targets);
    image.write_pixel(0, 0, std::array{std::byte{3}, std::byte{3}, std::byte{3}});
    static_cast<void>(set.commit_tile_history_step(std::move(capture)));
    const bool shrink = expect_error(
        [&] { set.configure_tile_history_budget(image.tile_bytes() - 1); },
        TileHistoryErrorCode::over_budget, "history budget shrank below already retained storage");
    image.write_pixel(0, 0, std::array{std::byte{4}, std::byte{4}, std::byte{4}});
    const bool stale = expect_error([&] { static_cast<void>(set.undo_tiles()); },
                                    TileHistoryErrorCode::stale_state,
                                    "undo overwrote a tile changed outside history");
    return shrink && stale &&
           expect(std::to_integer<unsigned>(image.read_pixel(0, 0).front()) == 4,
                  "stale undo changed the externally edited tile");
}

bool stale_step_is_rejected_before_any_exchange() {
    TextureSet set = texture_set();
    auto& image = set.channels().pixels("pbr.base_color");
    set.configure_tile_history_budget(image.tile_bytes() * 2);
    const std::array targets{target(0), target(1)};
    auto capture = set.begin_tile_history_step("two-tiles", targets);
    image.write_pixel(0, 0, std::array{std::byte{5}, std::byte{5}, std::byte{5}});
    image.write_pixel(64, 0, std::array{std::byte{6}, std::byte{6}, std::byte{6}});
    static_cast<void>(set.commit_tile_history_step(std::move(capture)));
    image.write_pixel(64, 0, std::array{std::byte{7}, std::byte{7}, std::byte{7}});
    const bool stale =
        expect_error([&] { static_cast<void>(set.undo_tiles()); },
                     TileHistoryErrorCode::stale_state, "multi-tile stale undo was not refused");
    return stale && expect(std::to_integer<unsigned>(image.read_pixel(0, 0).front()) == 5 &&
                               std::to_integer<unsigned>(image.read_pixel(64, 0).front()) == 7,
                           "stale undo partially restored an earlier target");
}

bool empty_undo_and_redo_are_typed() {
    TextureSet set = texture_set();
    return expect_error([&] { static_cast<void>(set.undo_tiles()); }, TileHistoryErrorCode::no_undo,
                        "empty history undo was not distinguished") &&
           expect_error([&] { static_cast<void>(set.redo_tiles()); }, TileHistoryErrorCode::no_redo,
                        "empty history redo was not distinguished");
}

}  // namespace

int main() {
    return undo_and_redo_exchange_exact_storage_owners() &&
                   commit_retains_only_declared_tiles_that_changed() &&
                   large_canvas_history_is_proportional_to_dirty_tiles() &&
                   ceiling_discards_the_oldest_step() &&
                   budget_and_capture_refusals_happen_before_edits() &&
                   unchanged_capture_and_new_edit_manage_redo_explicitly() &&
                   external_tile_change_and_budget_shrink_are_refused() &&
                   stale_step_is_rejected_before_any_exchange() && empty_undo_and_redo_are_typed()
               ? 0
               : 1;
}
