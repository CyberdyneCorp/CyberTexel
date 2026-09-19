#include <array>
#include <cmath>
#include <ctex/paint/rejection.hpp>
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

Stamp stamp(Vec3d position, std::uint64_t source, std::uint64_t instance, std::uint64_t ordinal) {
    return {.position = position,
            .frame = {},
            .radius = 1.0,
            .opacity = 1.0,
            .hardness = 1.0,
            .rotation_radians = 0.0,
            .elongation = 1.0,
            .flow = 1.0,
            .tip_resource_identity = "builtin.circle",
            .source_ordinal = source,
            .symmetry_instance = instance,
            .ordinal = ordinal};
}

ResolvedStroke one_stamp_stroke() {
    return {.reconstruction_version = canonical_stroke_reconstruction_version,
            .tip_mode = TipMode::discrete_alpha,
            .symmetry_instance_count = 1,
            .stamps = {stamp({}, 0, 0, 0)},
            .swept_segments = {}};
}

SurfaceTexel texel(Vec3d normal = {0.0, 0.0, 1.0}, Vec3d geometric_normal = {0.0, 0.0, 1.0}) {
    return {.position = {},
            .normal = normal,
            .geometric_normal = geometric_normal,
            .uv = {},
            .triangle = 0};
}

TextureSpaceRaster surface(std::initializer_list<SurfaceTexel> texels) {
    return {.width = static_cast<std::uint32_t>(texels.size()),
            .height = 1,
            .tile_origin = {},
            .texels = texels};
}

DepthProjectionContext depth_context(std::span<const Vec2d> screen,
                                     std::span<const double> surface_depth,
                                     std::span<const double> visible_depth,
                                     std::uint64_t instance = 0, bool transform_consistent = true) {
    return {.symmetry_instance = instance,
            .viewport_width = 1,
            .viewport_height = 1,
            .screen_positions = screen,
            .surface_depth = surface_depth,
            .visible_depth = visible_depth,
            .transform_consistent = transform_consistent};
}

bool default_depth_and_angle_rejection_filter_contributions() {
    const TextureSpaceRaster samples = surface({texel(), texel(), texel({1.0, 0.0, 0.0})});
    const std::array<Vec2d, 3> screen{Vec2d{0.5, 0.5}, Vec2d{0.5, 0.5}, Vec2d{0.5, 0.5}};
    const std::array<double, 3> projected_depth{0.5, 0.7, 0.5};
    const std::array<double, 1> visible_depth{0.5};
    const std::array contexts{
        depth_context(screen, projected_depth, visible_depth),
    };
    const auto result = evaluate_rejected_coverage(
        samples, one_stamp_stroke(), {}, {.depth_contexts = contexts, .view_directions = {}});
    return expect(result.coverage.values == std::vector<double>({1.0, 0.0, 0.0}),
                  "default depth and angle rejection did not filter covered texels") &&
           expect(result.report.depth_disposition ==
                          DepthRejectionDisposition::consistent_per_instance &&
                      result.report.depth_rejected_contributions == 1 &&
                      result.report.angle_rejected_contributions == 1,
                  "rejection report did not attribute the filtered contributions");
}

bool depth_rejection_can_be_disabled_per_operation() {
    const TextureSpaceRaster samples = surface({texel()});
    const std::array<Vec2d, 1> screen{Vec2d{0.5, 0.5}};
    const std::array<double, 1> projected_depth{0.7};
    const std::array<double, 1> visible_depth{0.5};
    const std::array contexts{depth_context(screen, projected_depth, visible_depth)};
    const auto result = evaluate_rejected_coverage(
        samples, one_stamp_stroke(),
        {.depth_enabled = false,
         .depth_bias = default_depth_rejection_bias,
         .symmetry_depth_policy = SymmetryDepthPolicy::require_consistent_per_instance,
         .angle_enabled = true,
         .minimum_normal_dot = default_angle_rejection_dot,
         .backface_enabled = false},
        {.depth_contexts = contexts, .view_directions = {}});
    return expect(
        result.coverage.values == std::vector<double>({1.0}) &&
            result.report.depth_disposition == DepthRejectionDisposition::disabled_by_operation,
        "disabled depth rejection still required or applied a depth buffer");
}

