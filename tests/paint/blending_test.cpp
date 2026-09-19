#include <array>
#include <cmath>
#include <ctex/graph/portable_nodes.hpp>
#include <ctex/paint/blending.hpp>
#include <iostream>
#include <limits>
#include <string_view>
#include <vector>

namespace {

using ctex::graph::ColourValue;
using namespace ctex::paint;

constexpr std::array blend_modes{
    std::string_view{"normal"},       std::string_view{"darken"},
    std::string_view{"multiply"},     std::string_view{"color_burn"},
    std::string_view{"lighten"},      std::string_view{"screen"},
    std::string_view{"color_dodge"},  std::string_view{"add"},
    std::string_view{"overlay"},      std::string_view{"soft_light"},
    std::string_view{"linear_light"}, std::string_view{"difference"},
    std::string_view{"exclusion"},    std::string_view{"subtract"},
    std::string_view{"divide"},       std::string_view{"hue"},
    std::string_view{"saturation"},   std::string_view{"color"},
    std::string_view{"value"},        std::string_view{"pass_through"},
};

bool expect(bool condition, std::string_view message) {
    if (!condition) {
        std::cerr << message << '\n';
    }
    return condition;
}

bool near(float actual, float expected, float tolerance = 1.0e-6F) {
    return std::abs(actual - expected) <= tolerance;
}

bool near(ColourValue actual, ColourValue expected) {
    return near(actual.r, expected.r) && near(actual.g, expected.g) && near(actual.b, expected.b) &&
           near(actual.a, expected.a);
}

DepositionRaster deposition(std::initializer_list<double> strength) {
    return {.width = static_cast<std::uint32_t>(strength.size()),
            .height = 1,
            .mode = DepositionMode::non_building,
            .non_building_coverage = std::vector<double>(strength.size(), 0.0),
            .build_up_deposition = std::vector<double>(strength.size(), 0.0),
            .strength = strength,
            .applied_stamp_count = 1};
}

bool multiply_uses_the_stroke_start_snapshot() {
    std::vector<ColourValue> source{{0.25F, 0.5F, 0.75F, 0.2F}};
    const std::array paint{ColourValue{0.8F, 0.4F, 0.2F, 1.0F}};
    StrokeSnapshotBlender blender(1, 1, source, "multiply");
    source[0] = {1.0F, 1.0F, 1.0F, 1.0F};
    const std::array first_strength{0.5};
    blender.shade(paint, first_strength);
    const ColourValue first = blender.result().pixels[0];
    const std::array second_strength{0.75};
    blender.shade(paint, second_strength);
    const ColourValue expected_first = ctex::graph::blend_colour(
        "multiply", {0.25F, 0.5F, 0.75F, 0.2F}, paint[0], first_strength[0]);
    const ColourValue expected_second = ctex::graph::blend_colour(
        "multiply", {0.25F, 0.5F, 0.75F, 0.2F}, paint[0], second_strength[0]);
    const ColourValue recursive_second =
        ctex::graph::blend_colour("multiply", first, paint[0], second_strength[0]);
    return expect(near(first, expected_first),
                  "multiply paint did not interpolate from the stroke-start value") &&
           expect(near(blender.result().pixels[0], expected_second) &&
                      !near(blender.result().pixels[0], recursive_second),
                  "later shading read the partially painted result instead of the snapshot") &&
           expect(blender.stroke_start_snapshot()[0] == ColourValue{0.25F, 0.5F, 0.75F, 0.2F},
                  "stroke-start snapshot aliased mutable caller storage");
}

bool every_blend_mode_uses_the_shared_formula() {
    constexpr ColourValue base{0.2F, 0.6F, 0.8F, 0.4F};
    constexpr ColourValue paint{0.9F, 0.3F, 0.1F, 0.8F};
    constexpr double strength = 0.65;
    for (const std::string_view mode : blend_modes) {
        StrokeSnapshotBlender blender(1, 1, std::span(&base, 1), mode);
        blender.shade(std::span(&paint, 1), std::span(&strength, 1));
        if (!expect(near(blender.result().pixels[0],
                         ctex::graph::blend_colour(mode, base, paint, strength)),
                    "paint diverged from the shared formula for " + std::string(mode))) {
            return false;
        }
    }
    return true;
}

bool separate_strokes_accumulate_from_separate_snapshots() {
    const std::array black{ColourValue{0.0F, 0.0F, 0.0F, 1.0F}};
    const std::array red{ColourValue{1.0F, 0.0F, 0.0F, 1.0F}};
    const DepositionRaster half = deposition({0.5});
    const auto first = blend_stroke_snapshot(1, 1, black, red, half, "normal");
    const auto second = blend_stroke_snapshot(1, 1, first.pixels, red, half, "normal");
    return expect(near(first.pixels[0], {0.5F, 0.0F, 0.0F, 1.0F}) &&
                      near(second.pixels[0], {0.75F, 0.0F, 0.0F, 1.0F}),
                  "separate strokes did not accumulate from distinct start snapshots");
}

bool deposition_strength_drives_the_shading_stage() {
    const std::array snapshot{ColourValue{0.1F, 0.2F, 0.3F, 1.0F},
                              ColourValue{0.8F, 0.7F, 0.6F, 1.0F}};
    const std::array paint{ColourValue{1.0F, 1.0F, 1.0F, 1.0F},
                           ColourValue{0.0F, 0.0F, 0.0F, 1.0F}};
    const DepositionRaster strength = deposition({0.0, 1.0});
    const auto result = blend_stroke_snapshot(2, 1, snapshot, paint, strength, "normal");
    return expect(result.pixels[0] == snapshot[0] && result.pixels[1] == paint[1],
                  "shading did not consume the deposition strength per texel");
}

bool invalid_shading_is_transactionally_refused() {
    const std::array snapshot{ColourValue{0.1F, 0.2F, 0.3F, 1.0F},
                              ColourValue{0.4F, 0.5F, 0.6F, 1.0F}};
    const std::array paint{ColourValue{0.8F, 0.7F, 0.6F, 1.0F},
                           ColourValue{0.2F, 0.3F, 0.4F, 1.0F}};
    StrokeSnapshotBlender blender(2, 1, snapshot, "screen");
    const std::array valid_strength{0.25, 0.75};
    blender.shade(paint, valid_strength);
    const auto before = blender.result().pixels;
    const std::array invalid_strength{0.5, std::numeric_limits<double>::quiet_NaN()};
    bool strength_refused = false;
    try {
        blender.shade(paint, invalid_strength);
    } catch (const std::invalid_argument&) {
        strength_refused = true;
    }
    bool mode_refused = false;
    try {
        static_cast<void>(StrokeSnapshotBlender(2, 1, snapshot, "unknown"));
    } catch (const std::invalid_argument&) {
        mode_refused = true;
    }
    bool dimensions_refused = false;
    try {
        static_cast<void>(
            blend_stroke_snapshot(1, 1, snapshot, paint, deposition({0.5}), "normal"));
    } catch (const std::invalid_argument&) {
        dimensions_refused = true;
    }
    return expect(
        strength_refused && mode_refused && dimensions_refused && blender.result().pixels == before,
        "invalid shading partially changed output or was not refused");
}

}  // namespace

int main() {
    return multiply_uses_the_stroke_start_snapshot() &&
                   every_blend_mode_uses_the_shared_formula() &&
                   separate_strokes_accumulate_from_separate_snapshots() &&
                   deposition_strength_drives_the_shading_stage() &&
                   invalid_shading_is_transactionally_refused()
               ? 0
               : 1;
}
