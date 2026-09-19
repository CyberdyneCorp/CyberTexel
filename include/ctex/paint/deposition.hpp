#ifndef CTEX_PAINT_DEPOSITION_HPP
#define CTEX_PAINT_DEPOSITION_HPP

#include <cstddef>
#include <cstdint>
#include <ctex/paint/rejection.hpp>
#include <span>
#include <vector>

namespace ctex::paint {

enum class DepositionMode : std::uint8_t { non_building, build_up };

struct DepositionRaster {
    std::uint32_t width{};
    std::uint32_t height{};
    DepositionMode mode{DepositionMode::non_building};
    std::vector<double> non_building_coverage;
    std::vector<double> build_up_deposition;
    std::vector<double> strength;
    std::size_t applied_stamp_count{};
};

class StrokeDepositionAccumulator {
public:
    StrokeDepositionAccumulator(const ResolvedStroke& stroke, std::uint32_t width,
                                std::uint32_t height,
                                DepositionMode mode = DepositionMode::non_building);

    void apply(std::span<const RejectedStampCoverage> stamp_events);

    [[nodiscard]] const DepositionRaster& result() const noexcept { return result_; }
    [[nodiscard]] std::uint64_t next_stamp_ordinal() const noexcept { return next_stamp_ordinal_; }

private:
    ResolvedStroke stroke_;
    DepositionRaster result_;
    std::uint64_t next_stamp_ordinal_{};
};

[[nodiscard]] DepositionRaster evaluate_deposition(
    const ResolvedStroke& stroke, const RejectedCoverageRaster& rejected,
    DepositionMode mode = DepositionMode::non_building);

}  // namespace ctex::paint

#endif
