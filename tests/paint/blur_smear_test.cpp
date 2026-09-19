#include <algorithm>
#include <array>
#include <cmath>
#include <ctex/paint/blur_smear.hpp>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
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

bool near(float actual, float expected, float tolerance = 1.0e-6F) {
    return std::abs(actual - expected) <= tolerance;
}

Stamp stamp() {
    return {.position = {},
            .frame = {},
            .radius = 1.0,
            .opacity = 1.0,
            .hardness = 1.0,
            .rotation_radians = 0.0,
            .elongation = 1.0,
            .flow = 1.0,
            .tip_resource_identity = "builtin.circle",
            .source_ordinal = 0,
            .symmetry_instance = 0,
            .ordinal = 0};
}

ResolvedStroke stroke() {
    return {.reconstruction_version = canonical_stroke_reconstruction_version,
            .tip_mode = TipMode::discrete_alpha,
            .symmetry_instance_count = 1,
            .stamps = {stamp()},
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

PaintToolChannelRaster channel(std::string id, std::initializer_list<float> values,
                               std::uint8_t component_count = 1) {
    PaintToolChannelRaster result{
        .semantic_id = std::move(id), .component_count = component_count, .pixels = {}};
    for (const float value : values) {
        result.pixels.push_back({value, value, value, 1.0F});
    }
    return result;
}

SurfaceAdjacentSample sample(std::size_t texel, std::int32_t offset_x, std::int32_t offset_y = 0,
                             StrokeFrame frame = {}) {
    return {.texel_index = texel,
            .tangent_frame = frame,
            .offset_x = offset_x,
            .offset_y = offset_y,
            .weight = 1.0};
}

std::vector<BlurNeighborhood> linear_blur_neighborhoods(std::size_t count, std::uint32_t radius) {
    std::vector<BlurNeighborhood> result(count);
    for (std::size_t destination = 0; destination < count; ++destination) {
        const std::size_t first = destination > radius ? destination - radius : 0;
        const std::size_t last = std::min(count - 1, destination + radius);
        for (std::size_t source = first; source <= last; ++source) {
            result[destination].horizontal_samples.push_back(
                sample(source,
                       static_cast<std::int32_t>(source) - static_cast<std::int32_t>(destination)));
        }
        result[destination].vertical_samples.push_back(sample(destination, 0));
    }
    return result;
}

std::vector<SmearMapping> drag_from_left(std::size_t count) {
    std::vector<SmearMapping> result(count);
    for (std::size_t destination = 0; destination < count; ++destination) {
        const std::size_t source = destination == 0 ? 0 : destination - 1;
        result[destination] = {.output_frame = {},
                               .upstream_sample = sample(source, source == destination ? 0 : -1)};
    }
    return result;
}

BlurSettings blur_settings(std::uint32_t radius, std::span<const BlurNeighborhood> neighborhoods) {
    return {.radius = radius,
            .deposition_mode = DepositionMode::non_building,
            .blend_mode = "normal",
            .masks = {},
            .neighborhoods = neighborhoods};
}

SmearSettings smear_settings(std::span<const SmearMapping> mappings) {
    return {.strength = 0.5,
            .footprint = {.radius_x = 1, .radius_y = 0},
            .deposition_mode = DepositionMode::non_building,
            .blend_mode = "normal",
            .masks = {},
            .mappings = mappings};
}

bool blur_is_separable_configurable_and_snapshot_based() {
    const std::array layer{channel("pbr.roughness", {0.0F, 0.0F, 1.0F, 0.0F, 0.0F})};
    const auto radius_one = linear_blur_neighborhoods(5, 1);
    const auto radius_two = linear_blur_neighborhoods(5, 2);
    const BlurResult narrow =
        apply_blur(stroke(), coverage({1, 1, 1, 1, 1}), layer, blur_settings(1, radius_one));
    const BlurResult wide =
        apply_blur(stroke(), coverage({1, 1, 1, 1, 1}), layer, blur_settings(2, radius_two));

    return expect(near(narrow.channels[0].pixels[1].r, 1.0F / 3.0F) &&
                      near(narrow.channels[0].pixels[2].r, 1.0F / 3.0F) &&
                      near(narrow.channels[0].pixels[3].r, 1.0F / 3.0F),
                  "blur fed filtered output back into the same stroke") &&
           expect(near(wide.channels[0].pixels[2].r, 0.2F) &&
                      wide.footprint == SamplingFootprint{2, 2},
                  "blur radius did not change its declared separable footprint and output");
}

bool smear_drags_snapshot_content_with_strength_and_masks() {
    const std::array layer{channel("pbr.base_color", {0.0F, 0.2F, 0.4F, 0.6F}, 3)};
    const auto mappings = drag_from_left(4);
    const std::array<double, 4> selection{1.0, 1.0, 1.0, 0.0};
    SmearSettings settings = smear_settings(mappings);
    settings.masks.screen_selection = PaintMaskView{selection};
    const SmearResult result = apply_smear(stroke(), coverage({1, 1, 1, 1}), layer, settings);

    return expect(result.effective_strength == std::vector<double>({0.5, 0.5, 0.5, 0.0}),
                  "smear did not combine its strength with canonical masking") &&
           expect(near(result.channels[0].pixels[0].r, 0.0F) &&
                      near(result.channels[0].pixels[1].r, 0.1F) &&
                      near(result.channels[0].pixels[2].r, 0.3F) &&
                      near(result.channels[0].pixels[3].r, 0.6F),
                  "smear did not drag stroke-start content along its upstream mapping");
}

ColourValue encoded_normal(double x, double y, double z) {
    return {static_cast<float>(x * 0.5 + 0.5), static_cast<float>(y * 0.5 + 0.5),
            static_cast<float>(z * 0.5 + 0.5), 1.0F};
}

bool smear_transforms_normals_across_a_mirrored_seam() {
    const StrokeFrame regular{};
    const StrokeFrame mirrored{
        .tangent = {1.0, 0.0, 0.0}, .bitangent = {0.0, -1.0, 0.0}, .normal = {0.0, 0.0, 1.0}};
    const std::array layer{PaintToolChannelRaster{
        .semantic_id = "pbr.normal",
        .component_count = 3,
        .pixels = {encoded_normal(0.0, 0.6, 0.8), encoded_normal(0.0, -0.3, 0.4)}}};
    const std::array mappings{
        SmearMapping{.output_frame = regular, .upstream_sample = sample(1, 1, 0, mirrored)},
        SmearMapping{.output_frame = mirrored, .upstream_sample = sample(0, -1, 0, regular)},
    };
    SmearSettings settings = smear_settings(mappings);
    settings.strength = 1.0;
    const SmearResult result = apply_smear(stroke(), coverage({1, 0}), layer, settings);
    return expect(
        near(result.channels[0].pixels[0].g, 0.8F) && near(result.channels[0].pixels[1].g, 0.35F),
        "smear flipped a seam normal or changed a rejected destination");
}

bool blur_transforms_normals_in_both_separable_passes() {
    const StrokeFrame regular{};
    const StrokeFrame mirrored{
        .tangent = {1.0, 0.0, 0.0}, .bitangent = {0.0, -1.0, 0.0}, .normal = {0.0, 0.0, 1.0}};
    const std::array layer{PaintToolChannelRaster{
        .semantic_id = "pbr.normal",
        .component_count = 3,
        .pixels = {encoded_normal(0.0, 0.6, 0.8), encoded_normal(0.0, -0.6, 0.8)}}};
    std::array<BlurNeighborhood, 2> neighborhoods;
    neighborhoods[0] = {.output_frame = regular,
                        .horizontal_samples = {sample(0, 0, 0, regular), sample(1, 1, 0, mirrored)},
                        .vertical_samples = {sample(0, 0, 0, regular)}};
    neighborhoods[1] = {
        .output_frame = mirrored,
        .horizontal_samples = {sample(0, -1, 0, regular), sample(1, 0, 0, mirrored)},
        .vertical_samples = {sample(1, 0, 0, mirrored)}};
    const BlurResult result =
        apply_blur(stroke(), coverage({1, 1}), layer, blur_settings(1, neighborhoods));
    return expect(
        near(result.channels[0].pixels[0].g, 0.8F) && near(result.channels[0].pixels[1].g, 0.2F),
        "separable blur lost tangent-frame orientation across a mirrored seam");
}

bool blur_and_smear_parameters_are_bounded_and_reported() {
    const std::array layer{channel("pbr.roughness", {0.0F})};
    const auto neighborhoods = linear_blur_neighborhoods(1, 1);
    const auto mappings = drag_from_left(1);
    const BlurResult blur =
        apply_blur(stroke(), coverage({1}), layer, blur_settings(0, neighborhoods));

    SmearSettings clamped_settings = smear_settings(mappings);
    clamped_settings.strength = 2.0;
    clamped_settings.footprint = {.radius_x = maximum_blur_smear_radius + 1, .radius_y = 0};
    const SmearResult smear = apply_smear(stroke(), coverage({1}), layer, clamped_settings);

    bool zero_footprint_refused = false;
    try {
        SmearSettings invalid = smear_settings(mappings);
        invalid.footprint = {};
        static_cast<void>(apply_smear(stroke(), coverage({1}), layer, invalid));
    } catch (const std::invalid_argument&) {
        zero_footprint_refused = true;
    }
    bool non_finite_strength_refused = false;
    try {
        SmearSettings invalid = smear_settings(mappings);
        invalid.strength = std::numeric_limits<double>::infinity();
        static_cast<void>(apply_smear(stroke(), coverage({1}), layer, invalid));
    } catch (const std::invalid_argument&) {
        non_finite_strength_refused = true;
    }
    return expect(
               blur.footprint == SamplingFootprint{1, 1} &&
                   blur.parameter_report.clamp_for("blur.radius") ==
                       ToolParameterClamp{.name = "blur.radius", .supplied = 0.0, .resolved = 1.0},
               "blur radius was not bounded, reported, and used") &&
           expect(smear.strength == 1.0 &&
                      smear.footprint == SamplingFootprint{maximum_blur_smear_radius, 0} &&
                      smear.parameter_report.clamps.size() == 2 &&
                      smear.parameter_report.clamp_for("smear.strength") &&
                      smear.parameter_report.clamp_for("smear.footprint.radius_x"),
                  "smear parameters were not bounded, reported, and used") &&
           expect(zero_footprint_refused && non_finite_strength_refused,
                  "a structurally invalid smear request was accepted");
}

}  // namespace

int main() {
    return blur_is_separable_configurable_and_snapshot_based() &&
                   smear_drags_snapshot_content_with_strength_and_masks() &&
                   smear_transforms_normals_across_a_mirrored_seam() &&
                   blur_transforms_normals_in_both_separable_passes() &&
                   blur_and_smear_parameters_are_bounded_and_reported()
               ? 0
               : 1;
}
