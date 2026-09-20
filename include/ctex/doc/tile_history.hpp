#ifndef CTEX_DOC_TILE_HISTORY_HPP
#define CTEX_DOC_TILE_HISTORY_HPP

#include <cstddef>
#include <cstdint>
#include <ctex/doc/channels.hpp>
#include <ctex/image/tiled_image.hpp>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace ctex::doc {

class TileHistory;

struct TileHistoryTarget {
    std::string semantic_id;
    image::TileCoordinate coordinate{};
    friend bool operator==(const TileHistoryTarget&, const TileHistoryTarget&) = default;
};

struct TileHistoryBudgetReport {
    std::size_t budget_bytes{};
    std::size_t retained_bytes{};
    std::size_t available_bytes{};
    std::size_t proposed_step_bytes{};
    std::size_t additional_steps_at_proposed_size{};
    std::size_t undo_steps{};
    std::size_t redo_steps{};
};

struct TileHistoryCommitResult {
    bool committed{};
    std::string step_identifier;
    std::size_t tile_count{};
    std::size_t retained_bytes{};
};

struct TileHistoryRestoreResult {
    std::string step_identifier;
    std::size_t tile_count{};
    std::size_t exchanged_storage_count{};
    std::size_t copied_pixel_bytes{};
};

enum class TileHistoryErrorCode : std::uint8_t {
    invalid_capture,
    over_budget,
    no_undo,
    no_redo,
    stale_state,
};

class TileHistoryError final : public std::invalid_argument {
public:
    TileHistoryError(TileHistoryErrorCode code, std::string message);
    [[nodiscard]] TileHistoryErrorCode code() const noexcept { return code_; }

private:
    TileHistoryErrorCode code_;
};

class TileHistoryCapture {
public:
    TileHistoryCapture() = default;
    TileHistoryCapture(const TileHistoryCapture&) = delete;
    TileHistoryCapture& operator=(const TileHistoryCapture&) = delete;
    TileHistoryCapture(TileHistoryCapture&&) noexcept = default;
    TileHistoryCapture& operator=(TileHistoryCapture&&) noexcept = default;

    [[nodiscard]] std::string_view step_identifier() const noexcept { return step_identifier_; }
    [[nodiscard]] std::size_t declared_tile_count() const noexcept { return entries_.size(); }
    [[nodiscard]] std::size_t reserved_bytes() const noexcept { return reserved_bytes_; }

private:
    friend class TileHistory;
    struct Entry {
        TileHistoryTarget target;
        std::uint32_t width{};
        std::uint32_t height{};
        std::uint32_t tile_size{};
        image::PixelFormat format{};
        image::Generation generation{};
        image::TileStorageSnapshot storage;
        std::size_t reserved_bytes{};
    };

    std::string step_identifier_;
    const TileHistory* owner_{};
    std::uint64_t history_sequence_{};
    std::size_t reserved_bytes_{};
    std::vector<Entry> entries_;
};

class TileHistory {
public:
    explicit TileHistory(std::size_t budget_bytes = 0) : budget_bytes_(budget_bytes) {}

    void configure_budget(std::size_t budget_bytes);
    [[nodiscard]] TileHistoryBudgetReport budget_report(
        std::size_t proposed_step_bytes = 0) const noexcept;
    [[nodiscard]] TileHistoryCapture begin_step(const TextureChannels& channels,
                                                std::string step_identifier,
                                                std::span<const TileHistoryTarget> targets) const;
    [[nodiscard]] TileHistoryCommitResult commit_step(TextureChannels& channels,
                                                      TileHistoryCapture capture);
    [[nodiscard]] TileHistoryRestoreResult undo(TextureChannels& channels);
    [[nodiscard]] TileHistoryRestoreResult redo(TextureChannels& channels);

    [[nodiscard]] std::size_t undo_step_count() const noexcept { return undo_steps_.size(); }
    [[nodiscard]] std::size_t redo_step_count() const noexcept { return redo_steps_.size(); }
    [[nodiscard]] std::size_t retained_bytes() const noexcept { return undo_bytes_ + redo_bytes_; }

private:
    struct StepEntry {
        TileHistoryCapture::Entry captured;
        image::Generation expected_generation{};
    };
    struct Step {
        std::string identifier;
        std::size_t retained_bytes{};
        std::vector<StepEntry> entries;
    };

    [[nodiscard]] TileHistoryRestoreResult restore(
        TextureChannels& channels, std::vector<Step>& source, std::vector<Step>& destination,
        std::size_t& source_bytes, std::size_t& destination_bytes, TileHistoryErrorCode empty_code);
    static void validate_image(const image::TiledImage& image,
                               const TileHistoryCapture::Entry& captured);

    std::size_t budget_bytes_{};
    std::size_t undo_bytes_{};
    std::size_t redo_bytes_{};
    std::uint64_t sequence_{};
    std::vector<Step> undo_steps_;
    std::vector<Step> redo_steps_;
};

}  // namespace ctex::doc

#endif
