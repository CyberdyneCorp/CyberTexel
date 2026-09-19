#ifndef CTEX_PAINT_BLUR_SMEAR_HPP
#define CTEX_PAINT_BLUR_SMEAR_HPP

#include <cstddef>
#include <cstdint>
#include <ctex/paint/brush.hpp>
#include <ctex/paint/seam_filter.hpp>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace ctex::paint {

struct BlurNeighborhood {
    StrokeFrame output_frame;
    std::vector<SurfaceAdjacentSample> horizontal_samples;
    std::vector<SurfaceAdjacentSample> vertical_samples;
};

struct BlurSettings {
    std::uint32_t radius{1};
    DepositionMode deposition_mode{DepositionMode::non_building};
    std::string_view blend_mode{"normal"};
    PaintMaskInputs masks;
    std::span<const BlurNeighborhood> neighborhoods;
};

struct BlurResult {
    std::uint32_t width{};
    std::uint32_t height{};
    SamplingFootprint footprint;
    DepositionRaster deposition;
    std::vector<PaintToolChannelRaster> filtered_snapshot;
    std::vector<PaintToolChannelRaster> channels;
    std::vector<std::string> applied_channel_ids;
};

[[nodiscard]] BlurResult apply_blur(
    const ResolvedStroke& stroke, const RejectedCoverageRaster& rejected,
    std::span<const PaintToolChannelRaster> enabled_layer_stroke_start_snapshot,
    const BlurSettings& settings);

struct SmearMapping {
    StrokeFrame output_frame;
    SurfaceAdjacentSample upstream_sample;
};

struct SmearSettings {
    double strength{0.5};
    SamplingFootprint footprint{1, 1};
    DepositionMode deposition_mode{DepositionMode::non_building};
    std::string_view blend_mode{"normal"};
    PaintMaskInputs masks;
    std::span<const SmearMapping> mappings;
};

struct SmearResult {
    std::uint32_t width{};
    std::uint32_t height{};
    SamplingFootprint footprint;
    double strength{};
    DepositionRaster deposition;
    std::vector<double> effective_strength;
    std::vector<PaintToolChannelRaster> dragged_snapshot;
    std::vector<PaintToolChannelRaster> channels;
    std::vector<std::string> applied_channel_ids;
};

[[nodiscard]] SmearResult apply_smear(
    const ResolvedStroke& stroke, const RejectedCoverageRaster& rejected,
    std::span<const PaintToolChannelRaster> enabled_layer_stroke_start_snapshot,
    const SmearSettings& settings);

}  // namespace ctex::paint

#endif
