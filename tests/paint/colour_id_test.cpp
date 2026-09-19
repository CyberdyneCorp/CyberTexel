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

bool maximum_tolerance_covers_the_normalized_rgb_domain() {
    const std::array pixels{ColourValue{1.0F, 1.0F, 1.0F, 1.0F}};
    const ColourIdSelection result =
        select_colour_id({.width = 1, .height = 1, .pixels = pixels},
                         ColourValue{0.0F, 0.0F, 0.0F, 1.0F}, maximum_colour_id_tolerance + 1.0);
    return expect(result.status == ColourIdSelectionStatus::matched &&
                      result.tolerance == maximum_colour_id_tolerance &&
                      result.parameter_report.clamp_for("colour_id.tolerance") ==
                          ToolParameterClamp{.name = "colour_id.tolerance",
                                             .supplied = maximum_colour_id_tolerance + 1.0,
                                             .resolved = maximum_colour_id_tolerance},
                  "maximum colour-ID tolerance did not cover and report the RGB domain");
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
    const ColourIdSelection clamped =
        select_colour_id({.width = 1, .height = 1, .pixels = pixels}, pixels[0], -0.1);
    bool tolerance_refused = false;
    try {
        static_cast<void>(select_colour_id({.width = 1, .height = 1, .pixels = pixels}, pixels[0],
                                           std::numeric_limits<double>::infinity()));
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
    const std::array out_of_domain_pixels{ColourValue{1.1F, 0.0F, 0.0F, 1.0F}};
    bool domain_refused = false;
    try {
        static_cast<void>(select_colour_id(
            {.width = 1, .height = 1, .pixels = out_of_domain_pixels}, pixels[0], 0.0));
    } catch (const std::invalid_argument&) {
        domain_refused = true;
    }
    return expect(clamped.tolerance == 0.0 &&
                      clamped.parameter_report.clamp_for("colour_id.tolerance") ==
                          ToolParameterClamp{
                              .name = "colour_id.tolerance", .supplied = -0.1, .resolved = 0.0},
                  "colour-ID tolerance was not bounded and reported") &&
           expect(size_refused && tolerance_refused && pixels_refused && domain_refused,
                  "invalid colour-ID map or non-finite tolerance was not refused");
}

}  // namespace

int main() {
    return tolerance_selects_only_matching_ids() &&
                   zero_tolerance_reports_filtered_map_as_empty() &&
                   maximum_tolerance_covers_the_normalized_rgb_domain() &&
                   result_serves_every_selection_role() && invalid_inputs_are_refused()
               ? 0
               : 1;
}
