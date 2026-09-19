#ifndef CTEX_PAINT_BRUSH_HPP
#define CTEX_PAINT_BRUSH_HPP

#include <cstdint>
#include <ctex/graph/document.hpp>
#include <ctex/paint/blending.hpp>
#include <ctex/paint/masking.hpp>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace ctex::paint {

struct PaintToolChannelRaster {
    std::string semantic_id;
    std::uint8_t component_count{};
    std::vector<graph::ColourValue> pixels;
};

struct BrushSettings {
    DepositionMode deposition_mode{DepositionMode::non_building};
    std::string_view blend_mode{"normal"};
    PaintMaskInputs masks;
};

struct BrushResult {
    std::uint32_t width{};
    std::uint32_t height{};
    DepositionRaster deposition;
    std::vector<PaintToolChannelRaster> channels;
    std::vector<std::string> applied_channel_ids;
};

[[nodiscard]] BrushResult apply_brush(
    const ResolvedStroke& stroke, const RejectedCoverageRaster& rejected,
    std::span<const PaintToolChannelRaster> enabled_layer_snapshot,
    std::span<const PaintToolChannelRaster> material, const BrushSettings& settings = {});

enum class EraserTarget : std::uint8_t { layer_opacity, mask };

struct EraserSettings {
    DepositionMode deposition_mode{DepositionMode::non_building};
    PaintMaskInputs masks;
};

struct EraserResult {
    std::uint32_t width{};
    std::uint32_t height{};
    EraserTarget target{EraserTarget::layer_opacity};
    DepositionRaster deposition;
    std::vector<double> values;
};

[[nodiscard]] EraserResult apply_eraser(const ResolvedStroke& stroke,
                                        const RejectedCoverageRaster& rejected,
                                        std::span<const double> stroke_start_values,
                                        EraserTarget target, const EraserSettings& settings = {});

}  // namespace ctex::paint

#endif
