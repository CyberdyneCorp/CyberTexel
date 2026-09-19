#ifndef CTEX_PAINT_COVERAGE_DETAIL_HPP
#define CTEX_PAINT_COVERAGE_DETAIL_HPP

#include <cstdint>
#include <ctex/paint/coverage.hpp>

namespace ctex::paint::detail {

struct CoverageContribution {
    double value{};
    Vec3d reference_normal;
    std::uint64_t symmetry_instance{};
};

void validate_surface_raster(const TextureSpaceRaster& surface);

[[nodiscard]] CoverageContribution stamp_contribution(Vec3d point, const Stamp& stamp,
                                                      bool transformed_tip);
[[nodiscard]] CoverageContribution segment_contribution(Vec3d point, const Stamp& start,
                                                        const Stamp& end);

}  // namespace ctex::paint::detail

#endif
