#include <array>
#include <cmath>
#include <ctex/paint/seam_dilation.hpp>
#include <iostream>
#include <limits>
#include <string_view>
#include <vector>

namespace {

using namespace ctex::paint;

bool expect(bool condition, std::string_view message) {
    if (!condition) {
        std::cerr << message << '\n';
    }
    return condition;
}

SeamDilationRaster scalar_raster(std::initializer_list<double> pixels) {
    return {.width = static_cast<std::uint32_t>(pixels.size()),
            .height = 1,
            .component_count = 1,
            .pixels = pixels};
}

bool extrapolation_preserves_a_gradient() {
    const SeamDilationRaster source = scalar_raster({-20.0, -20.0, 2.0, 3.0, 4.0, -20.0});
    const std::array<std::uint8_t, 6> coverage{0, 0, 1, 1, 1, 0};
    const SeamDilationResult result = dilate_uv_seams(source, coverage);
    return expect(result.raster.pixels == std::vector<double>({0.0, 1.0, 2.0, 3.0, 4.0, 5.0}),
                  "seam dilation repeated an edge value instead of continuing its gradient") &&
           expect(result.dilated_texel_count == 3 && result.zero_gradient_texel_count == 0,
                  "gradient dilation report did not count its extrapolated texels");
}

bool every_component_is_extrapolated_without_clamping() {
    const SeamDilationRaster source{
        .width = 3,
        .height = 1,
        .component_count = 4,
        .pixels = {-9.0, -9.0, -9.0, -9.0, 1.0, 2.0, 3.0, 4.0, 2.0, 4.0, 6.0, 8.0}};
    const std::array<std::uint8_t, 3> coverage{0, 1, 1};
    const auto result = dilate_uv_seams(source, coverage, 1);
    return expect(result.raster.pixels == std::vector<double>({0.0, 0.0, 0.0, 0.0, 1.0, 2.0, 3.0,
                                                               4.0, 2.0, 4.0, 6.0, 8.0}),
                  "seam dilation did not extrapolate every channel independently");
}

bool zero_radius_disables_dilation() {
    const SeamDilationRaster source = scalar_raster({-1.0, 2.0, 3.0, -1.0});
    const std::array<std::uint8_t, 4> coverage{0, 1, 1, 0};
    const auto result = dilate_uv_seams(source, coverage, 0);
    return expect(result.raster.pixels == source.pixels && result.dilated_texel_count == 0 &&
                      result.zero_gradient_texel_count == 0,
                  "a zero dilation radius changed the raster");
}

bool a_thin_island_uses_reported_zero_gradient_extrapolation() {
    const SeamDilationRaster source = scalar_raster({-1.0, 7.0, -1.0});
    const std::array<std::uint8_t, 3> coverage{0, 1, 0};
    const auto result = dilate_uv_seams(source, coverage, 1);
    return expect(result.raster.pixels == std::vector<double>({7.0, 7.0, 7.0}) &&
                      result.dilated_texel_count == 2 && result.zero_gradient_texel_count == 2,
                  "thin-island fallback was not a reported zero-gradient extrapolation");
}

bool long_stroke_defers_all_tiles_until_finish() {
    DeferredStrokeDilation stroke;
    const std::array<std::uint8_t, 5> coverage{0, 1, 1, 1, 0};
    stroke.stage_tile({.u = 0, .v = 0}, scalar_raster({-1.0, 1.0, 2.0, 3.0, -1.0}), coverage);
    const StrokeDilationOutput first_frame = stroke.provisional_preview();
    stroke.stage_tile({.u = 1, .v = 0}, scalar_raster({-1.0, 4.0, 5.0, 6.0, -1.0}), coverage);
    stroke.stage_tile({.u = 0, .v = 0}, scalar_raster({-1.0, 10.0, 11.0, 12.0, -1.0}), coverage);
    const StrokeDilationOutput later_frame = stroke.provisional_preview();
    const StrokeDilationOutput& final = stroke.finish();
    const StrokeDilationOutput* first_finish = &final;
    const StrokeDilationOutput* second_finish = &stroke.finish();
    return expect(stroke.radius() == default_seam_dilation_radius,
                  "stroke dilation did not use the documented default radius") &&
           expect(first_frame.state == DilationPreviewState::provisional &&
                      first_frame.dilation_pass_count == 0 && first_frame.tiles.size() == 1 &&
                      first_frame.tiles[0].raster.pixels.front() == -1.0,
                  "interactive preview was not explicitly provisional and undilated") &&
           expect(later_frame.state == DilationPreviewState::provisional &&
                      later_frame.tiles.size() == 2 &&
                      later_frame.tiles[0].raster.pixels[1] == 10.0,
                  "later stroke frames did not retain every dirtied tile's latest pixels") &&
           expect(final.state == DilationPreviewState::final && final.tiles.size() == 2 &&
                      final.dilation_pass_count == 2 &&
                      final.tiles[0].raster.pixels.front() == 9.0 &&
                      final.tiles[0].raster.pixels.back() == 13.0 &&
                      final.tiles[1].raster.pixels.front() == 3.0 &&
                      final.tiles[1].raster.pixels.back() == 7.0,
                  "stroke finish did not dilate every dirtied tile exactly once") &&
           expect(first_finish == second_finish && stroke.finished() &&
                      stroke.finish().dilation_pass_count == 2,
                  "repeated finalization reran deferred dilation");
}

bool staging_after_finish_and_invalid_inputs_are_transactional() {
    DeferredStrokeDilation stroke;
    const SeamDilationRaster valid = scalar_raster({-1.0, 1.0, 2.0, -1.0});
    const std::array<std::uint8_t, 4> coverage{0, 1, 1, 0};
    stroke.stage_tile({}, valid, coverage);
    bool invalid_refused = false;
    try {
        const std::array<std::uint8_t, 4> invalid_coverage{0, 1, 2, 0};
        stroke.stage_tile({}, valid, invalid_coverage);
    } catch (const std::invalid_argument&) {
        invalid_refused = true;
    }
    const auto preview = stroke.provisional_preview();
    static_cast<void>(stroke.finish());
    bool late_refused = false;
    try {
        stroke.stage_tile({.u = 1, .v = 0}, valid, coverage);
    } catch (const std::logic_error&) {
        late_refused = true;
    }
    bool non_finite_refused = false;
    try {
        static_cast<void>(dilate_uv_seams({.width = 1,
                                           .height = 1,
                                           .component_count = 1,
                                           .pixels = {std::numeric_limits<double>::quiet_NaN()}},
                                          std::array<std::uint8_t, 1>{1}));
    } catch (const std::invalid_argument&) {
        non_finite_refused = true;
    }
    return expect(invalid_refused && late_refused && non_finite_refused,
                  "invalid or late seam-dilation input was accepted") &&
           expect(preview.tiles.size() == 1 && preview.tiles[0].raster.pixels == valid.pixels,
                  "invalid replacement partially changed the staged tile");
}

bool deferred_zero_radius_is_final_without_a_pass() {
    DeferredStrokeDilation stroke(0);
    const SeamDilationRaster source = scalar_raster({-1.0, 2.0, -1.0});
    const std::array<std::uint8_t, 3> coverage{0, 1, 0};
    stroke.stage_tile({}, source, coverage);
    const auto& final = stroke.finish();
    return expect(final.state == DilationPreviewState::final && final.dilation_pass_count == 0 &&
                      final.tiles[0].raster.pixels == source.pixels,
                  "disabled deferred dilation ran a pass or changed pixels");
}

}  // namespace

int main() {
    return extrapolation_preserves_a_gradient() &&
                   every_component_is_extrapolated_without_clamping() &&
                   zero_radius_disables_dilation() &&
                   a_thin_island_uses_reported_zero_gradient_extrapolation() &&
                   long_stroke_defers_all_tiles_until_finish() &&
                   staging_after_finish_and_invalid_inputs_are_transactional() &&
                   deferred_zero_radius_is_final_without_a_pass()
               ? 0
               : 1;
}