// parameter-audit: rejection.depth_bias
// parameter-audit: rejection.minimum_normal_dot
bool depth_bias_and_angle_threshold_are_configurable() {
    const TextureSpaceRaster samples = surface({texel({0.8, 0.0, 0.6})});
    const std::array<Vec2d, 1> screen{Vec2d{0.5, 0.5}};
    const std::array<double, 1> projected_depth{0.50005};
    const std::array<double, 1> visible_depth{0.5};
    const std::array contexts{depth_context(screen, projected_depth, visible_depth)};
    const auto defaults = evaluate_rejected_coverage(
        samples, one_stamp_stroke(), {}, {.depth_contexts = contexts, .view_directions = {}});
    const auto strict_depth = evaluate_rejected_coverage(
        samples, one_stamp_stroke(),
        {.depth_enabled = true,
         .depth_bias = 0.0,
         .symmetry_depth_policy = SymmetryDepthPolicy::require_consistent_per_instance,
         .angle_enabled = true,
         .minimum_normal_dot = default_angle_rejection_dot,
         .backface_enabled = false},
        {.depth_contexts = contexts, .view_directions = {}});
    const auto strict_angle = evaluate_rejected_coverage(
        samples, one_stamp_stroke(),
        {.depth_enabled = true,
         .depth_bias = default_depth_rejection_bias,
         .symmetry_depth_policy = SymmetryDepthPolicy::require_consistent_per_instance,
         .angle_enabled = true,
         .minimum_normal_dot = 0.7,
         .backface_enabled = false},
        {.depth_contexts = contexts, .view_directions = {}});
    return expect(defaults.coverage.values == std::vector<double>({1.0}),
                  "default depth bias or angle threshold rejected an accepted texel") &&
           expect(strict_depth.coverage.values == std::vector<double>({0.0}) &&
                      strict_depth.report.depth_rejected_contributions == 1,
                  "configurable depth bias was not applied") &&
           expect(strict_angle.coverage.values == std::vector<double>({0.0}) &&
                      strict_angle.report.angle_rejected_contributions == 1,
                  "configurable angle threshold was not applied");
}

bool symmetry_requires_consistent_depth_or_reports_disabled_derived_depth() {
    ResolvedStroke stroke{.reconstruction_version = canonical_stroke_reconstruction_version,
                          .tip_mode = TipMode::discrete_alpha,
                          .symmetry_instance_count = 2,
                          .stamps = {stamp({10.0, 0.0, 0.0}, 0, 0, 0), stamp({}, 0, 1, 1)},
                          .swept_segments = {}};
    const TextureSpaceRaster samples = surface({texel()});
    const std::array<Vec2d, 1> screen{Vec2d{0.5, 0.5}};
    const std::array<double, 1> projected_depth{0.7};
    const std::array<double, 1> visible_depth{0.5};
    const std::array base_context{
        depth_context(screen, projected_depth, visible_depth),
    };
    bool missing_mirror_refused = false;
    try {
        static_cast<void>(evaluate_rejected_coverage(
            samples, stroke, {}, {.depth_contexts = base_context, .view_directions = {}}));
    } catch (const std::invalid_argument&) {
        missing_mirror_refused = true;
    }
    const std::array both_contexts{
        depth_context(screen, projected_depth, visible_depth, 0),
        depth_context(screen, projected_depth, visible_depth, 1),
    };
    const std::array inconsistent_contexts{
        depth_context(screen, projected_depth, visible_depth, 0),
        depth_context(screen, projected_depth, visible_depth, 1, false),
    };
    bool inconsistent_mirror_refused = false;
    try {
        static_cast<void>(evaluate_rejected_coverage(
            samples, stroke, {}, {.depth_contexts = inconsistent_contexts, .view_directions = {}}));
    } catch (const std::invalid_argument&) {
        inconsistent_mirror_refused = true;
    }
    const auto consistent = evaluate_rejected_coverage(
        samples, stroke, {}, {.depth_contexts = both_contexts, .view_directions = {}});
    const auto disabled = evaluate_rejected_coverage(
        samples, stroke,
        {.depth_enabled = true,
         .depth_bias = default_depth_rejection_bias,
         .symmetry_depth_policy = SymmetryDepthPolicy::disable_for_derived_symmetry,
         .angle_enabled = true,
         .minimum_normal_dot = default_angle_rejection_dot,
         .backface_enabled = false},
        {.depth_contexts = base_context, .view_directions = {}});
    return expect(missing_mirror_refused && inconsistent_mirror_refused,
                  "symmetry accepted an implicit inconsistent depth transform") &&
           expect(consistent.coverage.values == std::vector<double>({0.0}),
                  "per-instance depth did not reject an occluded mirrored contribution") &&
           expect(disabled.coverage.values == std::vector<double>({1.0}) &&
                      disabled.report.depth_disposition ==
                          DepthRejectionDisposition::disabled_for_derived_symmetry,
                  "derived-symmetry depth disablement was not applied and reported");
}

