#ifndef CTEX_PAINT_MASKING_HPP
#define CTEX_PAINT_MASKING_HPP

#include <cstddef>
#include <cstdint>
#include <ctex/paint/rejection.hpp>
#include <optional>
#include <span>
#include <vector>

namespace ctex::paint {

struct PaintMaskView {
    std::span<const double> values;
};

struct PaintMaskInputs {
    std::span<const PaintMaskView> active_layer_masks;
    std::optional<PaintMaskView> colour_id_selection;
    std::optional<PaintMaskView> geometry_selection;
    std::optional<PaintMaskView> screen_selection;
    std::optional<PaintMaskView> uv_island_selection;
};

struct CombinedPaintMask {
    std::uint32_t width{};
    std::uint32_t height{};
    std::vector<double> values;
    std::size_t active_input_count{};
};

[[nodiscard]] CombinedPaintMask combine_paint_masks(std::uint32_t width, std::uint32_t height,
                                                    const PaintMaskInputs& inputs = {});

[[nodiscard]] RejectedCoverageRaster apply_paint_masks(const RejectedCoverageRaster& rejected,
                                                       const PaintMaskInputs& inputs = {});

}  // namespace ctex::paint

#endif
