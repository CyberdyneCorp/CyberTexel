#include <algorithm>
#include <cmath>
#include <ctex/paint/rejection.hpp>
#include <limits>
#include <stdexcept>
#include <string>

#include "coverage_detail.hpp"

namespace ctex::paint {
namespace {

constexpr double vector_epsilon = 1.0e-12;

bool finite(double value) { return std::isfinite(value); }

bool finite(Vec2d value) { return finite(value.x) && finite(value.y); }

bool finite(Vec3d value) { return finite(value.x) && finite(value.y) && finite(value.z); }

double dot(Vec3d left, Vec3d right) {
    return left.x * right.x + left.y * right.y + left.z * right.z;
}

double length(Vec3d value) { return std::sqrt(dot(value, value)); }

Vec3d normalized(Vec3d value, std::string_view role) {
    const double magnitude = length(value);
    if (!finite(value) || magnitude <= vector_epsilon) {
        throw std::invalid_argument(std::string(role) + " must be finite and non-zero");
    }
    return {value.x / magnitude, value.y / magnitude, value.z / magnitude};
}

std::size_t checked_pixel_count(std::uint32_t width, std::uint32_t height) {
    if (width == 0 || height == 0 ||
        static_cast<std::size_t>(width) >
            std::numeric_limits<std::size_t>::max() / static_cast<std::size_t>(height)) {
        throw std::invalid_argument("depth viewport dimensions are invalid");
    }
    return static_cast<std::size_t>(width) * height;
}

DepthRejectionDisposition depth_disposition(const ResolvedStroke& stroke,
                                            const RejectionSettings& settings) {
    if (!settings.depth_enabled) {
        return DepthRejectionDisposition::disabled_by_operation;
    }
    if (stroke.symmetry_instance_count > 1 &&
        settings.symmetry_depth_policy == SymmetryDepthPolicy::disable_for_derived_symmetry) {
        return DepthRejectionDisposition::disabled_for_derived_symmetry;
    }
    return DepthRejectionDisposition::consistent_per_instance;
}

bool depth_required(std::uint64_t instance, DepthRejectionDisposition disposition) {
    return disposition == DepthRejectionDisposition::consistent_per_instance || instance == 0;
}

using DepthContextIndex = std::vector<const DepthProjectionContext*>;

DepthContextIndex validate_depth_contexts(const TextureSpaceRaster& surface,
                                          const ResolvedStroke& stroke,
                                          const RejectionSettings& settings,
                                          DepthRejectionDisposition disposition,
                                          const RejectionInput& input) {
    DepthContextIndex contexts(stroke.symmetry_instance_count, nullptr);
    if (!settings.depth_enabled) {
        return contexts;
    }
    for (const DepthProjectionContext& context : input.depth_contexts) {
        if (context.symmetry_instance >= contexts.size() ||
            contexts[context.symmetry_instance] != nullptr || !context.transform_consistent ||
            context.screen_positions.size() != surface.texels.size() ||
            context.surface_depth.size() != surface.texels.size() ||
            context.visible_depth.size() !=
                checked_pixel_count(context.viewport_width, context.viewport_height)) {
            throw std::invalid_argument("depth projection context is inconsistent");
        }
        for (std::size_t index = 0; index < context.screen_positions.size(); ++index) {
            if (!finite(context.screen_positions[index]) || !finite(context.surface_depth[index])) {
                throw std::invalid_argument("depth projection texels must be finite");
            }
        }
        if (!std::all_of(context.visible_depth.begin(), context.visible_depth.end(),
                         [](double value) { return finite(value); })) {
            throw std::invalid_argument("visible depth buffer must be finite");
        }
        contexts[context.symmetry_instance] = &context;
    }
    for (std::uint64_t instance = 0; instance < contexts.size(); ++instance) {
        if (depth_required(instance, disposition) && contexts[instance] == nullptr) {
            throw std::invalid_argument(
                "depth rejection requires a consistent context per instance");
        }
    }
    return contexts;
}

RejectionSettings resolve_settings(const RejectionSettings& settings,
                                   ToolParameterReport& parameter_report) {
    if ((settings.symmetry_depth_policy != SymmetryDepthPolicy::require_consistent_per_instance &&
         settings.symmetry_depth_policy != SymmetryDepthPolicy::disable_for_derived_symmetry)) {
        throw std::invalid_argument("paint rejection symmetry policy is invalid");
    }
    RejectionSettings resolved = settings;
    resolved.depth_bias = validate_tool_parameter(rejection_depth_bias_parameter,
                                                  settings.depth_bias, parameter_report);
    resolved.minimum_normal_dot = validate_tool_parameter(
        rejection_minimum_normal_dot_parameter, settings.minimum_normal_dot, parameter_report);
    return resolved;
}

void validate_view_directions(const TextureSpaceRaster& surface, const RejectionSettings& settings,
                              const RejectionInput& input) {
    if (!settings.backface_enabled) {
        return;
    }
    if (input.view_directions.size() != surface.texels.size()) {
        throw std::invalid_argument("backface rejection requires one view direction per texel");
    }
    for (std::size_t index = 0; index < surface.texels.size(); ++index) {
        if (surface.covered(index)) {
            static_cast<void>(normalized(input.view_directions[index], "view direction"));
        }
    }
}

bool faces_camera(const SurfaceTexel& texel, Vec3d view_direction) {
    return dot(normalized(texel.geometric_normal, "geometric normal"),
               normalized(view_direction, "view direction")) > 0.0;
}

bool occluded(const DepthProjectionContext& context, std::size_t texel, double bias) {
    const Vec2d screen = context.screen_positions[texel];
    if (screen.x < 0.0 || screen.y < 0.0 ||
        screen.x >= static_cast<double>(context.viewport_width) ||
        screen.y >= static_cast<double>(context.viewport_height)) {
        return false;
    }
    const auto x = static_cast<std::uint32_t>(std::floor(screen.x));
    const auto y = static_cast<std::uint32_t>(std::floor(screen.y));
    const std::size_t pixel = static_cast<std::size_t>(y) * context.viewport_width + x;
    return context.surface_depth[texel] > context.visible_depth[pixel] + bias;
}

struct EvaluationContext {
    const TextureSpaceRaster& surface;
    const RejectionSettings& settings;
    const DepthContextIndex& depth_contexts;
    DepthRejectionDisposition depth_disposition;
    RejectionReport& report;
};

bool accepts_contribution(const detail::CoverageContribution& contribution, std::size_t texel,
                          const EvaluationContext& context) {
    if (contribution.value <= 0.0) {
        return false;
    }
    const Vec3d surface_normal = normalized(context.surface.texels[texel].normal, "surface normal");
    const Vec3d reference_normal = normalized(contribution.reference_normal, "reference normal");
    if (context.settings.angle_enabled &&
        dot(surface_normal, reference_normal) < context.settings.minimum_normal_dot) {
        ++context.report.angle_rejected_contributions;
        return false;
    }
    if (context.settings.depth_enabled &&
        depth_required(contribution.symmetry_instance, context.depth_disposition) &&
        occluded(*context.depth_contexts[contribution.symmetry_instance], texel,
                 context.settings.depth_bias)) {
        ++context.report.depth_rejected_contributions;
        return false;
    }
    return true;
}

std::vector<const SweptSegment*> segment_ending_at(const ResolvedStroke& stroke) {
    std::vector<const SweptSegment*> result(stroke.stamps.size(), nullptr);
    for (const SweptSegment& segment : stroke.swept_segments) {
        result[segment.end_stamp_ordinal] = &segment;
    }
    return result;
}

detail::CoverageContribution canonical_contribution(
    Vec3d point, const ResolvedStroke& stroke, std::size_t stamp_index,
    const std::vector<const SweptSegment*>& ending_segments) {
    const Stamp& stamp = stroke.stamps[stamp_index];
    if (stroke.tip_mode == TipMode::continuous_sweep && ending_segments[stamp_index] != nullptr) {
        const SweptSegment& segment = *ending_segments[stamp_index];
        return detail::segment_contribution(point, stroke.stamps[segment.start_stamp_ordinal],
                                            stroke.stamps[segment.end_stamp_ordinal]);
    }
    return detail::stamp_contribution(point, stamp, stroke.tip_mode == TipMode::discrete_alpha);
}

}  // namespace

RejectedCoverageRaster evaluate_rejected_coverage(const TextureSpaceRaster& surface,
                                                  const ResolvedStroke& stroke,
                                                  const RejectionSettings& settings,
                                                  const RejectionInput& input) {
    detail::validate_surface_raster(surface);
    ToolParameterReport parameter_report;
    const RejectionSettings resolved_settings = resolve_settings(settings, parameter_report);
    validate_view_directions(surface, resolved_settings, input);
    const ResolvedStroke validated = ingest_resolved_stroke(stroke);
    RejectedCoverageRaster output{
        .coverage = {.width = surface.width,
                     .height = surface.height,
                     .values = std::vector<double>(surface.texels.size(), 0.0)},
        .stamp_events = {},
        .report = {.depth_disposition = depth_disposition(validated, resolved_settings),
                   .resolved_settings = resolved_settings,
                   .parameter_report = std::move(parameter_report)}};
    output.stamp_events.reserve(validated.stamps.size());
    for (const Stamp& stamp : validated.stamps) {
        output.stamp_events.push_back({.stamp_ordinal = stamp.ordinal,
                                       .values = std::vector<double>(surface.texels.size(), 0.0)});
    }
    const DepthContextIndex contexts = validate_depth_contexts(
        surface, validated, resolved_settings, output.report.depth_disposition, input);
    const EvaluationContext context{surface, resolved_settings, contexts,
                                    output.report.depth_disposition, output.report};
    const auto ending_segments = segment_ending_at(validated);
    for (std::size_t texel = 0; texel < surface.texels.size(); ++texel) {
        if (!surface.covered(texel)) {
            continue;
        }
        if (resolved_settings.backface_enabled &&
            !faces_camera(surface.texels[texel], input.view_directions[texel])) {
            ++output.report.backface_rejected_texels;
            continue;
        }
        for (std::size_t stamp_index = 0; stamp_index < validated.stamps.size(); ++stamp_index) {
            const auto contribution = canonical_contribution(
                surface.texels[texel].position, validated, stamp_index, ending_segments);
            if (!accepts_contribution(contribution, texel, context)) {
                continue;
            }
            output.stamp_events[stamp_index].values[texel] = contribution.value;
            output.coverage.values[texel] =
                std::max(output.coverage.values[texel], contribution.value);
        }
    }
    return output;
}

AlphaDiscardThresholdResult alpha_discard_threshold(const AlphaDiscardSettings& settings) {
    if (settings.format != AlphaDiscardFormat::unorm8 &&
        settings.format != AlphaDiscardFormat::unorm16 &&
        settings.format != AlphaDiscardFormat::floating_point) {
        throw std::invalid_argument("alpha discard format is invalid");
    }
    const double default_threshold = settings.format == AlphaDiscardFormat::unorm8
                                         ? default_alpha_discard_8_bit
                                         : default_alpha_discard_high_precision;
    ToolParameterReport parameter_report;
    double threshold = default_threshold;
    if (settings.threshold) {
        const ToolParameterDescriptor descriptor{"alpha_discard.threshold", default_threshold, 0.0,
                                                 1.0};
        threshold = validate_tool_parameter(descriptor, *settings.threshold, parameter_report);
    }
    return {.threshold = threshold, .parameter_report = std::move(parameter_report)};
}

AlphaDiscardResult apply_alpha_discard(std::span<const double> accumulated_strength,
                                       const AlphaDiscardSettings& settings) {
    AlphaDiscardThresholdResult threshold = alpha_discard_threshold(settings);
    AlphaDiscardResult result{.threshold = threshold.threshold,
                              .retained_strength = {},
                              .write_mask = {},
                              .parameter_report = std::move(threshold.parameter_report)};
    result.retained_strength.reserve(accumulated_strength.size());
    result.write_mask.reserve(accumulated_strength.size());
    for (const double strength : accumulated_strength) {
        if (!finite(strength) || strength < 0.0 || strength > 1.0) {
            throw std::invalid_argument("accumulated paint strength must be normalized and finite");
        }
        result.retained_strength.push_back(strength);
        result.write_mask.push_back(strength >= result.threshold ? 1 : 0);
    }
    return result;
}

}  // namespace ctex::paint
