#include <array>
#include <cmath>
#include <ctex/paint/brush.hpp>
#include <iostream>
#include <stdexcept>
#include <string_view>
#include <utility>
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

bool near(double actual, double expected, double tolerance = 1.0e-9) {
    return std::abs(actual - expected) <= tolerance;
}

bool near(float actual, float expected, float tolerance = 1.0e-6F) {
    return std::abs(actual - expected) <= tolerance;
}

Stamp stamp(double opacity = 1.0, double flow = 1.0) {
    return {.position = {},
            .frame = {},
            .radius = 1.0,
            .opacity = opacity,
            .hardness = 1.0,
            .rotation_radians = 0.0,
            .elongation = 1.0,
            .flow = flow,
            .tip_resource_identity = "builtin.circle",
            .source_ordinal = 0,
            .symmetry_instance = 0,
            .ordinal = 0};
}

ResolvedStroke stroke(double opacity = 1.0, double flow = 1.0) {
    return {.reconstruction_version = canonical_stroke_reconstruction_version,
            .tip_mode = TipMode::discrete_alpha,
            .symmetry_instance_count = 1,
            .stamps = {stamp(opacity, flow)},
            .swept_segments = {}};
}

RejectedCoverageRaster coverage(std::initializer_list<double> values) {
    const std::vector<double> pixels(values);
    return {.coverage = {.width = static_cast<std::uint32_t>(pixels.size()),
                         .height = 1,
                         .values = pixels},
            .stamp_events = {{.stamp_ordinal = 0, .values = pixels}},
            .report = {}};
}

PaintToolChannelRaster channel(std::string semantic_id, std::initializer_list<ColourValue> pixels,
                               std::uint8_t component_count = 3) {
    return {.semantic_id = std::move(semantic_id),
            .component_count = component_count,
            .pixels = pixels};
}

bool brush_applies_material_to_every_enabled_channel() {
    const std::array layer{
        channel("pbr.base_color", {{0.0F, 0.0F, 0.0F, 1.0F}, {0.0F, 0.0F, 0.0F, 1.0F}}),
        channel("pbr.roughness", {{0.2F, 0.2F, 0.2F, 1.0F}, {0.2F, 0.2F, 0.2F, 1.0F}}, 1),
    };
    const std::array material{
        channel("pbr.base_color", {{1.0F, 0.0F, 0.0F, 1.0F}, {1.0F, 0.0F, 0.0F, 1.0F}}),
        channel("pbr.roughness", {{0.8F, 0.8F, 0.8F, 1.0F}, {0.8F, 0.8F, 0.8F, 1.0F}}, 1),
        channel("pbr.metallic", {{1.0F, 1.0F, 1.0F, 1.0F}, {1.0F, 1.0F, 1.0F, 1.0F}}, 1),
    };
    const std::array<double, 2> selection{1.0, 0.0};
    const BrushResult result = apply_brush(stroke(0.5), coverage({1.0, 1.0}), layer, material,
                                           {.deposition_mode = DepositionMode::non_building,
                                            .blend_mode = "normal",
                                            .masks = {.active_layer_masks = {},
                                                      .colour_id_selection = std::nullopt,
                                                      .geometry_selection = std::nullopt,
                                                      .screen_selection = PaintMaskView{selection},
                                                      .uv_island_selection = std::nullopt}});
    return expect(result.applied_channel_ids ==
                      std::vector<std::string>({"pbr.base_color", "pbr.roughness"}),
                  "brush did not report every enabled channel in layer order") &&
           expect(result.channels.size() == 2 && result.channels[0].component_count == 3 &&
                      result.channels[1].component_count == 1 &&
                      result.channels[0].pixels[1] == layer[0].pixels[1] &&
                      result.channels[1].pixels[1] == layer[1].pixels[1],
                  "brush modified a disabled or masked channel texel") &&
           expect(near(result.channels[0].pixels[0].r, 0.5F) &&
                      near(result.channels[1].pixels[0].r, 0.5F),
                  "brush did not apply material through canonical stroke strength");
}

bool eraser_reduces_layer_opacity_and_mask_values() {
    const ResolvedStroke half_strength = stroke(0.5);
    const RejectedCoverageRaster accepted = coverage({1.0, 0.5});
    const std::array layer_opacity{0.8, 0.4};
    const EraserResult layer =
        apply_eraser(half_strength, accepted, layer_opacity, EraserTarget::layer_opacity);
    const std::array mask_values{0.6, 1.0};
    const EraserResult mask =
        apply_eraser(half_strength, accepted, mask_values, EraserTarget::mask);
    return expect(near(layer.values[0], 0.4) && near(layer.values[1], 0.3),
                  "eraser did not reduce layer opacity in proportion to stroke strength") &&
           expect(near(mask.values[0], 0.3) && near(mask.values[1], 0.75) &&
                      mask.target == EraserTarget::mask,
                  "eraser did not reduce mask values through the same stroke path");
}

bool invalid_tool_inputs_are_transactionally_refused() {
    const std::array layer{channel("pbr.base_color", {{0.0F, 0.0F, 0.0F, 1.0F}})};
    const std::array wrong_material{channel("pbr.roughness", {{0.5F, 0.5F, 0.5F, 1.0F}})};
    bool missing_channel_refused = false;
    try {
        static_cast<void>(apply_brush(stroke(), coverage({1.0}), layer, wrong_material));
    } catch (const std::invalid_argument&) {
        missing_channel_refused = true;
    }
    const std::array wrong_components{channel("pbr.base_color", {{0.5F, 0.5F, 0.5F, 1.0F}}, 4)};
    bool component_mismatch_refused = false;
    try {
        static_cast<void>(apply_brush(stroke(), coverage({1.0}), layer, wrong_components));
    } catch (const std::invalid_argument&) {
        component_mismatch_refused = true;
    }
    bool invalid_opacity_refused = false;
    try {
        const std::array invalid{1.1};
        static_cast<void>(
            apply_eraser(stroke(), coverage({1.0}), invalid, EraserTarget::layer_opacity));
    } catch (const std::invalid_argument&) {
        invalid_opacity_refused = true;
    }
    return expect(missing_channel_refused && component_mismatch_refused &&
                      invalid_opacity_refused &&
                      layer[0].pixels[0] == ColourValue{0.0F, 0.0F, 0.0F, 1.0F},
                  "invalid brush or eraser input changed its stroke-start source");
}

}  // namespace

int main() {
    return brush_applies_material_to_every_enabled_channel() &&
                   eraser_reduces_layer_opacity_and_mask_values() &&
                   invalid_tool_inputs_are_transactionally_refused()
               ? 0
               : 1;
}
