#include <algorithm>
#include <ctex/doc/tile_history.hpp>
#include <limits>
#include <map>
#include <set>
#include <utility>

namespace ctex::doc {
namespace {

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
        result.reserved_bytes_ = checked_add(result.reserved_bytes_, image.tile_bytes());
        result.entries_.push_back({
            .target = target,
            .width = image.width(),
            .height = image.height(),
            .tile_size = image.tile_size(),
            .format = image.format(),
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
        .identifier = std::move(capture.step_identifier_), .retained_bytes = 0, .entries = {}};
    step.entries.reserve(capture.entries_.size());
    for (TileHistoryCapture::Entry& captured : capture.entries_) {
        image::TiledImage& image = channels.pixels(captured.target.semantic_id);
        validate_image(image, captured);
        const image::Generation generation = image.tile_generation(captured.target.coordinate);
        if (generation == captured.generation) {
            continue;
        }
        if (image.tile_revision(captured.target.coordinate) == 0) {
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
            .retained_bytes = retained_bytes()};
}

TileHistoryRestoreResult TileHistory::undo(TextureChannels& channels) {
    return restore(channels, undo_steps_, redo_steps_, undo_bytes_, redo_bytes_,
                   TileHistoryErrorCode::no_undo);
}

TileHistoryRestoreResult TileHistory::redo(TextureChannels& channels) {
    return restore(channels, redo_steps_, undo_steps_, redo_bytes_, undo_bytes_,
                   TileHistoryErrorCode::no_redo);
}

TileHistoryRestoreResult TileHistory::restore(TextureChannels& channels, std::vector<Step>& source,
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
    for (StepEntry& entry : step.entries) {
        image::TiledImage& image = channels.pixels(entry.captured.target.semantic_id);
        entry.captured.storage = image.exchange_tile_storage(entry.captured.target.coordinate,
                                                             std::move(entry.captured.storage));
        entry.expected_generation = image.tile_generation(entry.captured.target.coordinate);
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
            .copied_pixel_bytes = 0};
}

}  // namespace ctex::doc
