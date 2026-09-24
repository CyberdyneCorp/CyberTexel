#include <algorithm>
#include <array>
#include <cstddef>
#include <ctex/doc/document.hpp>
#include <iostream>
#include <string_view>
#include <vector>

namespace {

using namespace ctex::doc;

bool expect(bool condition, std::string_view message) {
    if (!condition) {
        std::cerr << message << '\n';
    }
    return condition;
}

template <typename Exception, typename Callable>
bool expect_error(Callable&& callable, std::string_view message) {
    try {
        callable();
    } catch (const Exception&) {
        return true;
    } catch (...) {
    }
    return expect(false, message);
}

TextureSet texture_set() {
    TextureSet result({.display_name = "Material",
                       .partition_kind = PartitionSourceKind::material,
                       .partition_key = "material",
                       .uv_set = "uv0",
                       .width = 128,
                       .height = 64,
                       .default_bit_depth = 8});
    result.channels().enable("pbr.base_color");
    result.configure_tile_history_budget(result.channels().pixels("pbr.base_color").tile_bytes() *
                                         2);
    return result;
}

TileHistoryTarget target(std::uint32_t x) {
    return {.semantic_id = "pbr.base_color", .coordinate = {x, 0}};
}

LayerEntry group_entry(std::string identifier) {
    LayerEntry entry;
    entry.identifier = std::move(identifier);
    entry.display_name = "Group";
    entry.kind = LayerEntryKind::group;
    return entry;
}

LayerOperationRequest create_group(std::string identifier) {
    CreateLayerOperation operation;
    operation.entry = group_entry(std::move(identifier));
    LayerOperationRequest request;
    request.operation = std::move(operation);
    request.resolved_content.width = 128;
    request.resolved_content.height = 64;
    return request;
}

bool forty_writes_commit_as_one_step() {
    TextureSet set = texture_set();
    auto& live = set.channels().pixels("pbr.base_color");
    const ctex::image::RevisionCursor before_revision = live.revision_cursor();
    const std::array targets{target(0)};
    auto transaction = set.begin_transaction("gesture", targets);
    for (unsigned value = 1; value <= 40; ++value) {
        const auto component = static_cast<unsigned char>(value);
        const std::array pixel{std::byte{component}, std::byte{component}, std::byte{component}};
        transaction.write_pixel("pbr.base_color", 0, 0, pixel);
    }
    const bool isolated =
        expect(live.revision_cursor() == before_revision && !live.is_tile_allocated({0, 0}),
               "staged gesture changed the live channel before commit");
    const TileHistoryCommitResult committed = transaction.commit();
    const TileHistoryRestoreResult undone = set.undo_tiles();
    const bool undo_ok = expect(committed.committed && committed.tile_count == 1 &&
                                    set.tile_history_budget_report().redo_steps == 1 &&
                                    !live.is_tile_allocated({0, 0}) && undone.tile_count == 1,
                                "forty writes did not commit and undo as one tile-history step");
    const TileHistoryRestoreResult redone = set.redo_tiles();
    return isolated && undo_ok &&
           expect(std::to_integer<unsigned>(live.read_pixel(0, 0).front()) == 40 &&
                      redone.tile_count == 1,
                  "redo did not restore the final staged gesture value");
}

bool mixed_pixel_and_layer_transaction_is_one_step() {
    TextureSet set = texture_set();
    const std::array targets{target(0)};
    auto transaction = set.begin_transaction("mixed", targets);
    transaction.write_pixel("pbr.base_color", 1, 1,
                            std::array{std::byte{7}, std::byte{8}, std::byte{9}});
    static_cast<void>(transaction.apply_layer_operation(create_group("group")));
    const bool isolated =
        expect(set.layer_stack().empty() &&
                   !set.channels().pixels("pbr.base_color").is_tile_allocated({0, 0}),
               "mixed transaction published before commit");
    const TileHistoryCommitResult committed = transaction.commit();
    const bool published = expect(committed.layer_stack_changed && committed.tile_count == 1 &&
                                      set.layer_stack().contains("group"),
                                  "mixed transaction did not publish both kinds of state");
    const TileHistoryRestoreResult undone = set.undo_tiles();
    const bool undo_ok =
        expect(undone.layer_stack_exchanged && set.layer_stack().empty() &&
                   !set.channels().pixels("pbr.base_color").is_tile_allocated({0, 0}),
               "one undo did not restore pixels and layer structure");
    const TileHistoryRestoreResult redone = set.redo_tiles();
    return isolated && published && undo_ok &&
           expect(redone.layer_stack_exchanged && set.layer_stack().contains("group") &&
                      std::to_integer<unsigned>(
                          set.channels().pixels("pbr.base_color").read_pixel(1, 1).front()) == 7,
                  "one redo did not restore the complete mixed transaction");
}

bool cancellation_and_destruction_leave_live_state_exact() {
    TextureSet set = texture_set();
    auto& live = set.channels().pixels("pbr.base_color");
    const ctex::image::RevisionCursor revision = live.revision_cursor();
    const TileHistoryBudgetReport history = set.tile_history_budget_report();
    const std::array targets{target(0)};
    {
        auto transaction = set.begin_transaction("discard-on-destruction", targets);
        transaction.write_pixel("pbr.base_color", 0, 0,
                                std::array{std::byte{1}, std::byte{2}, std::byte{3}});
        static_cast<void>(transaction.apply_layer_operation(create_group("discarded")));
    }
    auto cancelled = set.begin_transaction("cancel", targets);
    cancelled.write_pixel("pbr.base_color", 0, 0,
                          std::array{std::byte{4}, std::byte{5}, std::byte{6}});
    cancelled.cancel();
    const bool inactive = expect_error<TextureSetTransactionError>(
        [&] {
            cancelled.write_pixel("pbr.base_color", 0, 0,
                                  std::array{std::byte{7}, std::byte{8}, std::byte{9}});
        },
        "cancelled transaction remained active");
    return inactive &&
           expect(live.revision_cursor() == revision && !live.is_tile_allocated({0, 0}) &&
                      set.layer_stack().empty() &&
                      set.tile_history_budget_report().retained_bytes == history.retained_bytes &&
                      set.tile_history_budget_report().undo_steps == history.undo_steps,
                  "cancelled or abandoned transaction changed live state or history");
}

bool stale_and_undeclared_transactions_are_refused() {
    TextureSet set = texture_set();
    const std::array targets{target(0)};
    auto transaction = set.begin_transaction("stale", targets);
    transaction.write_pixel("pbr.base_color", 0, 0,
                            std::array{std::byte{1}, std::byte{1}, std::byte{1}});
    set.channels()
        .pixels("pbr.base_color")
        .write_pixel(0, 0, std::array{std::byte{9}, std::byte{9}, std::byte{9}});
    const bool stale =
        expect_error<TileHistoryError>([&] { static_cast<void>(transaction.commit()); },
                                       "transaction committed over a live change made after open");
    auto bounded = set.begin_transaction("bounded", targets);
    const bool undeclared = expect_error<TextureSetTransactionError>(
        [&] {
            bounded.write_pixel("pbr.base_color", 64, 0,
                                std::array{std::byte{2}, std::byte{2}, std::byte{2}});
        },
        "transaction accepted an undeclared target tile");
    bounded.cancel();
    const bool pixel_stale_ok =
        expect(std::to_integer<unsigned>(
                   set.channels().pixels("pbr.base_color").read_pixel(0, 0).front()) == 9 &&
                   set.layer_stack().empty() && set.tile_history_budget_report().undo_steps == 0,
               "refused transaction published staged state");

    auto layer_stale = set.begin_transaction("layer-stale");
    static_cast<void>(layer_stale.apply_layer_operation(create_group("staged")));
    set.layer_stack().append(group_entry("external"));
    const bool layer_commit_refused = expect_error<TileHistoryError>(
        [&] { static_cast<void>(layer_stale.commit()); },
        "transaction overwrote a live layer-stack change made after open");

    TextureSet undo_set = texture_set();
    auto committed = undo_set.begin_transaction("undo-stale", targets);
    committed.write_pixel("pbr.base_color", 0, 0,
                          std::array{std::byte{3}, std::byte{3}, std::byte{3}});
    static_cast<void>(committed.apply_layer_operation(create_group("committed")));
    static_cast<void>(committed.commit());
    undo_set.layer_stack().append(group_entry("external"));
    const bool layer_undo_refused =
        expect_error<TileHistoryError>([&] { static_cast<void>(undo_set.undo_tiles()); },
                                       "undo overwrote a layer-stack change made outside history");
    const bool layer_stale_ok =
        expect(undo_set.layer_stack().contains("committed") &&
                   undo_set.layer_stack().contains("external") &&
                   std::to_integer<unsigned>(
                       undo_set.channels().pixels("pbr.base_color").read_pixel(0, 0).front()) == 3,
               "stale layer-stack undo partially restored transaction pixels");
    return stale && undeclared && pixel_stale_ok && layer_commit_refused && layer_undo_refused &&
           layer_stale_ok;
}

bool layer_only_transaction_uses_no_pixel_budget() {
    TextureSet set = texture_set();
    set.configure_tile_history_budget(0);
    auto unchanged = set.begin_transaction("unchanged");
    const TileHistoryCommitResult no_step = unchanged.commit();
    auto transaction = set.begin_transaction("layer-only");
    static_cast<void>(transaction.apply_layer_operation(create_group("group")));
    const TileHistoryCommitResult committed = transaction.commit();
    static_cast<void>(set.undo_tiles());
    return expect(!no_step.committed && no_step.tile_count == 0 && committed.committed &&
                      committed.layer_stack_changed && committed.retained_bytes == 0 &&
                      set.layer_stack().empty(),
                  "layer-only transaction consumed pixel budget or was not undoable");
}

bool metadata_edits_are_command_history() {
    TextureSet set = texture_set();
    set.layer_stack().append(group_entry("group"));
    set.configure_tile_history_budget(0);
    auto transaction = set.begin_transaction("rename");
    transaction.layer_stack().set_display_name("group", "Renamed group");
    const TileHistoryCommitResult committed = transaction.commit();
    const TileHistoryBudgetReport after_commit = set.tile_history_budget_report();
    const TileHistoryRestoreResult undone = set.undo_tiles();
    const bool undo_ok =
        expect(committed.committed && committed.layer_stack_changed &&
                   committed.retained_bytes == 0 && after_commit.retained_bytes == 0 &&
                   after_commit.undo_steps == 1 && undone.layer_stack_exchanged &&
                   set.layer_stack().entry("group").display_name == "Group",
               "renaming was not retained as a zero-pixel command history step");
    const TileHistoryRestoreResult redone = set.redo_tiles();
    return undo_ok && expect(redone.layer_stack_exchanged &&
                                 set.layer_stack().entry("group").display_name == "Renamed group" &&
                                 set.tile_history_budget_report().retained_bytes == 0,
                             "redo did not restore the zero-pixel metadata edit");
}

bool bulk_region_round_trip_and_refusals() {
    TextureSet set = texture_set();
    auto& image = set.channels().pixels("pbr.base_color");
    std::vector<std::byte> supplied(128 * 64 * 3);
    for (std::size_t index = 0; index < supplied.size(); ++index) {
        supplied[index] = static_cast<std::byte>(index % 251);
    }
    const std::array targets{target(0), target(1)};
    auto transaction = set.begin_transaction("import", targets);
    transaction.write_region("pbr.base_color", {0, 0, 128, 64}, supplied, 128 * 3);
    const bool isolated =
        expect(!image.is_tile_allocated({0, 0}), "bulk region changed live pixels before commit");
    const auto committed = transaction.commit();
    bool identical = committed.committed && committed.tile_count == 2;
    for (std::uint32_t y = 0; y < 64 && identical; ++y) {
        for (std::uint32_t x = 0; x < 128 && identical; ++x) {
            const auto actual = image.read_pixel(x, y);
            const auto expected = std::span(supplied).subspan((y * 128 + x) * 3, 3);
            identical = std::ranges::equal(actual, expected);
        }
    }
    auto unchanged = set.begin_transaction("unchanged-import", targets);
    unchanged.write_region("pbr.base_color", {0, 0, 128, 64}, supplied, 128 * 3);
    const bool no_change = !unchanged.commit().committed;
    const auto undone = set.undo_tiles();
    const bool undo_ok =
        expect(undone.exchanged_storage_count == 2 && undone.copied_pixel_bytes == 0 &&
                   !image.is_tile_allocated({0, 0}) && !image.is_tile_allocated({1, 0}),
               "bulk undo copied pixels or failed to restore storage");
    static_cast<void>(set.redo_tiles());
    auto partial = set.begin_transaction("partial", std::array{target(0)});
    const std::array replacement{std::byte{1}, std::byte{2}, std::byte{3}};
    partial.write_region("pbr.base_color", {63, 63, 1, 1}, replacement, 3);
    const bool missing_target = expect_error<TextureSetTransactionError>(
        [&] {
            partial.write_region("pbr.base_color", {63, 0, 2, 1}, std::span(supplied).first(6), 6);
        },
        "bulk write accepted an undeclared tile");
    const bool bad_size = expect_error<std::invalid_argument>(
        [&] { partial.write_region("pbr.base_color", {0, 0, 2, 1}, replacement, 6); },
        "bulk write accepted a byte-count mismatch");
    const bool bad_bounds = expect_error<std::out_of_range>(
        [&] { partial.write_region("pbr.base_color", {128, 0, 1, 1}, replacement, 3); },
        "bulk write accepted an out-of-bounds rectangle");
    const auto partial_commit = partial.commit();
    const bool partial_ok = partial_commit.tile_count == 1 &&
                            std::ranges::equal(image.read_pixel(63, 63), replacement) &&
                            std::ranges::equal(image.read_pixel(64, 63),
                                               std::span(supplied).subspan((63 * 128 + 64) * 3, 3));
    return isolated && no_change &&
           expect(identical, "bulk write did not round-trip byte-identically") && undo_ok &&
           expect(partial_ok, "partial region changed pixels outside its bounds") &&
           missing_target && bad_size && bad_bounds;
}

}  // namespace

int main() {
    return forty_writes_commit_as_one_step() && mixed_pixel_and_layer_transaction_is_one_step() &&
                   cancellation_and_destruction_leave_live_state_exact() &&
                   stale_and_undeclared_transactions_are_refused() &&
                   layer_only_transaction_uses_no_pixel_budget() &&
                   metadata_edits_are_command_history() && bulk_region_round_trip_and_refusals()
               ? 0
               : 1;
}
