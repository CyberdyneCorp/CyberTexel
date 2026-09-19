#ifndef CTEX_PAINT_CLONE_HPP
#define CTEX_PAINT_CLONE_HPP

#include <cstddef>
#include <cstdint>
#include <ctex/paint/brush.hpp>
#include <ctex/paint/surface_cache.hpp>
#include <limits>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace ctex::paint {

enum class CloneMode : std::uint8_t { aligned, fixed };

struct CloneSource {
    std::string texture_set_id;
    Vec2d uv;
};

class CloneSourceState {
public:
    void set_source(std::string texture_set_id, Vec2d uv);
    void clear_source() noexcept;

    [[nodiscard]] const std::optional<CloneSource>& source() const noexcept { return source_; }

private:
    std::optional<CloneSource> source_;
};

struct CloneSettings {
    CloneMode mode{CloneMode::aligned};
    Vec2d destination_anchor_uv;
    DepositionMode deposition_mode{DepositionMode::non_building};
    std::string_view blend_mode{"normal"};
    PaintMaskInputs masks;
};

inline constexpr std::size_t no_clone_sample = std::numeric_limits<std::size_t>::max();

struct CloneResult {
    std::uint32_t width{};
    std::uint32_t height{};
    CloneMode mode{CloneMode::aligned};
    Vec2d source_anchor_uv;
    Vec2d destination_anchor_uv;
    DepositionRaster deposition;
    std::vector<std::size_t> source_sample_indices;
    std::vector<PaintToolChannelRaster> channels;
    std::vector<std::string> applied_channel_ids;
};

[[nodiscard]] CloneResult apply_clone(
    const CachedSurfaceMaps& destination_surface, const ResolvedStroke& stroke,
    const RejectedCoverageRaster& rejected, const CloneSourceState& source_state,
    std::span<const PaintToolChannelRaster> enabled_layer_snapshot,
    std::span<const PaintToolChannelRaster> source_snapshot, const CloneSettings& settings = {});

}  // namespace ctex::paint

#endif
