#include <array>
#include <cmath>
#include <ctex/paint/stroke.hpp>
#include <exception>
#include <iostream>
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
    };
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
    const std::array complete{
        sample(0.0, 0),
        sample(5.0, 5'000'000),
        sample(10.0, 10'000'000),
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

    StrokeResolver resolver;
    const std::array reversed{sample(0.0, 2), sample(1.0, 1)};
    bool timestamps_refused = false;
    try {
        resolver.append_samples(reversed);
    } catch (const StrokeResolutionError&) {
        timestamps_refused = true;
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
                   spacing_contract_has_versioned_defaults_and_bounds() &&
                   invalid_input_is_rejected_without_partial_resolution()
               ? 0
               : 1;
}