bool backface_rejection_uses_counter_clockwise_geometric_normal() {
    const TextureSpaceRaster samples = surface({texel({0.0, 0.0, 1.0}, {0.0, 0.0, -1.0})});
    const std::array<Vec3d, 1> view{Vec3d{0.0, 0.0, 1.0}};
    const RejectionSettings settings{
        .depth_enabled = false,
        .depth_bias = default_depth_rejection_bias,
        .symmetry_depth_policy = SymmetryDepthPolicy::require_consistent_per_instance,
        .angle_enabled = false,
        .minimum_normal_dot = default_angle_rejection_dot,
        .backface_enabled = true};
    const auto result = evaluate_rejected_coverage(samples, one_stamp_stroke(), settings,
                                                   {.depth_contexts = {}, .view_directions = view});
    RejectionSettings disabled = settings;
    disabled.backface_enabled = false;
    const auto accepted = evaluate_rejected_coverage(samples, one_stamp_stroke(), disabled,
                                                     {.depth_contexts = {}, .view_directions = {}});
    return expect(result.coverage.values == std::vector<double>({0.0}) &&
                      result.report.backface_rejected_texels == 1,
                  "backface rejection did not use the triangle winding normal") &&
           expect(accepted.coverage.values == std::vector<double>({1.0}),
                  "disabled backface rejection still discarded an away-facing texel");
}

// parameter-audit: alpha_discard.threshold
bool alpha_discard_uses_precision_defaults_without_losing_accumulation() {
    const std::array<double, 4> strength{0.003, 0.004, 0.099, 0.1};
    const auto eight_bit = apply_alpha_discard(
        strength, {.format = AlphaDiscardFormat::unorm8, .threshold = std::nullopt});
    const auto float_result = apply_alpha_discard(
        strength, {.format = AlphaDiscardFormat::floating_point, .threshold = std::nullopt});
    const auto sixteen_bit = apply_alpha_discard(
        strength, {.format = AlphaDiscardFormat::unorm16, .threshold = std::nullopt});
    const auto custom =
        apply_alpha_discard(strength, {.format = AlphaDiscardFormat::unorm8, .threshold = 0.05});
    return expect(eight_bit.threshold == 0.1 &&
                      eight_bit.write_mask == std::vector<std::uint8_t>({0, 0, 0, 1}),
                  "8-bit alpha discard did not use the 0.1 default") &&
           expect(float_result.threshold == 0.004 &&
                      float_result.write_mask == std::vector<std::uint8_t>({0, 1, 1, 1}),
                  "high-precision alpha discard did not use the 0.004 default") &&
           expect(
               sixteen_bit.threshold == 0.004 && sixteen_bit.write_mask == float_result.write_mask,
               "16-bit alpha discard did not use the high-precision default") &&
           expect(custom.write_mask == std::vector<std::uint8_t>({0, 0, 1, 1}),
                  "custom alpha discard threshold was not applied") &&
           expect(eight_bit.retained_strength ==
                          std::vector<double>(strength.begin(), strength.end()) &&
                      float_result.retained_strength ==
                          std::vector<double>(strength.begin(), strength.end()),
                  "alpha discard erased accumulated deposition below the write threshold");
}

