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

inline constexpr std::uint32_t maximum_blur_smear_radius = 4'096;
inline constexpr ToolParameterDescriptor blur_radius_parameter{"blur.radius", 1.0, 1.0,
                                                               maximum_blur_smear_radius};
inline constexpr ToolParameterDescriptor smear_strength_parameter{"smear.strength", 0.5, 0.0, 1.0};
inline constexpr ToolParameterDescriptor smear_footprint_radius_x_parameter{
    "smear.footprint.radius_x", 1.0, 0.0, maximum_blur_smear_radius};
inline constexpr ToolParameterDescriptor smear_footprint_radius_y_parameter{
    "smear.footprint.radius_y", 1.0, 0.0, maximum_blur_smear_radius};

struct BlurNeighborhood {
    StrokeFrame output_frame;
    std::vector<SurfaceAdjacentSample> horizontal_samples;
    std::vector<SurfaceAdjacentSample> vertical_samples;
};

struct BlurSettings {
    std::uint32_t radius{static_cast<std::uint32_t>(blur_radius_parameter.default_value)};
    DepositionMode deposition_mode{DepositionMode::non_building};
    std::string_view blend_mode{"normal"};
    PaintMaskInputs masks;
    std::span<const BlurNeighborhood> neighborhoods;
};

struct BlurResult {
    std::uint32_t width{};
    std::uint32_t height{};
    ToolParameterReport parameter_report;
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
    double strength{smear_strength_parameter.default_value};
    SamplingFootprint footprint{
        static_cast<std::uint32_t>(smear_footprint_radius_x_parameter.default_value),
        static_cast<std::uint32_t>(smear_footprint_radius_y_parameter.default_value)};
    DepositionMode deposition_mode{DepositionMode::non_building};
    std::string_view blend_mode{"normal"};
    PaintMaskInputs masks;
    std::span<const SmearMapping> mappings;
};

struct SmearResult {
    std::uint32_t width{};
    std::uint32_t height{};
    ToolParameterReport parameter_report;
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
