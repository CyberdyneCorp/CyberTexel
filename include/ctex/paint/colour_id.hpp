#ifndef CTEX_PAINT_COLOUR_ID_HPP
#define CTEX_PAINT_COLOUR_ID_HPP

#include <cstddef>
#include <cstdint>
#include <ctex/graph/document.hpp>
#include <ctex/paint/masking.hpp>
#include <span>
#include <vector>

namespace ctex::paint {

inline constexpr double maximum_colour_id_tolerance = 1.73205080756887729353;
inline constexpr ToolParameterDescriptor colour_id_tolerance_parameter{
    "colour_id.tolerance", 0.0, 0.0, maximum_colour_id_tolerance};

struct ColourIdMapView {
    std::uint32_t width{};
    std::uint32_t height{};
    std::span<const graph::ColourValue> pixels;
};

enum class ColourIdSelectionStatus : std::uint8_t { matched, empty };

struct ColourIdSelection {
    std::uint32_t width{};
    std::uint32_t height{};
    graph::ColourValue picked_colour{};
    double tolerance{};
    ToolParameterReport parameter_report;
    std::vector<double> values;
    std::size_t selected_texel_count{};
    ColourIdSelectionStatus status{ColourIdSelectionStatus::empty};

    [[nodiscard]] PaintMaskView as_paint_restriction() const& noexcept { return {values}; }
    [[nodiscard]] PaintMaskView as_mask_source() const& noexcept { return {values}; }
    [[nodiscard]] PaintMaskView as_visibility_filter() const& noexcept { return {values}; }
    PaintMaskView as_paint_restriction() const&& = delete;
    PaintMaskView as_mask_source() const&& = delete;
    PaintMaskView as_visibility_filter() const&& = delete;
};

[[nodiscard]] ColourIdSelection select_colour_id(ColourIdMapView map,
                                                 graph::ColourValue picked_colour,
                                                 double tolerance);

}  // namespace ctex::paint

#endif