bool rejection_parameters_are_bounded_reported_and_used() {
    const TextureSpaceRaster samples = surface({texel({0.8, 0.0, 0.6})});
    const std::array<Vec2d, 1> screen{Vec2d{0.5, 0.5}};
    const std::array<double, 1> projected_depth{0.50005};
    const std::array<double, 1> visible_depth{0.5};
    const std::array contexts{depth_context(screen, projected_depth, visible_depth)};
    const RejectedCoverageRaster depth = evaluate_rejected_coverage(
        samples, one_stamp_stroke(),
        {.depth_enabled = true,
         .depth_bias = -1.0,
         .symmetry_depth_policy = SymmetryDepthPolicy::require_consistent_per_instance,
         .angle_enabled = false,
         .minimum_normal_dot = default_angle_rejection_dot,
         .backface_enabled = false},
        {.depth_contexts = contexts, .view_directions = {}});
    const RejectedCoverageRaster angle = evaluate_rejected_coverage(
        samples, one_stamp_stroke(),
        {.depth_enabled = false,
         .depth_bias = default_depth_rejection_bias,
         .symmetry_depth_policy = SymmetryDepthPolicy::require_consistent_per_instance,
         .angle_enabled = true,
         .minimum_normal_dot = 2.0,
         .backface_enabled = false});
    const std::array<double, 2> strength{0.5, 1.0};
    const AlphaDiscardResult alpha =
        apply_alpha_discard(strength, {.format = AlphaDiscardFormat::unorm8, .threshold = 2.0});

    return expect(depth.coverage.values == std::vector<double>({0.0}) &&
                      depth.report.resolved_settings.depth_bias == 0.0 &&
                      depth.report.parameter_report.clamp_for("rejection.depth_bias") ==
                          ToolParameterClamp{"rejection.depth_bias", -1.0, 0.0},
                  "resolved depth bias did not drive rejection or report its clamp") &&
           expect(angle.coverage.values == std::vector<double>({0.0}) &&
                      angle.report.resolved_settings.minimum_normal_dot == 1.0 &&
                      angle.report.parameter_report.clamp_for("rejection.minimum_normal_dot") ==
                          ToolParameterClamp{"rejection.minimum_normal_dot", 2.0, 1.0},
                  "resolved angle threshold did not drive rejection or report its clamp") &&
           expect(alpha.threshold == 1.0 && alpha.write_mask == std::vector<std::uint8_t>({0, 1}) &&
                      alpha.parameter_report.clamp_for("alpha_discard.threshold") ==
                          ToolParameterClamp{"alpha_discard.threshold", 2.0, 1.0},
                  "resolved alpha threshold did not drive writes or report its clamp");
}

bool invalid_rejection_inputs_are_refused() {
    const TextureSpaceRaster samples = surface({texel()});
    bool view_refused = false;
    try {
        static_cast<void>(evaluate_rejected_coverage(
            samples, one_stamp_stroke(),
            {.depth_enabled = false,
             .depth_bias = default_depth_rejection_bias,
             .symmetry_depth_policy = SymmetryDepthPolicy::require_consistent_per_instance,
             .angle_enabled = true,
             .minimum_normal_dot = default_angle_rejection_dot,
             .backface_enabled = true}));
    } catch (const std::invalid_argument&) {
        view_refused = true;
    }
    bool alpha_refused = false;
    try {
        const std::array strength{std::numeric_limits<double>::quiet_NaN()};
        static_cast<void>(apply_alpha_discard(strength));
    } catch (const std::invalid_argument&) {
        alpha_refused = true;
    }
    return expect(view_refused && alpha_refused,
                  "invalid rejection or alpha-discard input was not refused");
}

}  // namespace

int main() {
    return default_depth_and_angle_rejection_filter_contributions() &&
                   depth_rejection_can_be_disabled_per_operation() &&
                   depth_bias_and_angle_threshold_are_configurable() &&
                   symmetry_requires_consistent_depth_or_reports_disabled_derived_depth() &&
                   backface_rejection_uses_counter_clockwise_geometric_normal() &&
                   alpha_discard_uses_precision_defaults_without_losing_accumulation() &&
                   rejection_parameters_are_bounded_reported_and_used() &&
                   invalid_rejection_inputs_are_refused()
               ? 0
               : 1;
}
