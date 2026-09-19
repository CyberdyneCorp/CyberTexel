#include <array>
#include <ctex/paint/colour_id.hpp>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string_view>
#include <vector>

namespace {

using ctex::graph::ColourValue;
using namespace ctex::paint;

bool expect(bool condition, std::string_view message) {
    if (!condition) {
        std::cerr << message << '\n';
    }
    return condition;
}

bool tolerance_selects_only_matching_ids() {
    const std::array pixels{
        ColourValue{1.0F, 0.0F, 0.0F, 1.0F}, ColourValue{0.98F, 0.01F, 0.0F, 0.25F},
        ColourValue{0.0F, 1.0F, 0.0F, 1.0F}, ColourValue{0.95F, 0.05F, 0.0F, 1.0F}};
    const ColourIdSelection result = select_colour_id({.width = 4, .height = 1, .pixels = pixels},
                                                      ColourValue{1.0F, 0.0F, 0.0F, 0.0F}, 0.03);
    return expect(result.values == std::vector<double>({1.0, 1.0, 0.0, 0.0}) &&
                      result.selected_texel_count == 2 &&
                      result.status == ColourIdSelectionStatus::matched,
                  "colour-ID tolerance did not select the expected RGB region");
}

bool zero_tolerance_reports_filtered_map_as_empty() {
    const std::array pixels{ColourValue{0.99F, 0.01F, 0.0F, 1.0F},
                            ColourValue{0.97F, 0.03F, 0.0F, 1.0F}};
    const ColourIdSelection result = select_colour_id({.width = 2, .height = 1, .pixels = pixels},
                                                      ColourValue{1.0F, 0.0F, 0.0F, 1.0F}, 0.0);
    return expect(result.values == std::vector<double>({0.0, 0.0}) &&
                      result.selected_texel_count == 0 &&
                      result.status == ColourIdSelectionStatus::empty && result.tolerance == 0.0,
                  "zero tolerance silently widened instead of reporting an empty selection");
}

RejectedCoverageRaster rejected() {
    return {.coverage = {.width = 3, .height = 1, .values = {1.0, 1.0, 1.0}},
            .stamp_events = {{.stamp_ordinal = 0, .values = {1.0, 1.0, 1.0}}},
            .report = {}};
}

bool result_serves_every_selection_role() {
    const std::array pixels{ColourValue{0.0F, 0.0F, 1.0F, 1.0F},
                            ColourValue{1.0F, 0.0F, 0.0F, 1.0F},
                            ColourValue{0.0F, 0.0F, 1.0F, 1.0F}};
    const ColourIdSelection selection =
        select_colour_id({.width = 3, .height = 1, .pixels = pixels}, pixels[0], 0.0);
    const PaintMaskView paint = selection.as_paint_restriction();
    const PaintMaskView mask = selection.as_mask_source();
    const PaintMaskView visibility = selection.as_visibility_filter();
    const RejectedCoverageRaster restricted =
        apply_paint_masks(rejected(), {.active_layer_masks = {},
                                       .colour_id_selection = paint,
                                       .geometry_selection = std::nullopt,
                                       .screen_selection = std::nullopt,
                                       .uv_island_selection = std::nullopt});
    return expect(mask.values.data() == selection.values.data() &&
                      visibility.values.data() == selection.values.data(),
                  "colour-ID role adapters did not preserve the selection") &&
           expect(restricted.coverage.values == std::vector<double>({1.0, 0.0, 1.0}),
                  "colour-ID selection did not restrict subsequent paint");
}

bool invalid_inputs_are_refused() {
    const std::array pixels{ColourValue{1.0F, 0.0F, 0.0F, 1.0F}};
    bool size_refused = false;
    try {
        static_cast<void>(
            select_colour_id({.width = 2, .height = 1, .pixels = pixels}, pixels[0], 0.0));
    } catch (const std::invalid_argument&) {
        size_refused = true;
    }
    bool tolerance_refused = false;
    try {
        static_cast<void>(
            select_colour_id({.width = 1, .height = 1, .pixels = pixels}, pixels[0], -0.1));
    } catch (const std::invalid_argument&) {
        tolerance_refused = true;
    }
    const std::array invalid_pixels{
        ColourValue{std::numeric_limits<float>::quiet_NaN(), 0.0F, 0.0F, 1.0F}};
    bool pixels_refused = false;
    try {
        static_cast<void>(
            select_colour_id({.width = 1, .height = 1, .pixels = invalid_pixels}, pixels[0], 0.0));
    } catch (const std::invalid_argument&) {
        pixels_refused = true;
    }
    return expect(size_refused && tolerance_refused && pixels_refused,
                  "invalid colour-ID map or tolerance was not refused");
}

}  // namespace

int main() {
    return tolerance_selects_only_matching_ids() &&
                   zero_tolerance_reports_filtered_map_as_empty() &&
                   result_serves_every_selection_role() && invalid_inputs_are_refused()
               ? 0
               : 1;
}
