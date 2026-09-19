#ifndef CTEX_PAINT_BLENDING_HPP
#define CTEX_PAINT_BLENDING_HPP

#include <cstdint>
#include <ctex/graph/document.hpp>
#include <ctex/paint/deposition.hpp>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace ctex::paint {

struct StrokeBlendRaster {
    std::uint32_t width{};
    std::uint32_t height{};
    std::vector<graph::ColourValue> pixels;
};

class StrokeSnapshotBlender {
public:
    StrokeSnapshotBlender(std::uint32_t width, std::uint32_t height,
                          std::span<const graph::ColourValue> stroke_start_snapshot,
                          std::string_view blend_mode);

    void shade(std::span<const graph::ColourValue> paint, std::span<const double> strength);
    void shade(std::span<const graph::ColourValue> paint, const DepositionRaster& deposition);

    [[nodiscard]] std::string_view blend_mode() const noexcept { return blend_mode_; }
    [[nodiscard]] std::span<const graph::ColourValue> stroke_start_snapshot() const noexcept {
        return snapshot_;
    }
    [[nodiscard]] const StrokeBlendRaster& result() const noexcept { return result_; }

private:
    std::string blend_mode_;
    std::vector<graph::ColourValue> snapshot_;
    StrokeBlendRaster result_;
};

[[nodiscard]] StrokeBlendRaster blend_stroke_snapshot(
    std::uint32_t width, std::uint32_t height,
    std::span<const graph::ColourValue> stroke_start_snapshot,
    std::span<const graph::ColourValue> paint, const DepositionRaster& deposition,
    std::string_view blend_mode);

}  // namespace ctex::paint

#endif
