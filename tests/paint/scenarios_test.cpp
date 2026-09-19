#include <array>
#include <ctex/paint/coverage.hpp>
#include <ctex/paint/deposition.hpp>
#include <ctex/paint/rejection.hpp>
#include <ctex/paint/stroke.hpp>
#include <iostream>
#include <span>
#include <string_view>

namespace {

using namespace ctex::paint;

bool expect(bool condition, std::string_view message) {
    if (!condition) {
        std::cerr << message << '\n';
    }
    return condition;
}

StrokeInputSample sample(double x, std::uint64_t timestamp_nanoseconds, double pressure) {
    return {.position = {x, 0.0, 0.0},
            .frame = {},
            .timestamp_nanoseconds = timestamp_nanoseconds,
            .pressure = pressure,
            .tilt = {}};
}

ResolvedStroke resolve_in_one_batch(const StrokeSettings& settings,
                                    std::span<const StrokeInputSample> samples) {
    StrokeResolver resolver(settings);
    resolver.append_samples(samples);
    return resolver.resolve();
}

ResolvedStroke resolve_one_sample_at_a_time(const StrokeSettings& settings,
                                            std::span<const StrokeInputSample> samples) {
    StrokeResolver resolver(settings);
    for (const StrokeInputSample& input : samples) {
        resolver.append_samples(std::span(&input, 1));
    }
    return resolver.resolve();
}

TextureSpaceRaster line_surface() {
    TextureSpaceRaster surface{.width = 5, .height = 1, .tile_origin = {}, .texels = {}};
    for (std::uint32_t x = 0; x < surface.width; ++x) {
        surface.texels.push_back({.position = {static_cast<double>(x), 0.0, 0.0},
                                  .normal = {0.0, 0.0, 1.0},
                                  .geometric_normal = {0.0, 0.0, 1.0},
                                  .uv = {static_cast<double>(x) / 4.0, 0.5},
                                  .triangle = 0});
    }
    return surface;
}

DepositionRaster deposit(const TextureSpaceRaster& surface, const ResolvedStroke& stroke) {
    const RejectionSettings rejection{
        .depth_enabled = false,
        .depth_bias = default_depth_rejection_bias,
        .symmetry_depth_policy = SymmetryDepthPolicy::require_consistent_per_instance,
        .angle_enabled = false,
        .minimum_normal_dot = default_angle_rejection_dot,
        .backface_enabled = false};
    return evaluate_deposition(stroke, evaluate_rejected_coverage(surface, stroke, rejection),
                               DepositionMode::build_up);
}

bool batching_preserves_stamps_and_deposition() {
    StrokeSettings settings;
    settings.radius = 1.0;
    settings.spacing_fraction = 0.5;
    settings.flow = 0.2;
    settings.stabilizer = {.radius = 0.25, .time_constant_seconds = 0.001};
    const std::array samples{
        sample(0.0, 0, 0.4),
        sample(2.0, 2'000'000, 0.7),
        sample(4.0, 4'000'000, 1.0),
    };

    const ResolvedStroke coalesced = resolve_in_one_batch(settings, samples);
    const ResolvedStroke incremental = resolve_one_sample_at_a_time(settings, samples);
    const TextureSpaceRaster surface = line_surface();
    const DepositionRaster coalesced_deposition = deposit(surface, coalesced);
    const DepositionRaster incremental_deposition = deposit(surface, incremental);

    return expect(coalesced == incremental,
                  "input batching changed the committed canonical stamp sequence") &&
           expect(coalesced_deposition.strength == incremental_deposition.strength &&
                      coalesced_deposition.build_up_deposition ==
                          incremental_deposition.build_up_deposition &&
                      coalesced_deposition.applied_stamp_count ==
                          incremental_deposition.applied_stamp_count,
                  "input batching changed committed deposition");
}

bool one_resolved_sequence_drives_paint_consumers() {
    StrokeSettings settings;
    settings.radius = 1.0;
    settings.spacing_fraction = 1.0;
    const std::array samples{sample(0.0, 0, 1.0), sample(4.0, 4'000'000, 1.0)};
    const ResolvedStroke stroke = resolve_in_one_batch(settings, samples);
    const TextureSpaceRaster surface = line_surface();
    const CoverageRaster coverage = evaluate_stroke_coverage(surface, stroke);
    const RejectionSettings rejection{
        .depth_enabled = false,
        .depth_bias = default_depth_rejection_bias,
        .symmetry_depth_policy = SymmetryDepthPolicy::require_consistent_per_instance,
        .angle_enabled = false,
        .minimum_normal_dot = default_angle_rejection_dot,
        .backface_enabled = false};
    const RejectedCoverageRaster accepted = evaluate_rejected_coverage(surface, stroke, rejection);
    const DepositionRaster deposition = evaluate_deposition(stroke, accepted);

    return expect(!stroke.stamps.empty() && coverage.values.size() == surface.texels.size(),
                  "coverage did not consume the resolved stroke") &&
           expect(accepted.stamp_events.size() == stroke.stamps.size(),
                  "rejection did not retain one event per resolved stamp") &&
           expect(deposition.applied_stamp_count == stroke.stamps.size() &&
                      deposition.strength.size() == coverage.values.size(),
                  "deposition did not consume the same resolved stamp sequence");
}

}  // namespace

int main() {
    return batching_preserves_stamps_and_deposition() &&
                   one_resolved_sequence_drives_paint_consumers()
               ? 0
               : 1;
}
