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

Stamp host_stamp(double x, std::uint64_t source_ordinal, std::uint64_t symmetry_instance,
                 std::uint64_t ordinal) {
    return {
        .position = {x, 2.0, -3.0},
        .frame = {},
        .radius = 2.5,
        .opacity = 0.35,
        .hardness = 0.65,
        .rotation_radians = 0.75,
        .elongation = 1.25,
        .flow = 0.45,
        .tip_resource_identity = "host.custom-tip",
        .source_ordinal = source_ordinal,
        .symmetry_instance = symmetry_instance,
        .ordinal = ordinal,
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

bool three_plane_symmetry_emits_eight_transformed_frames() {
    StrokeSettings settings;
    settings.symmetry = {
        .mirror_x = true,
        .mirror_y = true,
        .mirror_z = true,
        .radial_count = 1,
        .radial_axis = SymmetryAxis::z,
    };
    const std::array samples{sample_at({1.0, 2.0, 3.0}, 0)};
    const ResolvedStroke stroke = resolve(settings, samples);
    bool transforms_match = stroke.stamps.size() == 8;
    for (std::uint64_t mask = 0; transforms_match && mask < 8; ++mask) {
        const Stamp& stamp = stroke.stamps[mask];
        const Vec3d expected{
            (mask & 1U) != 0U ? -1.0 : 1.0,
            (mask & 2U) != 0U ? -2.0 : 2.0,
            (mask & 4U) != 0U ? -3.0 : 3.0,
        };
        transforms_match = stamp.position == expected && stamp.source_ordinal == 0 &&
                           stamp.symmetry_instance == mask && stamp.ordinal == mask;
    }
    const Stamp& x_mirror = stroke.stamps[1];
    return expect(stroke.symmetry_instance_count == 8 && stroke.swept_segments.empty() &&
                      transforms_match,
                  "three mirror planes did not emit all eight stable instances") &&
           expect(x_mirror.frame.tangent == Vec3d{-1.0, 0.0, 0.0} &&
                      x_mirror.frame.bitangent == Vec3d{0.0, 1.0, 0.0} &&
                      x_mirror.frame.normal == Vec3d{0.0, 0.0, 1.0},
                  "mirror symmetry did not transform the complete coordinate frame");
}

bool radial_symmetry_supports_each_object_axis() {
    StrokeSettings z_settings;
    z_settings.symmetry.radial_count = 6;
    const std::array z_samples{sample_at({1.0, 0.0, 0.0}, 0)};
    const ResolvedStroke around_z = resolve(z_settings, z_samples);
    bool evenly_rotated = around_z.stamps.size() == 6;
    for (std::size_t index = 0; evenly_rotated && index < around_z.stamps.size(); ++index) {
        const double angle = 2.0 * std::numbers::pi * static_cast<double>(index) / 6.0;
        evenly_rotated = near(around_z.stamps[index].position.x, std::cos(angle)) &&
                         near(around_z.stamps[index].position.y, std::sin(angle)) &&
                         near(around_z.stamps[index].position.z, 0.0);
    }

    StrokeSettings x_settings;
    x_settings.symmetry = {.radial_count = 4, .radial_axis = SymmetryAxis::x};
    const std::array x_samples{sample_at({0.0, 1.0, 0.0}, 0)};
    const ResolvedStroke around_x = resolve(x_settings, x_samples);

    StrokeSettings y_settings;
    y_settings.symmetry = {.radial_count = 4, .radial_axis = SymmetryAxis::y};
    const std::array y_samples{sample_at({1.0, 0.0, 0.0}, 0)};
    const ResolvedStroke around_y = resolve(y_settings, y_samples);
    return expect(around_z.symmetry_instance_count == 6 && evenly_rotated,
                  "six-fold radial symmetry was not evenly distributed around Z") &&
           expect(near(around_x.stamps[1].position.z, 1.0) &&
                      near(around_y.stamps[1].position.z, -1.0),
                  "radial symmetry did not honor the selected X or Y object axis");
}

bool symmetry_sweeps_never_connect_instances() {
    StrokeSettings settings;
    settings.spacing_fraction = 1.0;
    settings.input_mapping.pressure_radius.enabled = false;
    settings.symmetry.mirror_x = true;
    const std::array samples{sample(1.0, 0), sample(3.0, 2'000'000)};
    const ResolvedStroke stroke = resolve(settings, samples);
    const ResolvedStroke externally_ingested = ingest_resolved_stroke(stroke);
    return expect(stroke.symmetry_instance_count == 2 && stroke.stamps.size() == 6 &&
                      stroke.swept_segments.size() == 4,
                  "mirror expansion emitted the wrong stamp or sweep count") &&
           expect(externally_ingested == stroke,
                  "valid multi-instance external stroke was not preserved") &&
           expect(stroke.swept_segments[0] == SweptSegment{0, 1} &&
                      stroke.swept_segments[1] == SweptSegment{1, 2} &&
                      stroke.swept_segments[2] == SweptSegment{3, 4} &&
                      stroke.swept_segments[3] == SweptSegment{4, 5},
                  "a symmetry sweep connected two separate instances") &&
           expect(stroke.stamps[3].position == Vec3d{-1.0, 0.0, 0.0} &&
                      stroke.stamps[3].source_ordinal == 0 && stroke.stamps[5].source_ordinal == 2,
                  "symmetry copies lost their transformed position or source ordinal");
}

bool mirror_and_radial_symmetry_form_a_cartesian_product() {
    StrokeSettings settings;
    settings.symmetry = {
        .mirror_x = true,
        .mirror_y = true,
        .mirror_z = false,
        .radial_count = 3,
        .radial_axis = SymmetryAxis::z,
    };
    const std::array samples{sample_at({1.0, 2.0, 0.0}, 0)};
    const ResolvedStroke stroke = resolve(settings, samples);
    return expect(stroke.symmetry_instance_count == 12 && stroke.stamps.size() == 12,
                  "mirror and radial symmetry did not form their full Cartesian product") &&
           expect(stroke.stamps.front().position == Vec3d{1.0, 2.0, 0.0} &&
                      stroke.stamps.back().symmetry_instance == 11 &&
                      stroke.stamps.back().ordinal == 11,
                  "combined symmetry ordering or identity instance was unstable");
}

bool external_resolved_strokes_bypass_every_resolver_modifier() {
    const ResolvedStroke host{
        .reconstruction_version = canonical_stroke_reconstruction_version,
        .tip_mode = TipMode::continuous_sweep,
        .symmetry_instance_count = 1,
        .stamps = {host_stamp(0.0, 0, 0, 0), host_stamp(100.0, 1, 0, 1)},
        .swept_segments = {{0, 1}},
    };
    const ResolvedStroke ingested = ingest_resolved_stroke(host);
    return expect(ingested == host,
                  "external resolved stamps were reinterpreted during ingestion") &&
           expect(ingested.stamps.size() == 2 && ingested.stamps[1].position.x == 100.0 &&
                      ingested.stamps[0].radius == 2.5 && ingested.stamps[0].opacity == 0.35,
                  "external ingestion applied spacing, mapping, taper, jitter, or constraints");
}

bool external_discrete_stamps_preserve_visible_separation() {
    const ResolvedStroke host{
        .reconstruction_version = canonical_stroke_reconstruction_version,
        .tip_mode = TipMode::discrete_alpha,
        .symmetry_instance_count = 1,
        .stamps = {host_stamp(-20.0, 0, 0, 0), host_stamp(20.0, 1, 0, 1)},
        .swept_segments = {},
    };
    const ResolvedStroke ingested = ingest_resolved_stroke(host);
    return expect(ingested == host && ingested.swept_segments.empty(),
                  "external discrete-alpha stamps gained implicit coverage");
}

bool malformed_external_strokes_are_rejected_without_mutation() {
    const ResolvedStroke valid{
        .reconstruction_version = canonical_stroke_reconstruction_version,
        .tip_mode = TipMode::continuous_sweep,
        .symmetry_instance_count = 1,
        .stamps = {host_stamp(0.0, 0, 0, 0), host_stamp(1.0, 1, 0, 1)},
        .swept_segments = {{0, 1}},
    };
    const auto rejected = [](const ResolvedStroke& stroke) {
        try {
            static_cast<void>(ingest_resolved_stroke(stroke));
        } catch (const StrokeResolutionError&) {
            return true;
        }
        return false;
    };

    ResolvedStroke wrong_ordinal = valid;
    wrong_ordinal.stamps[1].ordinal = 7;
    ResolvedStroke invalid_frame = valid;
    invalid_frame.stamps[0].frame.tangent = {2.0, 0.0, 0.0};
    ResolvedStroke missing_sweep = valid;
    missing_sweep.swept_segments.clear();
    ResolvedStroke future = valid;
    future.reconstruction_version = canonical_stroke_reconstruction_version + 1;
    ResolvedStroke discrete_with_sweep = valid;
    discrete_with_sweep.tip_mode = TipMode::discrete_alpha;

    return expect(rejected(wrong_ordinal) && rejected(invalid_frame) && rejected(missing_sweep) &&
                      rejected(future) && rejected(discrete_with_sweep),
                  "a malformed external resolved stroke was accepted") &&
           expect(valid.stamps[1].ordinal == 1 &&
                      valid.swept_segments == std::vector{SweptSegment{0, 1}},
                  "failed external ingestion mutated the caller's stroke");
}

bool spacing_contract_has_versioned_defaults_and_bounds() {
    StrokeResolver defaults;
    StrokeSettings below_settings;
    below_settings.spacing_fraction = minimum_spacing_fraction - 0.001;
    const StrokeResolver below(below_settings);
    StrokeSettings above_settings;
    above_settings.spacing_fraction = maximum_spacing_fraction + 0.001;
    const StrokeResolver above(above_settings);
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
           expect(below.settings().spacing_fraction == minimum_spacing_fraction &&
                      above.settings().spacing_fraction == maximum_spacing_fraction &&
                      below.parameter_report().clamp_for("stroke.spacing_fraction").has_value() &&
                      above.parameter_report().clamp_for("stroke.spacing_fraction").has_value(),
                  "spacing outside the declared bounds was not clamped and reported");
}

bool radius_uses_shared_bounds_and_reports_clamps() {
    const StrokeInputSample input = sample(0.0, 0);
    StrokeSettings above;
    above.radius = maximum_stroke_radius * 2.0;
    StrokeResolver maximum(above);
    const auto upper_clamp = maximum.parameter_report().clamp_for("stroke.radius");
    maximum.append_samples({&input, 1});
    const ResolvedStroke maximum_stroke = maximum.resolve();

    StrokeSettings below;
    below.radius = 0.0;
    StrokeResolver minimum(below);
    const auto lower_clamp = minimum.parameter_report().clamp_for("stroke.radius");
    minimum.append_samples({&input, 1});
    const ResolvedStroke minimum_stroke = minimum.resolve();

    StrokeResolver unchanged;
    bool non_finite_refused = false;
    try {
        StrokeSettings invalid;
        invalid.radius = std::numeric_limits<double>::infinity();
        static_cast<void>(StrokeResolver(invalid));
    } catch (const StrokeResolutionError&) {
        non_finite_refused = true;
    }
    return expect(upper_clamp == ToolParameterClamp{.name = "stroke.radius",
                                                    .supplied = maximum_stroke_radius * 2.0,
                                                    .resolved = maximum_stroke_radius} &&
                      maximum.settings().radius == maximum_stroke_radius &&
                      maximum_stroke.stamps[0].radius == maximum_stroke_radius,
                  "radius above its maximum was not clamped, reported, and used") &&
           expect(lower_clamp == ToolParameterClamp{.name = "stroke.radius",
                                                    .supplied = 0.0,
                                                    .resolved = minimum_stroke_radius} &&
                      minimum_stroke.stamps[0].radius == minimum_stroke_radius,
                  "radius below its minimum was not clamped, reported, and used") &&
           expect(unchanged.parameter_report().clamps.empty(),
                  "an in-range default radius was reported as clamped") &&
           expect(non_finite_refused, "a non-finite radius was accepted or clamped");
}

bool base_parameters_share_bounds_and_reach_stamps() {
    const StrokeInputSample input = sample(0.0, 0);
    StrokeSettings settings;
    settings.opacity = 2.0;
    settings.hardness = -1.0;
    settings.rotation_radians = 10.0;
    settings.elongation = 0.0;
    settings.flow = -1.0;
    settings.stabilizer = {.radius = -1.0, .time_constant_seconds = 100.0};
    StrokeResolver resolver(settings);
    resolver.append_samples({&input, 1});
    const ResolvedStroke stroke = resolver.resolve();
    const Stamp& stamp = stroke.stamps.front();
    const ToolParameterReport& report = resolver.parameter_report();
    return expect(stamp.opacity == 1.0 && stamp.hardness == 0.0 &&
                      stamp.rotation_radians == maximum_stroke_rotation_radians &&
                      stamp.elongation == 0.01 && stamp.flow == 0.0,
                  "resolved stamps did not use clamped base parameters") &&
           expect(resolver.settings().stabilizer.radius == 0.0 &&
                      resolver.settings().stabilizer.time_constant_seconds == 60.0,
                  "stabilizer controls did not use their shared bounds") &&
           expect(report.clamps.size() == 7 && report.clamp_for("stroke.opacity").has_value() &&
                      report.clamp_for("stroke.hardness").has_value() &&
                      report.clamp_for("stroke.rotation_radians").has_value() &&
                      report.clamp_for("stroke.elongation").has_value() &&
                      report.clamp_for("stroke.flow").has_value() &&
                      report.clamp_for("stroke.stabilizer.radius").has_value() &&
                      report.clamp_for("stroke.stabilizer.time_constant_seconds").has_value(),
                  "base-parameter clamp report is incomplete");
}

bool modifier_parameters_share_bounds_and_reports() {
    StrokeSettings settings;
    settings.jitter = {.seed = 17,
                       .position_fraction = 5.0,
                       .radius_fraction = 2.0,
                       .rotation_radians = 10.0,
                       .opacity = 2.0,
                       .flow = 2.0};
    settings.taper.floor = 2.0;
    settings.constraint.grid_step = 0.0;
    settings.symmetry.radial_count = 0;
    const StrokeResolver resolver(settings);
    const StrokeSettings& resolved = resolver.settings();
    const ToolParameterReport& report = resolver.parameter_report();
    return expect(resolved.jitter.position_fraction == 4.0 &&
                      resolved.jitter.radius_fraction == 0.99 &&
                      resolved.jitter.rotation_radians == maximum_stroke_rotation_radians &&
                      resolved.jitter.opacity == 1.0 && resolved.jitter.flow == 1.0 &&
                      resolved.taper.floor == 1.0 &&
                      resolved.constraint.grid_step == stroke_position_tolerance &&
                      resolved.symmetry.radial_count == 1,
                  "stroke modifiers did not use their shared bounds") &&
           expect(report.clamps.size() == 8 &&
                      report.clamp_for("stroke.jitter.position_fraction").has_value() &&
                      report.clamp_for("stroke.jitter.radius_fraction").has_value() &&
                      report.clamp_for("stroke.jitter.rotation_radians").has_value() &&
                      report.clamp_for("stroke.jitter.opacity").has_value() &&
                      report.clamp_for("stroke.jitter.flow").has_value() &&
                      report.clamp_for("stroke.taper.floor").has_value() &&
                      report.clamp_for("stroke.constraint.grid_step").has_value() &&
                      report.clamp_for("stroke.symmetry.radial_count").has_value(),
                  "stroke-modifier clamp report is incomplete");
}

bool response_mapping_parameters_share_bounds_and_reports() {
    StrokeSettings settings;
    settings.input_mapping.pressure_radius = {true, {}, 0.0, 200.0};
    settings.input_mapping.pressure_opacity = {true, {}, -1.0, 2.0};
    settings.input_mapping.pressure_hardness = {true, {}, -1.0, 2.0};
    settings.input_mapping.pressure_flow = {true, {}, -1.0, 2.0};
    settings.input_mapping.pressure_rotation = {true, {}, -10.0, 10.0};
    settings.input_mapping.tilt_rotation = {true, {}, -1.0, 2.0};
    settings.input_mapping.tilt_elongation = {true, {}, 0.0, 200.0};
    const StrokeResolver resolver(settings);
    const StrokeInputMapping& resolved = resolver.settings().input_mapping;
    const ToolParameterReport& report = resolver.parameter_report();
    return expect(
               resolved.pressure_radius.minimum_output == 0.01 &&
                   resolved.pressure_radius.maximum_output == 100.0 &&
                   resolved.pressure_opacity.minimum_output == 0.0 &&
                   resolved.pressure_opacity.maximum_output == 1.0 &&
                   resolved.pressure_hardness.minimum_output == 0.0 &&
                   resolved.pressure_hardness.maximum_output == 1.0 &&
                   resolved.pressure_flow.minimum_output == 0.0 &&
                   resolved.pressure_flow.maximum_output == 1.0 &&
                   resolved.pressure_rotation.minimum_output == -maximum_stroke_rotation_radians &&
                   resolved.pressure_rotation.maximum_output == maximum_stroke_rotation_radians &&
                   resolved.tilt_rotation.minimum_output == 0.0 &&
                   resolved.tilt_rotation.maximum_output == 1.0 &&
                   resolved.tilt_elongation.minimum_output == 0.01 &&
                   resolved.tilt_elongation.maximum_output == 100.0,
               "response mapping endpoints did not use their shared bounds") &&
           expect(report.clamps.size() == 14 &&
                      report.clamp_for("stroke.input.pressure_radius.minimum_output") &&
                      report.clamp_for("stroke.input.pressure_radius.maximum_output") &&
                      report.clamp_for("stroke.input.pressure_opacity.minimum_output") &&
                      report.clamp_for("stroke.input.pressure_opacity.maximum_output") &&
                      report.clamp_for("stroke.input.pressure_hardness.minimum_output") &&
                      report.clamp_for("stroke.input.pressure_hardness.maximum_output") &&
                      report.clamp_for("stroke.input.pressure_flow.minimum_output") &&
                      report.clamp_for("stroke.input.pressure_flow.maximum_output") &&
                      report.clamp_for("stroke.input.pressure_rotation.minimum_output") &&
                      report.clamp_for("stroke.input.pressure_rotation.maximum_output") &&
                      report.clamp_for("stroke.input.tilt_rotation.minimum_output") &&
                      report.clamp_for("stroke.input.tilt_rotation.maximum_output") &&
                      report.clamp_for("stroke.input.tilt_elongation.minimum_output") &&
                      report.clamp_for("stroke.input.tilt_elongation.maximum_output"),
                  "response mapping clamp report is incomplete");
}

bool taper_extents_use_mode_specific_bounds_and_reports() {
    StrokeSettings active;
    active.taper.entry = {.unit = TaperUnit::stamp_count, .extent = 1.0};
    active.taper.exit = {.unit = TaperUnit::distance, .extent = maximum_taper_distance * 2.0};
    const StrokeResolver active_resolver(active);
    const TaperSettings& resolved = active_resolver.settings().taper;
    const ToolParameterReport& active_report = active_resolver.parameter_report();

    StrokeSettings disabled;
    disabled.taper.entry = {.unit = TaperUnit::none, .extent = 5.0};
    const StrokeResolver disabled_resolver(disabled);
    const auto disabled_clamp =
        disabled_resolver.parameter_report().clamp_for("stroke.taper.entry.extent");
    return expect(resolved.entry.extent == minimum_taper_stamp_count &&
                      resolved.exit.extent == maximum_taper_distance,
                  "active taper extents did not use their mode-specific bounds") &&
           expect(active_report.clamps.size() == 2 &&
                      active_report.clamp_for("stroke.taper.entry.extent") &&
                      active_report.clamp_for("stroke.taper.exit.extent"),
                  "active taper extent clamps were not reported") &&
           expect(disabled_resolver.settings().taper.entry.extent == 0.0 &&
                      disabled_clamp == ToolParameterClamp{.name = "stroke.taper.entry.extent",
                                                           .supplied = 5.0,
                                                           .resolved = 0.0},
                  "a disabled taper retained or failed to report an inert extent");
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

    StrokeSettings reversed_mapping;
    reversed_mapping.input_mapping.pressure_opacity.minimum_output = 0.8;
    reversed_mapping.input_mapping.pressure_opacity.maximum_output = 0.2;
    bool reversed_mapping_refused = false;
    try {
        static_cast<void>(StrokeResolver(reversed_mapping));
    } catch (const StrokeResolutionError&) {
        reversed_mapping_refused = true;
    }

    StrokeSettings non_finite_mapping;
    non_finite_mapping.input_mapping.pressure_flow.maximum_output =
        std::numeric_limits<double>::infinity();
    bool non_finite_mapping_refused = false;
    try {
        static_cast<void>(StrokeResolver(non_finite_mapping));
    } catch (const StrokeResolutionError&) {
        non_finite_mapping_refused = true;
    }

    StrokeSettings invalid_jitter;
    invalid_jitter.jitter.radius_fraction = std::numeric_limits<double>::infinity();
    bool invalid_jitter_refused = false;
    try {
        static_cast<void>(StrokeResolver(invalid_jitter));
    } catch (const StrokeResolutionError&) {
        invalid_jitter_refused = true;
    }

    StrokeSettings invalid_taper;
    invalid_taper.taper.entry = {.unit = TaperUnit::stamp_count, .extent = 2.5};
    bool invalid_taper_refused = false;
    try {
        static_cast<void>(StrokeResolver(invalid_taper));
    } catch (const StrokeResolutionError&) {
        invalid_taper_refused = true;
    }

    StrokeSettings invalid_taper_unit;
    invalid_taper_unit.taper.exit.unit = static_cast<TaperUnit>(255);
    bool invalid_taper_unit_refused = false;
    try {
        static_cast<void>(StrokeResolver(invalid_taper_unit));
    } catch (const StrokeResolutionError&) {
        invalid_taper_unit_refused = true;
    }

    StrokeSettings invalid_constraint;
    invalid_constraint.constraint.grid_step = std::numeric_limits<double>::quiet_NaN();
    bool invalid_constraint_refused = false;
    try {
        static_cast<void>(StrokeResolver(invalid_constraint));
    } catch (const StrokeResolutionError&) {
        invalid_constraint_refused = true;
    }

    StrokeSettings invalid_symmetry_axis;
    invalid_symmetry_axis.symmetry.radial_axis = static_cast<SymmetryAxis>(255);
    bool invalid_symmetry_axis_refused = false;
    try {
        static_cast<void>(StrokeResolver(invalid_symmetry_axis));
    } catch (const StrokeResolutionError&) {
        invalid_symmetry_axis_refused = true;
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
           expect(reversed_mapping_refused, "a reversed response mapping was accepted") &&
           expect(non_finite_mapping_refused, "a non-finite response mapping was accepted") &&
           expect(invalid_jitter_refused, "invalid jitter was accepted") &&
           expect(invalid_taper_refused, "invalid taper was accepted") &&
           expect(invalid_taper_unit_refused, "an unknown taper unit was accepted") &&
           expect(invalid_constraint_refused, "invalid constraint settings were accepted") &&
           expect(invalid_symmetry_axis_refused, "an invalid radial symmetry axis was accepted") &&
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
                   three_plane_symmetry_emits_eight_transformed_frames() &&
                   radial_symmetry_supports_each_object_axis() &&
                   symmetry_sweeps_never_connect_instances() &&
                   mirror_and_radial_symmetry_form_a_cartesian_product() &&
                   external_resolved_strokes_bypass_every_resolver_modifier() &&
                   external_discrete_stamps_preserve_visible_separation() &&
                   malformed_external_strokes_are_rejected_without_mutation() &&
                   spacing_contract_has_versioned_defaults_and_bounds() &&
                   radius_uses_shared_bounds_and_reports_clamps() &&
                   base_parameters_share_bounds_and_reach_stamps() &&
                   modifier_parameters_share_bounds_and_reports() &&
                   response_mapping_parameters_share_bounds_and_reports() &&
                   taper_extents_use_mode_specific_bounds_and_reports() &&
                   invalid_input_is_rejected_without_partial_resolution()
               ? 0
               : 1;
}
