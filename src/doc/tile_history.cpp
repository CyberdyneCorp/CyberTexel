#include <algorithm>
#include <ctex/doc/tile_history.hpp>
#include <limits>
#include <map>
#include <set>
#include <type_traits>
#include <utility>

namespace ctex::doc {
namespace {

static_assert(std::is_nothrow_move_assignable_v<LayerStack>);
static_assert(std::is_nothrow_swappable_v<LayerStack>);

[[noreturn]] void fail(TileHistoryErrorCode code, std::string message) {
    throw TileHistoryError(code, std::move(message));
}

std::size_t checked_add(std::size_t left, std::size_t right) {
    if (right > std::numeric_limits<std::size_t>::max() - left) {
        fail(TileHistoryErrorCode::over_budget,
             "tile history retained-byte calculation overflowed");
    }
    return left + right;
}

}  // namespace

TileHistoryError::TileHistoryError(TileHistoryErrorCode code, std::string message)
    : std::invalid_argument(std::move(message)), code_(code) {}

void TileHistory::validate_image(const image::TiledImage& image,
                                 const TileHistoryCapture::Entry& captured) {
    if (image.width() != captured.width || image.height() != captured.height ||
        image.tile_size() != captured.tile_size || image.format() != captured.format) {
        fail(TileHistoryErrorCode::stale_state,
             "tile history target image layout changed after capture");
    }
}

std::optional<TileHistory::LayerStackCommand> TileHistory::make_layer_command(
    const LayerStack& from, const LayerStack& to, std::uint64_t expected_revision) {
    std::map<std::string_view, const LayerEntry*, std::less<>> from_entries;
    std::map<std::string_view, const LayerEntry*, std::less<>> to_entries;
    for (const LayerEntry& entry : from.entries()) {
        from_entries.emplace(entry.identifier, &entry);
    }
    for (const LayerEntry& entry : to.entries()) {
        to_entries.emplace(entry.identifier, &entry);
    }

    LayerStackCommand command;
    command.expected_revision = expected_revision;
    for (const LayerEntry& entry : to.entries()) {
        const auto found = from_entries.find(entry.identifier);
        if (found == from_entries.end() || *found->second != entry) {
            command.replacement_entries.push_back(entry);
        }
    }
    for (const LayerEntry& entry : from.entries()) {
        if (!to_entries.contains(entry.identifier)) {
            command.removed_identifiers.push_back(entry.identifier);
        }
    }
    const bool same_order =
        from.size() == to.size() &&
        std::equal(from.entries().begin(), from.entries().end(), to.entries().begin(),
                   to.entries().end(), [](const LayerEntry& left, const LayerEntry& right) {
                       return left.identifier == right.identifier;
                   });
    if (!same_order) {
        command.order.emplace();
        command.order->reserve(to.size());
        for (const LayerEntry& entry : to.entries()) {
            command.order->push_back(entry.identifier);
        }
    }
    if (command.replacement_entries.empty() && command.removed_identifiers.empty() &&
        !command.order.has_value()) {
        return std::nullopt;
    }
    return command;
}

LayerStack TileHistory::apply_layer_command(const LayerStack& current,
                                            const LayerStackCommand& command) {
    std::vector<LayerEntry> entries(current.entries().begin(), current.entries().end());
    std::erase_if(entries, [&](const LayerEntry& entry) {
        return std::ranges::find(command.removed_identifiers, entry.identifier) !=
               command.removed_identifiers.end();
    });
    for (const LayerEntry& replacement : command.replacement_entries) {
        const auto found =
            std::ranges::find(entries, replacement.identifier, &LayerEntry::identifier);
        if (found == entries.end()) {
            entries.push_back(replacement);
        } else {
            *found = replacement;
        }
    }
    if (command.order.has_value()) {
        std::map<std::string, LayerEntry, std::less<>> by_identifier;
        for (LayerEntry& entry : entries) {
            by_identifier.emplace(entry.identifier, std::move(entry));
        }
        entries.clear();
        entries.reserve(command.order->size());
        for (const std::string& identifier : *command.order) {
            auto node = by_identifier.extract(identifier);
            if (node.empty()) {
                throw std::logic_error("layer history command order references a missing entry");
            }
            entries.push_back(std::move(node.mapped()));
        }
        if (!by_identifier.empty()) {
            throw std::logic_error("layer history command order omitted an entry");
        }
    }
    LayerStack result = current;
    result.assign(std::move(entries));
    return result;
}

void TileHistory::configure_budget(std::size_t budget_bytes) {
    if (budget_bytes < retained_bytes()) {
        fail(TileHistoryErrorCode::over_budget,
             "tile history budget cannot be reduced below retained history bytes");
    }
    budget_bytes_ = budget_bytes;
}

TileHistoryBudgetReport TileHistory::budget_report(std::size_t proposed_step_bytes) const noexcept {
    const std::size_t retained = retained_bytes();
    const std::size_t available = budget_bytes_ >= retained ? budget_bytes_ - retained : 0;
    return {
        .budget_bytes = budget_bytes_,
        .retained_bytes = retained,
        .available_bytes = available,
        .proposed_step_bytes = proposed_step_bytes,
        .additional_steps_at_proposed_size =
            proposed_step_bytes == 0 ? 0 : available / proposed_step_bytes,
        .undo_steps = undo_steps_.size(),
        .redo_steps = redo_steps_.size(),
    };
}

TileHistoryCapture TileHistory::begin_step(const TextureChannels& channels,
                                           std::string step_identifier,
                                           std::span<const TileHistoryTarget> targets) const {
    if (step_identifier.empty() || targets.empty()) {
        fail(TileHistoryErrorCode::invalid_capture,
             "tile history capture requires a step identity and at least one target");
    }
    TileHistoryCapture result;
    result.step_identifier_ = std::move(step_identifier);
    result.owner_ = this;
    result.history_sequence_ = sequence_;
    result.entries_.reserve(targets.size());
    std::set<std::pair<std::string, std::uint64_t>> unique;
    for (const TileHistoryTarget& target : targets) {
        if (target.semantic_id.empty()) {
            fail(TileHistoryErrorCode::invalid_capture,
                 "tile history target requires a channel semantic identity");
        }
        const std::uint64_t coordinate_key =
            (static_cast<std::uint64_t>(target.coordinate.y) << 32U) | target.coordinate.x;
        if (!unique.emplace(target.semantic_id, coordinate_key).second) {
            fail(TileHistoryErrorCode::invalid_capture,
                 "tile history targets must be unique within a step");
        }
        const image::TiledImage& image = channels.pixels(target.semantic_id);
        static_cast<void>(image.tile_extent(target.coordinate));
        const image::RevisionCursor cursor = image.revision_cursor();
        result.reserved_bytes_ = checked_add(result.reserved_bytes_, image.tile_bytes());
        result.entries_.push_back({
            .target = target,
            .width = image.width(),
            .height = image.height(),
            .tile_size = image.tile_size(),
            .format = image.format(),
            .revision_epoch = cursor.epoch,
            .revision = image.tile_revision(target.coordinate),
            .generation = image.tile_generation(target.coordinate),
            .storage = image.snapshot_tile_storage(target.coordinate),
            .reserved_bytes = image.tile_bytes(),
        });
    }
    if (result.reserved_bytes_ > budget_bytes_) {
        fail(TileHistoryErrorCode::over_budget,
             "tile history step requests " + std::to_string(result.reserved_bytes_) +
                 " bytes but the declared ceiling is " + std::to_string(budget_bytes_));
    }
    return result;
}

TileHistoryCapture TileHistory::begin_transaction(
    const TextureChannels& channels, const LayerStack& layer_stack, std::string step_identifier,
    std::span<const TileHistoryTarget> targets) const {
    TileHistoryCapture result;
    if (targets.empty()) {
        if (step_identifier.empty()) {
            fail(TileHistoryErrorCode::invalid_capture,
                 "texture-set transaction requires a step identity");
        }
        result.step_identifier_ = std::move(step_identifier);
        result.owner_ = this;
        result.history_sequence_ = sequence_;
    } else {
        result = begin_step(channels, std::move(step_identifier), targets);
    }
    result.layer_stack_ = layer_stack;
    result.layer_stack_revision_ = layer_stack.revision();
    return result;
}

TileHistoryCommitResult TileHistory::commit_step(TextureChannels& channels,
                                                 TileHistoryCapture capture) {
    if (capture.step_identifier_.empty() || capture.owner_ != this ||
        capture.history_sequence_ != sequence_) {
        fail(TileHistoryErrorCode::stale_state,
             "tile history capture is empty, already consumed, or stale");
    }
    if (sequence_ == std::numeric_limits<std::uint64_t>::max()) {
        fail(TileHistoryErrorCode::stale_state, "tile history sequence space is exhausted");
    }
    Step step{
        .identifier = std::move(capture.step_identifier_),
        .retained_bytes = 0,
        .entries = {},
        .layer_command = std::nullopt,
    };
    step.entries.reserve(capture.entries_.size());
    for (TileHistoryCapture::Entry& captured : capture.entries_) {
        image::TiledImage& image = channels.pixels(captured.target.semantic_id);
        validate_image(image, captured);
        const image::Generation generation = image.tile_generation(captured.target.coordinate);
        if (generation == captured.generation) {
            continue;
        }
        if (image.revision_cursor().epoch != captured.revision_epoch ||
            image.tile_revision(captured.target.coordinate) == 0) {
            fail(TileHistoryErrorCode::stale_state,
                 "tile history cannot commit an unpublished or reset tile generation");
        }
        step.retained_bytes = checked_add(step.retained_bytes, captured.reserved_bytes);
        step.entries.push_back(
            {.captured = std::move(captured), .expected_generation = generation});
    }
    if (step.entries.empty()) {
        return {.committed = false,
                .step_identifier = std::move(step.identifier),
                .tile_count = 0,
                .retained_bytes = retained_bytes()};
    }
    if (step.retained_bytes > budget_bytes_) {
        fail(TileHistoryErrorCode::over_budget,
             "tile history step requests " + std::to_string(step.retained_bytes) +
                 " bytes but the declared ceiling is " + std::to_string(budget_bytes_));
    }
    undo_steps_.reserve(undo_steps_.size() + 1);
    redo_steps_.clear();
    redo_bytes_ = 0;
    while (undo_bytes_ > budget_bytes_ - step.retained_bytes) {
        undo_bytes_ -= undo_steps_.front().retained_bytes;
        undo_steps_.erase(undo_steps_.begin());
    }
    undo_bytes_ += step.retained_bytes;
    const std::string identifier = step.identifier;
    const std::size_t tile_count = step.entries.size();
    undo_steps_.push_back(std::move(step));
    ++sequence_;
    return {.committed = true,
            .step_identifier = identifier,
            .tile_count = tile_count,
            .retained_bytes = retained_bytes(),
            .layer_stack_changed = false};
}

TileHistoryCommitResult TileHistory::commit_transaction(TextureChannels& channels,
                                                        LayerStack& layer_stack,
                                                        const TextureChannels& staged_channels,
                                                        LayerStack staged_layer_stack,
                                                        TileHistoryCapture capture) {
    if (capture.step_identifier_.empty() || capture.owner_ != this ||
        capture.history_sequence_ != sequence_ || !capture.layer_stack_.has_value() ||
        !capture.layer_stack_revision_.has_value()) {
        fail(TileHistoryErrorCode::stale_state,
             "texture-set transaction is empty, already consumed, or stale");
    }
    if (sequence_ == std::numeric_limits<std::uint64_t>::max()) {
        fail(TileHistoryErrorCode::stale_state, "tile history sequence space is exhausted");
    }

    std::optional<LayerStackCommand> layer_command = make_layer_command(
        staged_layer_stack, *capture.layer_stack_, staged_layer_stack.revision());
    const bool layer_changed = layer_command.has_value();
    if (layer_stack.revision() != *capture.layer_stack_revision_) {
        fail(TileHistoryErrorCode::stale_state,
             "live layer stack changed after the transaction opened");
    }
    Step step{
        .identifier = std::move(capture.step_identifier_),
        .retained_bytes = 0,
        .entries = {},
        .layer_command = std::move(layer_command),
    };
    step.entries.reserve(capture.entries_.size());
    std::vector<image::TileStorageSnapshot> replacements;
    replacements.reserve(capture.entries_.size());
    std::map<std::string, std::size_t, std::less<>> exchange_counts;
    std::map<std::string, std::size_t, std::less<>> new_allocation_counts;

    for (TileHistoryCapture::Entry& captured : capture.entries_) {
        image::TiledImage& live = channels.pixels(captured.target.semantic_id);
        const image::TiledImage& staged = staged_channels.pixels(captured.target.semantic_id);
        validate_image(live, captured);
        validate_image(staged, captured);
        if (live.revision_cursor().epoch != captured.revision_epoch ||
            live.tile_revision(captured.target.coordinate) != captured.revision ||
            live.tile_generation(captured.target.coordinate) != captured.generation) {
            fail(TileHistoryErrorCode::stale_state,
                 "live transaction target changed after the transaction opened");
        }
        const image::Generation staged_generation =
            staged.tile_generation(captured.target.coordinate);
        if (staged_generation == captured.generation) {
            if (staged.revision_cursor().epoch != captured.revision_epoch ||
                staged.tile_revision(captured.target.coordinate) != captured.revision) {
                fail(TileHistoryErrorCode::stale_state,
                     "staged transaction target reset its revision history");
            }
            continue;
        }
        if (staged.revision_cursor().epoch != captured.revision_epoch ||
            staged.tile_revision(captured.target.coordinate) == 0 ||
            captured.generation == std::numeric_limits<image::Generation>::max()) {
            fail(TileHistoryErrorCode::stale_state,
                 "staged transaction target has an invalid generation");
        }
        step.retained_bytes = checked_add(step.retained_bytes, captured.reserved_bytes);
        replacements.push_back(staged.snapshot_tile_storage(captured.target.coordinate));
        if (!replacements.back().empty() && !live.is_tile_allocated(captured.target.coordinate)) {
            ++new_allocation_counts[captured.target.semantic_id];
        }
        ++exchange_counts[captured.target.semantic_id];
        const image::Generation expected_generation =
            live.tile_generation(captured.target.coordinate) + 1;
        step.entries.push_back(
            {.captured = std::move(captured), .expected_generation = expected_generation});
    }

    if (step.entries.empty() && !layer_changed) {
        return {.committed = false,
                .step_identifier = std::move(step.identifier),
                .tile_count = 0,
                .retained_bytes = retained_bytes(),
                .layer_stack_changed = false};
    }
    if (step.retained_bytes > budget_bytes_) {
        fail(TileHistoryErrorCode::over_budget,
             "texture-set transaction requests " + std::to_string(step.retained_bytes) +
                 " history bytes but the declared ceiling is " + std::to_string(budget_bytes_));
    }
    for (const auto& [semantic_id, count] : exchange_counts) {
        const image::Revision revision = channels.pixels(semantic_id).revision();
        if (count > std::numeric_limits<image::Revision>::max() - revision) {
            fail(TileHistoryErrorCode::stale_state,
                 "texture-set transaction would exhaust image revision space");
        }
    }
    undo_steps_.reserve(undo_steps_.size() + 1);
    for (const auto& [semantic_id, count] : new_allocation_counts) {
        channels.pixels(semantic_id).prepare_tile_storage_exchanges(count);
    }
    const std::string identifier = step.identifier;
    const std::size_t tile_count = step.entries.size();

    for (std::size_t index = 0; index < step.entries.size(); ++index) {
        const TileHistoryTarget& target = step.entries[index].captured.target;
        static_cast<void>(
            channels.pixels(target.semantic_id)
                .exchange_tile_storage(target.coordinate, std::move(replacements[index])));
    }
    if (layer_changed) {
        layer_stack = std::move(staged_layer_stack);
    }
    redo_steps_.clear();
    redo_bytes_ = 0;
    while (undo_bytes_ > budget_bytes_ - step.retained_bytes) {
        undo_bytes_ -= undo_steps_.front().retained_bytes;
        undo_steps_.erase(undo_steps_.begin());
    }
    undo_bytes_ += step.retained_bytes;
    undo_steps_.push_back(std::move(step));
    ++sequence_;
    return {.committed = true,
            .step_identifier = identifier,
            .tile_count = tile_count,
            .retained_bytes = retained_bytes(),
            .layer_stack_changed = layer_changed};
}

TileHistoryRestoreResult TileHistory::undo(TextureChannels& channels, LayerStack& layer_stack) {
    return restore(channels, layer_stack, undo_steps_, redo_steps_, undo_bytes_, redo_bytes_,
                   TileHistoryErrorCode::no_undo);
}

TileHistoryRestoreResult TileHistory::redo(TextureChannels& channels, LayerStack& layer_stack) {
    return restore(channels, layer_stack, redo_steps_, undo_steps_, redo_bytes_, undo_bytes_,
                   TileHistoryErrorCode::no_redo);
}

TileHistoryRestoreResult TileHistory::restore(TextureChannels& channels, LayerStack& layer_stack,
                                              std::vector<Step>& source,
                                              std::vector<Step>& destination,
                                              std::size_t& source_bytes,
                                              std::size_t& destination_bytes,
                                              TileHistoryErrorCode empty_code) {
    if (source.empty()) {
        fail(empty_code, empty_code == TileHistoryErrorCode::no_undo
                             ? "tile history has no undo step"
                             : "tile history has no redo step");
    }
    if (sequence_ == std::numeric_limits<std::uint64_t>::max()) {
        fail(TileHistoryErrorCode::stale_state, "tile history sequence space is exhausted");
    }
    Step& step = source.back();
    if (step.layer_command.has_value() &&
        layer_stack.revision() != step.layer_command->expected_revision) {
        fail(TileHistoryErrorCode::stale_state,
             "live layer stack changed outside the next undo or redo step");
    }
    std::optional<LayerStack> restored_layer_stack;
    std::optional<LayerStackCommand> reverse_layer_command;
    if (step.layer_command.has_value()) {
        restored_layer_stack = apply_layer_command(layer_stack, *step.layer_command);
        reverse_layer_command = make_layer_command(*restored_layer_stack, layer_stack,
                                                   restored_layer_stack->revision());
        if (!reverse_layer_command.has_value()) {
            throw std::logic_error("layer history command did not change the layer stack");
        }
    }
    std::map<std::string, std::size_t, std::less<>> exchange_counts;
    std::map<std::string, std::size_t, std::less<>> new_allocation_counts;
    for (const StepEntry& entry : step.entries) {
        const image::TiledImage& image = channels.pixels(entry.captured.target.semantic_id);
        validate_image(image, entry.captured);
        if (image.tile_generation(entry.captured.target.coordinate) != entry.expected_generation ||
            image.tile_revision(entry.captured.target.coordinate) == 0 ||
            image.tile_generation(entry.captured.target.coordinate) ==
                std::numeric_limits<image::Generation>::max()) {
            fail(TileHistoryErrorCode::stale_state,
                 "tile history target changed outside the next undo or redo step");
        }
        ++exchange_counts[entry.captured.target.semantic_id];
        if (!entry.captured.storage.empty() &&
            !image.is_tile_allocated(entry.captured.target.coordinate)) {
            ++new_allocation_counts[entry.captured.target.semantic_id];
        }
    }
    for (const auto& [semantic_id, count] : exchange_counts) {
        const image::RevisionCursor cursor = channels.pixels(semantic_id).revision_cursor();
        if (count > std::numeric_limits<image::Revision>::max() - cursor.revision) {
            fail(TileHistoryErrorCode::stale_state,
                 "tile history restore would exhaust image revision space");
        }
    }
    destination.reserve(destination.size() + 1);
    for (const auto& [semantic_id, count] : new_allocation_counts) {
        channels.pixels(semantic_id).prepare_tile_storage_exchanges(count);
    }
    const std::string identifier = step.identifier;
    const std::size_t tile_count = step.entries.size();
    const bool layer_stack_exchanged = step.layer_command.has_value();
    for (StepEntry& entry : step.entries) {
        image::TiledImage& image = channels.pixels(entry.captured.target.semantic_id);
        entry.captured.storage = image.exchange_tile_storage(entry.captured.target.coordinate,
                                                             std::move(entry.captured.storage));
        entry.expected_generation = image.tile_generation(entry.captured.target.coordinate);
    }
    if (restored_layer_stack.has_value()) {
        layer_stack = std::move(*restored_layer_stack);
        step.layer_command = std::move(reverse_layer_command);
    }
    const std::size_t bytes = step.retained_bytes;
    destination.push_back(std::move(step));
    source.pop_back();
    source_bytes -= bytes;
    destination_bytes += bytes;
    ++sequence_;
    return {.step_identifier = identifier,
            .tile_count = tile_count,
            .exchanged_storage_count = tile_count,
            .copied_pixel_bytes = 0,
            .layer_stack_exchanged = layer_stack_exchanged};
}

}  // namespace ctex::doc
