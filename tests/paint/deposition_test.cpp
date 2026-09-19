#include <array>
#include <cmath>
#include <ctex/paint/deposition.hpp>
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

Stamp stamp(std::uint64_t ordinal, double opacity = 1.0, double flow = 1.0, Vec3d position = {}) {
    return {.position = position,
            .frame = {},
            .radius = 1.0,
            .opacity = opacity,
            .hardness = 1.0,
            .rotation_radians = 0.0,
            .elongation = 1.0,
            .flow = flow,
            .tip_resource_identity = "builtin.circle",
            .source_ordinal = ordinal,
            .symmetry_instance = 0,
            .ordinal = ordinal};
}

ResolvedStroke discrete_stroke(std::initializer_list<Stamp> stamps) {
    return {.reconstruction_version = canonical_stroke_reconstruction_version,
            .tip_mode = TipMode::discrete_alpha,
            .symmetry_instance_count = 1,
            .stamps = stamps,
            .swept_segments = {}};
}

RejectedStampCoverage event(std::uint64_t ordinal, std::initializer_list<double> coverage) {
    return {.stamp_ordinal = ordinal, .values = coverage};
}

bool non_building_uses_independent_coverage_and_strength_maxima() {
    const ResolvedStroke stroke = discrete_stroke({stamp(0, 0.4, 0.5), stamp(1, 0.5, 1.0)});
    const std::array events{event(0, {0.5}), event(1, {0.8})};
    StrokeDepositionAccumulator accumulator(stroke, 1, 1);
    accumulator.apply(events);
    const DepositionRaster& result = accumulator.result();
    return expect(result.mode == DepositionMode::non_building &&
                      near(result.non_building_coverage[0], 0.8) && near(result.strength[0], 0.4) &&
                      near(result.build_up_deposition[0], 0.0),
                  "non-building coverage and strength did not use separate maxima") &&
           expect(result.applied_stamp_count == 2 && accumulator.next_stamp_ordinal() == 2,
                  "non-building deposition did not consume each canonical stamp once");
}

bool non_building_scribble_does_not_darken_within_one_stroke() {
    const ResolvedStroke stroke =
        discrete_stroke({stamp(0, 0.5, 0.5), stamp(1, 0.5, 0.5), stamp(2, 0.5, 0.5)});
    const std::array events{event(0, {1.0}), event(1, {1.0}), event(2, {1.0})};
    StrokeDepositionAccumulator accumulator(stroke, 1, 1);
    accumulator.apply(events);
    StrokeDepositionAccumulator next_stroke(stroke, 1, 1);
    next_stroke.apply(std::span(events).first(1));
    return expect(near(accumulator.result().strength[0], 0.25),
                  "overlapping non-building stamps darkened within one stroke") &&
           expect(near(next_stroke.result().strength[0], 0.25),
                  "new stroke accumulation did not start from a fresh zero mask");
}

bool build_up_uses_flow_recurrence_and_opacity_cap() {
    const ResolvedStroke stroke =
        discrete_stroke({stamp(0, 0.8, 0.5), stamp(1, 0.2, 0.5), stamp(2, 1.0, 0.5)});
    const std::array events{event(0, {1.0}), event(1, {1.0}), event(2, {1.0})};
    StrokeDepositionAccumulator accumulator(stroke, 1, 1, DepositionMode::build_up);
    accumulator.apply(std::span(events).first(2));
    const double strength_before_high_opacity = accumulator.result().strength[0];
    accumulator.apply(std::span(events).subspan(2));
    const DepositionRaster& result = accumulator.result();
    return expect(near(strength_before_high_opacity, 0.4),
                  "lower opacity erased previously deposited build-up strength") &&
           expect(near(result.build_up_deposition[0], 0.875) && near(result.strength[0], 0.875) &&
                      near(result.non_building_coverage[0], 0.0),
                  "build-up did not use the specified flow recurrence and opacity cap");
}

bool low_flow_builds_toward_full_opacity() {
    std::vector<Stamp> stamps;
    std::vector<RejectedStampCoverage> events;
    for (std::uint64_t ordinal = 0; ordinal < 20; ++ordinal) {
        stamps.push_back(stamp(ordinal, 1.0, 0.1));
        events.push_back(event(ordinal, {1.0}));
    }
    const ResolvedStroke stroke{.reconstruction_version = canonical_stroke_reconstruction_version,
                                .tip_mode = TipMode::discrete_alpha,
                                .symmetry_instance_count = 1,
                                .stamps = stamps,
                                .swept_segments = {}};
    StrokeDepositionAccumulator accumulator(stroke, 1, 1, DepositionMode::build_up);
    accumulator.apply(events);
    const double expected = 1.0 - std::pow(0.9, 20.0);
    return expect(
        near(accumulator.result().strength[0], expected) && expected < 1.0 && expected > 0.8,
        "low-flow build-up did not approach full opacity over repeated stamps");
}

