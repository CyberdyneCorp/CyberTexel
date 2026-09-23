#ifndef CTEX_DOC_RESOLUTION_CHANGE_HPP
#define CTEX_DOC_RESOLUTION_CHANGE_HPP

#include <cstddef>
#include <cstdint>
#include <ctex/doc/document.hpp>
#include <ctex/image/resampling.hpp>
#include <span>
#include <stdexcept>
#include <string>
#include <vector>

namespace ctex::doc {

inline constexpr std::size_t default_resolution_change_byte_limit = 1ULL << 30;

enum class ResolutionChangePolicy : std::uint8_t { replay_eligible, resample_all, cancel };

enum class ResolutionReplayDisposition : std::uint8_t {
    resolution_independent,
    checkpoint_only,
    resample_required,
    unsupported_algorithm,
};

enum class CheckpointResamplePolicy : std::uint8_t { refuse, nearest, bilinear };

struct ResolutionReplaySource {
    std::string identifier;
    ResolutionReplayDisposition disposition{ResolutionReplayDisposition::checkpoint_only};
    bool checkpoint_available{};
};

// A replay raster is the complete, host-evaluated result for one enabled
// channel. udim_tile_number == 0 addresses the texture-set's base channel.
struct ResolutionReplayRaster {
    std::string semantic_id;
    std::uint32_t udim_tile_number{};
    std::span<const std::byte> pixels;
};

struct ResolutionChangeRequest {
    std::uint32_t width{};
    std::uint32_t height{};
    ResolutionChangePolicy policy{ResolutionChangePolicy::cancel};
    CheckpointResamplePolicy checkpoint_policy{CheckpointResamplePolicy::refuse};
    std::span<const ResolutionReplaySource> replay_sources;
    std::span<const ResolutionReplayRaster> replay_rasters;
    std::size_t maximum_working_bytes{default_resolution_change_byte_limit};
    std::size_t maximum_history_bytes{default_resolution_change_byte_limit};
};

struct ResolutionChangeReport {
    bool committed{};
    ResolutionChangePolicy policy{ResolutionChangePolicy::cancel};
    std::uint32_t source_width{};
    std::uint32_t source_height{};
    std::uint32_t target_width{};
    std::uint32_t target_height{};
    std::size_t replayed_source_count{};
    std::size_t resampled_source_count{};
    std::size_t procedural_entry_count{};
    std::size_t raster_count{};
    std::size_t staged_pixel_bytes{};
    std::size_t retained_history_bytes{};
};

struct ResolutionRestoreReport {
    std::uint32_t width{};
    std::uint32_t height{};
    std::size_t retained_history_bytes{};
};

enum class ResolutionChangeErrorCode : std::uint8_t {
    invalid_request,
    over_budget,
    replay_unavailable,
    missing_replay_output,
    no_undo,
    no_redo,
};

class ResolutionChangeError final : public std::runtime_error {
public:
    ResolutionChangeError(ResolutionChangeErrorCode code, std::string message);
    [[nodiscard]] ResolutionChangeErrorCode code() const noexcept { return code_; }

private:
    ResolutionChangeErrorCode code_;
};

[[nodiscard]] ResolutionChangeReport change_texture_set_resolution(
    TextureSet& texture_set, const ResolutionChangeRequest& request);
[[nodiscard]] ResolutionRestoreReport undo_texture_set_resolution(TextureSet& texture_set);
[[nodiscard]] ResolutionRestoreReport redo_texture_set_resolution(TextureSet& texture_set);

}  // namespace ctex::doc

#endif
