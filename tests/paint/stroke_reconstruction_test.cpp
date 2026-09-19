#include <algorithm>
#include <array>
#include <cmath>
#include <ctex/paint/stroke.hpp>
#include <exception>
#include <iostream>
#include <numbers>
#include <span>
#include <string_view>
#include <utility>
#include <vector>

namespace {

using namespace ctex::paint;

bool expect(bool condition, std::string_view message) {
    if (!condition) {
        std::cerr << message << '\n';
    }
    return condition;
}

bool near(double left, double right, double tolerance = stroke_position_tolerance) {
    return std::abs(left - right) <= tolerance;
}

StrokeInputSample sample(double x, std::uint64_t timestamp_nanoseconds) {
    return {
        .position = {x, 0.0, 0.0},
        .frame = {},
        .timestamp_nanoseconds = timestamp_nanoseconds,
        .pressure = std::nullopt,
        .tilt = {},
    };
}

StrokeInputSample sample_at(Vec3d position, std::uint64_t timestamp_nanoseconds) {
    StrokeInputSample result = sample(position.x, timestamp_nanoseconds);
    result.position = position;
    return result;
}

StrokeInputSample sample_with_input(double x, std::uint64_t timestamp_nanoseconds,
                                    std::optional<double> pressure, Vec2d tilt = {}) {
    StrokeInputSample result = sample(x, timestamp_nanoseconds);
    result.pressure = pressure;
    result.tilt = tilt;
    return result;
}

ResolvedStroke resolve(StrokeSettings settings, std::span<const StrokeInputSample> samples) {
    StrokeResolver resolver(std::move(settings));
    resolver.append_samples(samples);
    return resolver.resolve();
}

bool batching_and_redundant_samples_are_invariant() {
    StrokeSettings settings;
    settings.spacing_fraction = 0.5;
    settings.stabilizer = {.radius = 0.5, .time_constant_seconds = 0.002};
    settings.input_mapping.pressure_radius.minimum_output = 0.5;
    settings.input_mapping.tilt_elongation.enabled = true;
    settings.jitter = {.seed = 7,
                       .position_fraction = 0.1,
                       .radius_fraction = 0.1,
                       .rotation_radians = 0.1,
                       .opacity = 0.1,
                       .flow = 0.1};
    const std::array complete{
        sample_with_input(0.0, 0, 0.0),
        sample_with_input(5.0, 5'000'000, 0.5, {.x = 0.5, .y = 0.0}),
        sample_with_input(10.0, 10'000'000, 1.0, {.x = 1.0, .y = 0.0}),
    };
    const std::array minimal{complete.front(), complete.back()};
    const ResolvedStroke one_batch = resolve(settings, complete);

    StrokeResolver batched(settings);
    batched.append_samples(std::span(complete).first(1));
    batched.append_samples(std::span(complete).subspan(1, 1));
    batched.append_samples(std::span(complete).last(1));
    const ResolvedStroke three_batches = batched.resolve();
    const ResolvedStroke without_redundant_sample = resolve(settings, minimal);

    return expect(one_batch == three_batches,
                  "coalesced input batches changed canonical stroke reconstruction") &&
           expect(one_batch == without_redundant_sample,
                  "a redundant time-linear sample changed the resolved stamps");
}

bool stabilizer_follows_the_documented_timestamp_recurrence() {
    StrokeSettings settings;
    settings.radius = 2.0;
    settings.stabilizer = {.radius = 2.0, .time_constant_seconds = 0.001};
    const double expected_position = 8.0 * (1.0 - std::exp(-1.0));
    settings.spacing_fraction = expected_position / settings.radius;
    const std::array samples{sample(0.0, 0), sample(10.0, stabilization_time_step_nanoseconds)};
    const ResolvedStroke stroke = resolve(settings, samples);
    return expect(
        stroke.stamps.size() == 2 && near(stroke.stamps.back().position.x, expected_position),
        "stabilizer did not move toward the radius boundary by the timestamp factor");
}

bool a_fast_continuous_flick_emits_covering_sweeps() {
    StrokeSettings settings;
    settings.tip_mode = TipMode::continuous_sweep;
    settings.spacing_fraction = 4.0;
    const std::array samples{sample(0.0, 0), sample(10.0, stabilization_time_step_nanoseconds)};
    const ResolvedStroke stroke = resolve(settings, samples);
    return expect(stroke.stamps.size() == 4 && stroke.swept_segments.size() == 3,
                  "continuous flick omitted stamps or swept segments") &&
           expect(near(stroke.stamps[0].position.x, 0.0) &&
                      near(stroke.stamps[1].position.x, 4.0) &&
                      near(stroke.stamps[2].position.x, 8.0) &&
                      near(stroke.stamps[3].position.x, 10.0) &&
                      stroke.swept_segments[0] == SweptSegment{0, 1} &&
                      stroke.swept_segments[1] == SweptSegment{1, 2} &&
                      stroke.swept_segments[2] == SweptSegment{2, 3},
                  "continuous sweep did not connect the canonical stamp sequence");
}

bool discrete_alpha_tips_remain_separated() {
    StrokeSettings settings;
    settings.tip_mode = TipMode::discrete_alpha;
    settings.spacing_fraction = 3.0;
    const std::array samples{sample(0.0, 0), sample(6.0, stabilization_time_step_nanoseconds)};
    const ResolvedStroke stroke = resolve(settings, samples);
    return expect(stroke.stamps.size() == 3 && stroke.swept_segments.empty(),
                  "discrete-alpha mode emitted continuous swept coverage") &&
           expect(near(stroke.stamps[1].position.x - stroke.stamps[0].position.x, 3.0) &&
                      near(stroke.stamps[2].position.x - stroke.stamps[1].position.x, 3.0),
                  "discrete-alpha spacing did not preserve visible tip separation");
}

bool every_stamp_carries_the_resolved_contract() {
    StrokeSettings settings;
    settings.spacing_fraction = 1.0;
    settings.radius = 2.0;
    settings.opacity = 0.75;
    settings.hardness = 0.25;
    settings.rotation_radians = 0.5;
    settings.elongation = 1.5;
    settings.flow = 0.4;
    settings.tip_resource_identity = "brushes/chalk";
    const std::array samples{sample(0.0, 0), sample(4.0, stabilization_time_step_nanoseconds)};
    const ResolvedStroke stroke = resolve(settings, samples);
    const Stamp& last = stroke.stamps.back();
    return expect(
               stroke.reconstruction_version == canonical_stroke_reconstruction_version &&
                   last.ordinal == 2 && last.radius == settings.radius &&
                   last.opacity == settings.opacity && last.hardness == settings.hardness &&
                   last.rotation_radians == settings.rotation_radians &&
                   last.elongation == settings.elongation && last.flow == settings.flow &&
                   last.tip_resource_identity == settings.tip_resource_identity,
               "resolved stamp omitted required version, attributes, tip identity, or ordinal") &&
           expect(near(last.frame.tangent.x, 1.0) && near(last.frame.bitangent.y, 1.0) &&
                      near(last.frame.normal.z, 1.0),
                  "resolved stamp omitted its coordinate frame");
}

bool default_pen_mapping_changes_only_radius() {
    StrokeSettings settings;
    settings.radius = 10.0;
    settings.opacity = 0.8;
    settings.hardness = 0.7;
    settings.flow = 0.6;
    settings.rotation_radians = 0.4;
    const std::array samples{sample_with_input(0.0, 0, 0.5)};
    const ResolvedStroke stroke = resolve(settings, samples);
    const Stamp& stamp = stroke.stamps.front();
    return expect(near(stamp.radius, 5.05) && near(stamp.opacity, settings.opacity) &&
                      near(stamp.hardness, settings.hardness) && near(stamp.flow, settings.flow) &&
                      near(stamp.rotation_radians, settings.rotation_radians),
                  "default pen mapping changed a property other than radius");
}

bool pressure_properties_have_independent_curves_and_ranges() {
    StrokeSettings settings;
    settings.radius = 10.0;
    settings.opacity = 0.8;
    settings.hardness = 0.6;
    settings.flow = 0.5;
    settings.rotation_radians = 0.25;
    const ResponseCurve curved{{{0.0, 0.0}, {0.5, 0.25}, {1.0, 1.0}}};
    settings.input_mapping.pressure_radius = {true, curved, 0.2, 1.0};
    settings.input_mapping.pressure_opacity = {true, curved, 0.2, 1.0};
    settings.input_mapping.pressure_hardness = {true, curved, 0.4, 1.0};
    settings.input_mapping.pressure_flow = {true, curved, 0.1, 0.9};
    settings.input_mapping.pressure_rotation = {true, curved, -1.0, 1.0};
    const std::array samples{sample_with_input(0.0, 0, 0.5)};
    const ResolvedStroke stroke = resolve(settings, samples);
    const Stamp& stamp = stroke.stamps.front();
    return expect(near(stamp.radius, 4.0) && near(stamp.opacity, 0.32) &&
                      near(stamp.hardness, 0.33) && near(stamp.flow, 0.15) &&
                      near(stamp.rotation_radians, -0.25),
                  "pressure properties did not use their independent response mappings");
}

bool missing_pressure_is_full_pressure() {
    StrokeSettings settings;
    settings.input_mapping.pressure_opacity.enabled = true;
    settings.input_mapping.pressure_hardness.enabled = true;
    settings.input_mapping.pressure_flow.enabled = true;
    settings.input_mapping.pressure_rotation = {true, {}, -0.5, 0.5};
    const std::array missing{sample_with_input(0.0, 0, std::nullopt)};
    const std::array full{sample_with_input(0.0, 0, 1.0)};
    return expect(resolve(settings, missing) == resolve(settings, full),
                  "a device without pressure did not evaluate at full pressure");
}

bool tilt_maps_azimuth_and_magnitude() {
    StrokeSettings settings;
    settings.rotation_radians = 0.25;
    settings.elongation = 2.0;
    settings.input_mapping.tilt_rotation.enabled = true;
    settings.input_mapping.tilt_elongation.enabled = true;
    const std::array samples{sample_with_input(0.0, 0, std::nullopt, {.x = 0.0, .y = 0.5})};
    const ResolvedStroke stroke = resolve(settings, samples);
    const Stamp& stamp = stroke.stamps.front();
    return expect(
        near(stamp.rotation_radians, 0.25 + std::numbers::pi / 4.0) && near(stamp.elongation, 3.0),
        "tilt azimuth and magnitude did not map to rotation and elongation");
}

bool pressure_changes_are_not_discarded_as_redundant() {
    StrokeSettings settings;
    settings.spacing_fraction = 4.0;
    settings.input_mapping.pressure_radius.enabled = false;
    settings.input_mapping.pressure_opacity.enabled = true;
    const std::array varied{
        sample_with_input(0.0, 0, 0.0),
        sample_with_input(5.0, 5'000'000, 0.2),
        sample_with_input(10.0, 10'000'000, 1.0),
    };
    const std::array linear{varied.front(), varied.back()};
    return expect(resolve(settings, varied) != resolve(settings, linear),
                  "a non-linear pressure change was discarded as a redundant sample");
}

bool pressure_mapped_radius_drives_following_spacing() {
    StrokeSettings settings;
    settings.radius = 2.0;
    settings.spacing_fraction = 1.0;
    settings.input_mapping.pressure_radius.minimum_output = 0.5;
    const std::array samples{
        sample_with_input(0.0, 0, 0.0),
        sample_with_input(6.0, 6'000'000, 1.0),
    };
    const ResolvedStroke stroke = resolve(settings, samples);
    return expect(stroke.stamps.size() >= 3 && near(stroke.stamps[1].position.x, 1.0) &&
                      near(stroke.stamps[2].position.x - stroke.stamps[1].position.x,
                           stroke.stamps[1].radius),
                  "pressure-mapped radius did not determine the following stamp spacing");
}

bool jitter_is_seeded_by_stroke_and_ordinal() {
    StrokeSettings settings;
    settings.spacing_fraction = 1.0;
    settings.opacity = 0.5;
    settings.flow = 0.5;
    settings.input_mapping.pressure_radius.enabled = false;
    settings.jitter = {
        .seed = 42,
        .position_fraction = 0.5,
        .radius_fraction = 0.25,
        .rotation_radians = 0.4,
        .opacity = 0.2,
        .flow = 0.2,
    };
    const std::array samples{sample(0.0, 0), sample(3.0, 3'000'000)};
    const ResolvedStroke first = resolve(settings, samples);
    const ResolvedStroke repeated = resolve(settings, samples);
    StrokeSettings other_seed = settings;
    other_seed.jitter.seed = 43;
    const ResolvedStroke changed = resolve(other_seed, samples);

    StrokeSettings plain = settings;
    plain.jitter = {};
    const ResolvedStroke unmodified = resolve(plain, samples);
    const Stamp& jittered = first.stamps.front();
    const Stamp& base = unmodified.stamps.front();
    return expect(first == repeated, "the same jitter seed did not reproduce exact stamps") &&
           expect(first != changed, "changing the jitter seed did not change resolved stamps") &&
           expect(jittered.position != base.position && jittered.radius != base.radius &&
                      jittered.rotation_radians != base.rotation_radians &&
                      jittered.opacity != base.opacity && jittered.flow != base.flow,
                  "a configured jitter target remained unchanged");
}

bool stamp_count_taper_reaches_full_on_the_tenth_stamp() {
    StrokeSettings settings;
    settings.spacing_fraction = 1.0;
    settings.opacity = 0.8;
    settings.input_mapping.pressure_radius.enabled = false;
    settings.taper.entry = {.unit = TaperUnit::stamp_count, .extent = 10.0};
    settings.taper.floor = 0.2;
    settings.taper.affect_opacity = false;
    const std::array samples{sample(0.0, 0), sample(11.0, 11'000'000)};
    const ResolvedStroke stroke = resolve(settings, samples);
    return expect(stroke.stamps.size() == 12 && near(stroke.stamps[0].radius, 0.2) &&
                      near(stroke.stamps[8].radius, 0.2 + 0.8 * 8.0 / 9.0) &&
                      near(stroke.stamps[9].radius, 1.0),
                  "ten-stamp entry taper did not run from its floor to full radius") &&
           expect(near(stroke.stamps[0].opacity, settings.opacity),
                  "radius-only taper changed opacity");
}

bool distance_taper_applies_at_both_ends() {
    StrokeSettings settings;
    settings.spacing_fraction = 1.0;
    settings.opacity = 0.8;
    settings.input_mapping.pressure_radius.enabled = false;
    settings.taper.entry = {.unit = TaperUnit::distance, .extent = 2.0};
    settings.taper.exit = {.unit = TaperUnit::distance, .extent = 2.0};
    const std::array samples{sample(0.0, 0), sample(6.0, 6'000'000)};
    const ResolvedStroke stroke = resolve(settings, samples);
    return expect(
        near(stroke.stamps.front().radius, 0.0) && near(stroke.stamps.front().opacity, 0.0) &&
            near(stroke.stamps[1].radius, 0.5) && near(stroke.stamps[1].opacity, 0.4) &&
            near(stroke.stamps[2].radius, 1.0) && near(stroke.stamps[2].opacity, 0.8) &&
            near(stroke.stamps.back().radius, 0.0) && near(stroke.stamps.back().opacity, 0.0),
        "distance taper did not affect radius and opacity at both ends");
}

bool straight_line_constraint_ignores_intermediate_positions() {
    StrokeSettings settings;
    settings.spacing_fraction = 1.0;
    settings.input_mapping.pressure_radius.enabled = false;
    settings.constraint.mode = ConstraintMode::straight_line;
    const std::array bent{
        sample_at({0.0, 0.0, 0.0}, 0),
        sample_at({2.0, 7.0, -3.0}, 5'000'000),
        sample_at({10.0, 0.0, 0.0}, 10'000'000),
    };
    const std::array direct{bent.front(), bent.back()};
    return expect(resolve(settings, bent) == resolve(settings, direct),
                  "straight-line constraint retained an intermediate positional bend");
}

bool axis_and_grid_constraints_transform_the_path() {
    StrokeSettings axis_settings;
    axis_settings.spacing_fraction = 4.0;
    axis_settings.input_mapping.pressure_radius.enabled = false;
    axis_settings.constraint.mode = ConstraintMode::dominant_axis;
    const std::array axis_samples{
        sample_at({1.0, 2.0, 3.0}, 0),
        sample_at({8.0, 7.0, -4.0}, 5'000'000),
        sample_at({3.0, 12.0, 7.0}, 10'000'000),
    };
    const ResolvedStroke axis = resolve(axis_settings, axis_samples);
    const bool axis_locked = std::all_of(
        axis.stamps.begin(), axis.stamps.end(),
        [](const Stamp& s) { return near(s.position.x, 1.0) && near(s.position.z, 3.0); });

    StrokeSettings grid_settings;
    grid_settings.spacing_fraction = 4.0;
    grid_settings.input_mapping.pressure_radius.enabled = false;
    grid_settings.constraint = {.mode = ConstraintMode::grid, .grid_step = 1.0};
    const std::array grid_samples{
        sample_at({0.2, 0.4, -0.4}, 0),
        sample_at({2.2, 1.6, -0.6}, 2'000'000),
    };
    const ResolvedStroke grid = resolve(grid_settings, grid_samples);
    return expect(axis_locked, "dominant-axis constraint did not lock the non-dominant axes") &&
           expect(grid.stamps.front().position == Vec3d{} &&
                      grid.stamps.back().position == Vec3d{2.0, 2.0, -1.0},
                  "grid constraint did not snap reconstructed path endpoints");
}

bool spacing_contract_has_versioned_defaults_and_bounds() {
    StrokeResolver defaults;
    bool below_refused = false;
    bool above_refused = false;
    try {
        StrokeSettings below;
        below.spacing_fraction = minimum_spacing_fraction - 0.001;
        static_cast<void>(StrokeResolver(below));
    } catch (const StrokeResolutionError&) {
        below_refused = true;
    }
    try {
        StrokeSettings above;
        above.spacing_fraction = maximum_spacing_fraction + 0.001;
        static_cast<void>(StrokeResolver(above));
    } catch (const StrokeResolutionError&) {
        above_refused = true;
    }
    StrokeSettings minimum;
    minimum.spacing_fraction = minimum_spacing_fraction;
    StrokeSettings maximum;
    maximum.spacing_fraction = maximum_spacing_fraction;
    static_cast<void>(StrokeResolver(minimum));
    static_cast<void>(StrokeResolver(maximum));
    return expect(defaults.settings().reconstruction_version ==
                          canonical_stroke_reconstruction_version &&
                      defaults.settings().spacing_fraction == default_spacing_fraction,
                  "canonical reconstruction defaults changed without a version change") &&
           expect(below_refused && above_refused,
                  "spacing outside the declared inclusive bounds was accepted");
}

bool invalid_input_is_rejected_without_partial_resolution() {
    StrokeSettings future;
    future.reconstruction_version = canonical_stroke_reconstruction_version + 1;
    bool future_refused = false;
    try {
        static_cast<void>(StrokeResolver(future));
    } catch (const StrokeResolutionError&) {
        future_refused = true;
    }

    StrokeSettings invalid_mode;
    invalid_mode.tip_mode = static_cast<TipMode>(255);
    bool invalid_mode_refused = false;
    try {
        static_cast<void>(StrokeResolver(invalid_mode));
    } catch (const StrokeResolutionError&) {
        invalid_mode_refused = true;
    }

    StrokeSettings invalid_curve;
    invalid_curve.input_mapping.pressure_radius.curve.points = {{0.0, 0.0}, {0.0, 1.0}};
    bool invalid_curve_refused = false;
    try {
        static_cast<void>(StrokeResolver(invalid_curve));
    } catch (const StrokeResolutionError&) {
        invalid_curve_refused = true;
    }

    StrokeSettings invalid_jitter;
    invalid_jitter.jitter.radius_fraction = 1.0;
    bool invalid_jitter_refused = false;
    try {
        static_cast<void>(StrokeResolver(invalid_jitter));
    } catch (const StrokeResolutionError&) {
        invalid_jitter_refused = true;
    }

    StrokeSettings invalid_taper;
    invalid_taper.taper.entry = {.unit = TaperUnit::stamp_count, .extent = 1.0};
    bool invalid_taper_refused = false;
    try {
        static_cast<void>(StrokeResolver(invalid_taper));
    } catch (const StrokeResolutionError&) {
        invalid_taper_refused = true;
    }

    StrokeSettings invalid_constraint;
    invalid_constraint.constraint.grid_step = 0.0;
    bool invalid_constraint_refused = false;
    try {
        static_cast<void>(StrokeResolver(invalid_constraint));
    } catch (const StrokeResolutionError&) {
        invalid_constraint_refused = true;
    }

    StrokeResolver resolver;
    const std::array reversed{sample(0.0, 2), sample(1.0, 1)};
    bool timestamps_refused = false;
    try {
        resolver.append_samples(reversed);
    } catch (const StrokeResolutionError&) {
        timestamps_refused = true;
    }
    const std::array invalid_pressure{sample_with_input(0.0, 0, 1.1)};
    bool pressure_refused = false;
    try {
        resolver.append_samples(invalid_pressure);
    } catch (const StrokeResolutionError&) {
        pressure_refused = true;
    }
    const std::array invalid_tilt{sample_with_input(0.0, 0, std::nullopt, {.x = 1.0, .y = 1.0})};
    bool tilt_refused = false;
    try {
        resolver.append_samples(invalid_tilt);
    } catch (const StrokeResolutionError&) {
        tilt_refused = true;
    }
    const std::array valid{sample(0.0, 0)};
    resolver.append_samples(valid);
    static_cast<void>(resolver.resolve());
    bool second_resolution_refused = false;
    try {
        static_cast<void>(resolver.resolve());
    } catch (const std::logic_error&) {
        second_resolution_refused = true;
    }
    return expect(future_refused, "an unknown reconstruction version was accepted") &&
           expect(invalid_mode_refused, "an unknown tip mode was accepted") &&
           expect(invalid_curve_refused, "an invalid response curve was accepted") &&
           expect(invalid_jitter_refused, "invalid jitter was accepted") &&
           expect(invalid_taper_refused, "invalid taper was accepted") &&
           expect(invalid_constraint_refused, "invalid constraint settings were accepted") &&
           expect(pressure_refused, "an out-of-range pressure value was accepted") &&
           expect(tilt_refused, "an out-of-range tilt vector was accepted") &&
           expect(timestamps_refused && resolver.sample_count() == 1,
                  "non-monotonic timestamps partially mutated the resolver") &&
           expect(second_resolution_refused && resolver.is_resolved(),
                  "one stroke resolver produced more than one canonical sequence");
}

}  // namespace

int main() {
    return batching_and_redundant_samples_are_invariant() &&
                   stabilizer_follows_the_documented_timestamp_recurrence() &&
                   a_fast_continuous_flick_emits_covering_sweeps() &&
                   discrete_alpha_tips_remain_separated() &&
                   every_stamp_carries_the_resolved_contract() &&
                   default_pen_mapping_changes_only_radius() &&
                   pressure_properties_have_independent_curves_and_ranges() &&
                   missing_pressure_is_full_pressure() && tilt_maps_azimuth_and_magnitude() &&
                   pressure_changes_are_not_discarded_as_redundant() &&
                   pressure_mapped_radius_drives_following_spacing() &&
                   jitter_is_seeded_by_stroke_and_ordinal() &&
                   stamp_count_taper_reaches_full_on_the_tenth_stamp() &&
                   distance_taper_applies_at_both_ends() &&
                   straight_line_constraint_ignores_intermediate_positions() &&
                   axis_and_grid_constraints_transform_the_path() &&
                   spacing_contract_has_versioned_defaults_and_bounds() &&
                   invalid_input_is_rejected_without_partial_resolution()
               ? 0
               : 1;
}
