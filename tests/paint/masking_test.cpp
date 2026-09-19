#include <array>
#include <cmath>
#include <ctex/paint/deposition.hpp>
#include <ctex/paint/masking.hpp>
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

bool near(double actual, double expected, double tolerance = 1.0e-12) {
    return std::abs(actual - expected) <= tolerance;
}

RejectedCoverageRaster rejected() {
    return {
        .coverage = {.width = 3, .height = 1, .values = {1.0, 1.0, 0.75}},
        .stamp_events = {{.stamp_ordinal = 0, .values = {1.0, 0.5, 0.25}},
                         {.stamp_ordinal = 1, .values = {0.5, 1.0, 0.75}}},
        .report = {},
    };
}

ResolvedStroke stroke() {
    const auto make_stamp = [](std::uint64_t ordinal) {
        return Stamp{.position = {},
                     .frame = {},
                     .radius = 1.0,
                     .opacity = 1.0,
                     .hardness = 1.0,
                     .rotation_radians = 0.0,
                     .elongation = 1.0,
                     .flow = 1.0,
                     .tip_resource_identity = "builtin.circle",
                     .source_ordinal = ordinal,
                     .symmetry_instance = 0,
                     .ordinal = ordinal};
    };
    return {.reconstruction_version = canonical_stroke_reconstruction_version,
            .tip_mode = TipMode::discrete_alpha,
            .symmetry_instance_count = 1,
            .stamps = {make_stamp(0), make_stamp(1)},
            .swept_segments = {}};
}

bool absent_masks_are_identity() {
    const RejectedCoverageRaster source = rejected();
    const CombinedPaintMask mask = combine_paint_masks(3, 1);
    const auto result = apply_paint_masks(source);
    return expect(
               mask.values == std::vector<double>({1.0, 1.0, 1.0}) && mask.active_input_count == 0,
               "inactive masks did not produce the identity mask") &&
           expect(result.coverage.values == source.coverage.values &&
                      result.stamp_events[0].values == source.stamp_events[0].values &&
                      result.stamp_events[1].values == source.stamp_events[1].values,
                  "inactive masks changed accepted coverage");
}

bool colour_id_and_screen_selections_intersect() {
    const std::array<double, 3> colour_id{1.0, 1.0, 0.0};
    const std::array<double, 3> screen{0.0, 1.0, 1.0};
    const PaintMaskInputs inputs{
        .active_layer_masks = {},
        .colour_id_selection = PaintMaskView{colour_id},
        .geometry_selection = std::nullopt,
        .screen_selection = PaintMaskView{screen},
        .uv_island_selection = std::nullopt,
    };
    const auto mask = combine_paint_masks(3, 1, inputs);
    const auto result = apply_paint_masks(rejected(), inputs);
    return expect(
               mask.values == std::vector<double>({0.0, 1.0, 0.0}) && mask.active_input_count == 2,
               "colour-ID and screen selections did not intersect") &&
           expect(result.coverage.values == std::vector<double>({0.0, 1.0, 0.0}) &&
                      result.stamp_events[0].values == std::vector<double>({0.0, 0.5, 0.0}) &&
                      result.stamp_events[1].values == std::vector<double>({0.0, 1.0, 0.0}),
                  "selection intersection did not restrict every stamp event");
}

bool every_mask_class_multiplies_before_deposition() {
    const std::array<double, 3> layer_a{0.5, 0.5, 0.5};
    const std::array<double, 3> layer_b{0.5, 1.0, 1.0};
    const std::array layer_masks{PaintMaskView{layer_a}, PaintMaskView{layer_b}};
    const std::array<double, 3> colour_id{0.8, 1.0, 1.0};
    const std::array<double, 3> geometry{0.5, 1.0, 1.0};
    const std::array<double, 3> screen{0.25, 1.0, 1.0};
    const std::array<double, 3> island{0.5, 1.0, 1.0};
    const PaintMaskInputs inputs{
        .active_layer_masks = layer_masks,
        .colour_id_selection = PaintMaskView{colour_id},
        .geometry_selection = PaintMaskView{geometry},
        .screen_selection = PaintMaskView{screen},
        .uv_island_selection = PaintMaskView{island},
    };
    const auto mask = combine_paint_masks(3, 1, inputs);
    const auto masked = apply_paint_masks(rejected(), inputs);
    const auto deposition = evaluate_deposition(stroke(), masked);
    return expect(mask.active_input_count == 6 && near(mask.values[0], 0.0125) &&
                      near(mask.values[1], 0.5) && near(mask.values[2], 0.5),
                  "one or more mask classes were omitted from the intersection") &&
           expect(near(deposition.strength[0], 0.0125) && near(deposition.strength[1], 0.5) &&
                      near(deposition.strength[2], 0.375),
                  "mask intersection was not applied to c_i before deposition");
}

bool invalid_masks_are_transactionally_refused() {
    const RejectedCoverageRaster source = rejected();
    const std::array<double, 2> wrong_size{1.0, 1.0};
    bool size_refused = false;
    try {
        static_cast<void>(
            apply_paint_masks(source, {.active_layer_masks = {},
                                       .colour_id_selection = PaintMaskView{wrong_size},
                                       .geometry_selection = std::nullopt,
                                       .screen_selection = std::nullopt,
                                       .uv_island_selection = std::nullopt}));
    } catch (const std::invalid_argument&) {
        size_refused = true;
    }
    const std::array<double, 3> invalid_value{1.0, std::numeric_limits<double>::quiet_NaN(), 1.0};
    bool value_refused = false;
    try {
        static_cast<void>(combine_paint_masks(3, 1,
                                              {.active_layer_masks = {},
                                               .colour_id_selection = std::nullopt,
                                               .geometry_selection = PaintMaskView{invalid_value},
                                               .screen_selection = std::nullopt,
                                               .uv_island_selection = std::nullopt}));
    } catch (const std::invalid_argument&) {
        value_refused = true;
    }
    return expect(size_refused && value_refused &&
                      source.coverage.values == std::vector<double>({1.0, 1.0, 0.75}),
                  "invalid mask changed source coverage or was not refused");
}

}  // namespace

int main() {
    return absent_masks_are_identity() && colour_id_and_screen_selections_intersect() &&
                   every_mask_class_multiplies_before_deposition() &&
                   invalid_masks_are_transactionally_refused()
               ? 0
               : 1;
}