bool batches_and_repeated_raster_events_do_not_change_deposition() {
    const ResolvedStroke stroke =
        discrete_stroke({stamp(0, 1.0, 0.2), stamp(1, 1.0, 0.2), stamp(2, 1.0, 0.2)});
    const std::array events{event(0, {0.5, 1.0}), event(1, {1.0, 0.25}), event(2, {0.75, 0.5})};
    StrokeDepositionAccumulator single_batch(stroke, 2, 1, DepositionMode::build_up);
    single_batch.apply(events);
    StrokeDepositionAccumulator split_batches(stroke, 2, 1, DepositionMode::build_up);
    split_batches.apply(std::span(events).first(1));
    split_batches.apply(std::span(events).subspan(1));
    split_batches.apply(std::span(events).subspan(1, 1));
    StrokeDepositionAccumulator non_building_single(stroke, 2, 1);
    non_building_single.apply(events);
    StrokeDepositionAccumulator non_building_split(stroke, 2, 1);
    non_building_split.apply(std::span(events).first(2));
    non_building_split.apply(std::span(events).subspan(2));
    return expect(single_batch.result().build_up_deposition ==
                          split_batches.result().build_up_deposition &&
                      single_batch.result().strength == split_batches.result().strength &&
                      split_batches.result().applied_stamp_count == 3,
                  "batch boundaries or repeated rasterization changed build-up deposition") &&
           expect(non_building_single.result().non_building_coverage ==
                          non_building_split.result().non_building_coverage &&
                      non_building_single.result().strength == non_building_split.result().strength,
                  "batch boundaries changed non-building deposition");
}

bool continuous_sweeps_emit_one_deposition_event_per_stamp() {
    const ResolvedStroke stroke{
        .reconstruction_version = canonical_stroke_reconstruction_version,
        .tip_mode = TipMode::continuous_sweep,
        .symmetry_instance_count = 1,
        .stamps = {stamp(0, 1.0, 0.1), stamp(1, 1.0, 0.1), stamp(2, 1.0, 0.1)},
        .swept_segments = {{0, 1}, {1, 2}}};
    const TextureSpaceRaster surface{.width = 1,
                                     .height = 1,
                                     .tile_origin = {},
                                     .texels = {{.position = {},
                                                 .normal = {0.0, 0.0, 1.0},
                                                 .geometric_normal = {0.0, 0.0, 1.0},
                                                 .uv = {},
                                                 .triangle = 0}}};
    const auto rejected = evaluate_rejected_coverage(
        surface, stroke,
        {.depth_enabled = false,
         .depth_bias = default_depth_rejection_bias,
         .symmetry_depth_policy = SymmetryDepthPolicy::require_consistent_per_instance,
         .angle_enabled = false,
         .minimum_normal_dot = default_angle_rejection_dot,
         .backface_enabled = false});
    const auto result = evaluate_deposition(stroke, rejected, DepositionMode::build_up);
    return expect(rejected.stamp_events.size() == stroke.stamps.size() &&
                      result.applied_stamp_count == stroke.stamps.size(),
                  "continuous sweep rasterization created non-canonical deposition events") &&
           expect(near(result.build_up_deposition[0], 1.0 - std::pow(0.9, 3.0)),
                  "continuous deposition did not advance exactly once per resolved stamp");
}

bool alpha_discard_does_not_reset_build_up_deposition() {
    const ResolvedStroke stroke = discrete_stroke({stamp(0, 1.0, 0.003), stamp(1, 1.0, 0.003)});
    const std::array events{event(0, {1.0}), event(1, {1.0})};
    StrokeDepositionAccumulator accumulator(stroke, 1, 1, DepositionMode::build_up);
    accumulator.apply(std::span(events).first(1));
    const auto first_write = apply_alpha_discard(
        accumulator.result().strength,
        {.format = AlphaDiscardFormat::floating_point, .threshold = std::nullopt});
    accumulator.apply(std::span(events).subspan(1));
    const auto second_write = apply_alpha_discard(
        accumulator.result().strength,
        {.format = AlphaDiscardFormat::floating_point, .threshold = std::nullopt});
    return expect(first_write.write_mask == std::vector<std::uint8_t>({0}) &&
                      second_write.write_mask == std::vector<std::uint8_t>({1}) &&
                      near(accumulator.result().build_up_deposition[0], 1.0 - std::pow(0.997, 2.0)),
                  "alpha discard reset sub-threshold deposition before the next stamp");
}

bool invalid_or_gapped_batches_are_transactionally_refused() {
    const ResolvedStroke stroke = discrete_stroke({stamp(0), stamp(1)});
    StrokeDepositionAccumulator accumulator(stroke, 1, 1);
    const std::array gap{event(1, {1.0})};
    bool gap_refused = false;
    try {
        accumulator.apply(gap);
    } catch (const std::invalid_argument&) {
        gap_refused = true;
    }
    const std::array invalid{
        event(0, {1.0}),
        event(1, {std::numeric_limits<double>::quiet_NaN()}),
    };
    bool invalid_refused = false;
    try {
        accumulator.apply(invalid);
    } catch (const std::invalid_argument&) {
        invalid_refused = true;
    }
    const RejectedCoverageRaster incomplete{
        .coverage = {.width = 1, .height = 1, .values = {0.0}}, .stamp_events = {}, .report = {}};
    bool incomplete_refused = false;
    try {
        static_cast<void>(evaluate_deposition(stroke, incomplete));
    } catch (const std::invalid_argument&) {
        incomplete_refused = true;
    }
    return expect(gap_refused && invalid_refused && incomplete_refused &&
                      accumulator.next_stamp_ordinal() == 0 &&
                      accumulator.result().applied_stamp_count == 0 &&
                      accumulator.result().strength == std::vector<double>({0.0}),
                  "invalid deposition batch partially changed accumulator state");
}

}  // namespace

int main() {
    return non_building_uses_independent_coverage_and_strength_maxima() &&
                   non_building_scribble_does_not_darken_within_one_stroke() &&
                   build_up_uses_flow_recurrence_and_opacity_cap() &&
                   low_flow_builds_toward_full_opacity() &&
                   batches_and_repeated_raster_events_do_not_change_deposition() &&
                   continuous_sweeps_emit_one_deposition_event_per_stamp() &&
                   alpha_discard_does_not_reset_build_up_deposition() &&
                   invalid_or_gapped_batches_are_transactionally_refused()
               ? 0
               : 1;
}
