#include <algorithm>
#include <cmath>
#include <ctex/paint/stroke.hpp>
#include <limits>
#include <numbers>
#include <optional>
#include <string_view>
#include <utility>

namespace ctex::paint {
namespace {

struct PathPoint {
    Vec3d position;
    StrokeFrame frame;
    std::uint64_t timestamp_nanoseconds;
    double pressure;
    Vec2d tilt;
};

bool finite(double value) { return std::isfinite(value); }

bool finite(Vec3d value) { return finite(value.x) && finite(value.y) && finite(value.z); }

bool finite(Vec2d value) { return finite(value.x) && finite(value.y); }

double interpolate(double start, double end, double amount) {
    return start + (end - start) * amount;
}

Vec3d add(Vec3d left, Vec3d right) {
    return {left.x + right.x, left.y + right.y, left.z + right.z};
}

Vec3d subtract(Vec3d left, Vec3d right) {
    return {left.x - right.x, left.y - right.y, left.z - right.z};
}

Vec3d multiply(Vec3d value, double factor) {
    return {value.x * factor, value.y * factor, value.z * factor};
}

double dot(Vec3d left, Vec3d right) {
    return left.x * right.x + left.y * right.y + left.z * right.z;
}

Vec3d cross(Vec3d left, Vec3d right) {
    return {left.y * right.z - left.z * right.y, left.z * right.x - left.x * right.z,
            left.x * right.y - left.y * right.x};
}

double length(Vec3d value) { return std::sqrt(dot(value, value)); }

Vec3d normalized(Vec3d value, std::string_view role) {
    const double magnitude = length(value);
    if (!finite(magnitude) || magnitude <= stroke_position_tolerance) {
        throw StrokeResolutionError(std::string(role) + " vector is degenerate");
    }
    return multiply(value, 1.0 / magnitude);
}

Vec3d interpolate(Vec3d start, Vec3d end, double amount) {
    return add(start, multiply(subtract(end, start), amount));
}

Vec2d interpolate(Vec2d start, Vec2d end, double amount) {
    return {interpolate(start.x, end.x, amount), interpolate(start.y, end.y, amount)};
}

StrokeFrame orthonormalized(StrokeFrame frame) {
    const Vec3d normal = normalized(frame.normal, "normal");
    Vec3d tangent_in_plane = subtract(frame.tangent, multiply(normal, dot(frame.tangent, normal)));
    if (length(tangent_in_plane) <= stroke_position_tolerance) {
        tangent_in_plane = cross(frame.bitangent, normal);
    }
    const Vec3d tangent = normalized(tangent_in_plane, "tangent");
    const double handedness = dot(cross(tangent, frame.bitangent), normal) < 0.0 ? -1.0 : 1.0;
    return {
        .tangent = tangent,
        .bitangent = multiply(cross(normal, tangent), handedness),
        .normal = normal,
    };
}

StrokeFrame interpolate(StrokeFrame start, StrokeFrame end, double amount) {
    StrokeFrame blended{
        .tangent = interpolate(start.tangent, end.tangent, amount),
        .bitangent = interpolate(start.bitangent, end.bitangent, amount),
        .normal = interpolate(start.normal, end.normal, amount),
    };
    if (length(blended.normal) <= stroke_position_tolerance) {
        blended.normal = amount < 0.5 ? start.normal : end.normal;
    }
    if (length(blended.tangent) <= stroke_position_tolerance) {
        blended.tangent = amount < 0.5 ? start.tangent : end.tangent;
    }
    if (length(blended.bitangent) <= stroke_position_tolerance) {
        blended.bitangent = amount < 0.5 ? start.bitangent : end.bitangent;
    }
    return orthonormalized(blended);
}

bool near(Vec3d left, Vec3d right) {
    return length(subtract(left, right)) <= stroke_position_tolerance;
}

bool near(Vec2d left, Vec2d right) {
    return std::hypot(left.x - right.x, left.y - right.y) <= stroke_position_tolerance;
}

bool near(double left, double right) { return std::abs(left - right) <= stroke_position_tolerance; }

bool near(StrokeFrame left, StrokeFrame right) {
    return near(left.tangent, right.tangent) && near(left.bitangent, right.bitangent) &&
           near(left.normal, right.normal);
}

double effective_pressure(const StrokeInputSample& sample) { return sample.pressure.value_or(1.0); }

void validate_curve(const ResponseCurve& curve) {
    if (curve.points.size() < 2 || curve.points.front().input != 0.0 ||
        curve.points.back().input != 1.0) {
        throw StrokeResolutionError(
            "response curves require at least two points spanning input 0 through 1");
    }
    double previous_input = -1.0;
    for (const ResponseCurvePoint point : curve.points) {
        if (!finite(point.input) || !finite(point.output) || point.input < 0.0 ||
            point.input > 1.0 || point.output < 0.0 || point.output > 1.0 ||
            point.input <= previous_input) {
            throw StrokeResolutionError(
                "response curve inputs must increase and inputs and outputs must be normalized");
        }
        previous_input = point.input;
    }
}

void validate_mapping(const ResponseMapping& mapping) {
    validate_curve(mapping.curve);
    if (mapping.minimum_output > mapping.maximum_output) {
        throw StrokeResolutionError("response mapping output range is invalid");
    }
}

void validate_input_mapping(const StrokeInputMapping& mapping) {
    validate_mapping(mapping.pressure_radius);
    validate_mapping(mapping.pressure_opacity);
    validate_mapping(mapping.pressure_hardness);
    validate_mapping(mapping.pressure_flow);
    validate_mapping(mapping.pressure_rotation);
    validate_mapping(mapping.tilt_rotation);
    validate_mapping(mapping.tilt_elongation);
}

void resolve_mapping_parameters(ResponseMapping& mapping,
                                const ToolParameterDescriptor& minimum_descriptor,
                                const ToolParameterDescriptor& maximum_descriptor,
                                ToolParameterReport& report) {
    mapping.minimum_output =
        validate_tool_parameter(minimum_descriptor, mapping.minimum_output, report);
    mapping.maximum_output =
        validate_tool_parameter(maximum_descriptor, mapping.maximum_output, report);
}

void resolve_input_mapping_parameters(StrokeInputMapping& mapping, ToolParameterReport& report) {
    resolve_mapping_parameters(mapping.pressure_radius, stroke_pressure_radius_minimum_parameter,
                               stroke_pressure_radius_maximum_parameter, report);
    resolve_mapping_parameters(mapping.pressure_opacity, stroke_pressure_opacity_minimum_parameter,
                               stroke_pressure_opacity_maximum_parameter, report);
    resolve_mapping_parameters(mapping.pressure_hardness,
                               stroke_pressure_hardness_minimum_parameter,
                               stroke_pressure_hardness_maximum_parameter, report);
    resolve_mapping_parameters(mapping.pressure_flow, stroke_pressure_flow_minimum_parameter,
                               stroke_pressure_flow_maximum_parameter, report);
    resolve_mapping_parameters(mapping.pressure_rotation,
                               stroke_pressure_rotation_minimum_parameter,
                               stroke_pressure_rotation_maximum_parameter, report);
    resolve_mapping_parameters(mapping.tilt_rotation, stroke_tilt_rotation_minimum_parameter,
                               stroke_tilt_rotation_maximum_parameter, report);
    resolve_mapping_parameters(mapping.tilt_elongation, stroke_tilt_elongation_minimum_parameter,
                               stroke_tilt_elongation_maximum_parameter, report);
}

void validate_taper_span(TaperSpan span) {
    if (!finite(span.extent)) {
        throw StrokeResolutionError("taper extent must be finite");
    }
    if (span.unit == TaperUnit::none && span.extent == 0.0) {
        return;
    }
    if (span.unit == TaperUnit::stamp_count && span.extent >= 2.0 &&
        std::floor(span.extent) == span.extent) {
        return;
    }
    if (span.unit == TaperUnit::distance && span.extent > 0.0) {
        return;
    }
    throw StrokeResolutionError("taper span requires no extent, at least two stamps, or distance");
}

void validate_taper(const TaperSettings& taper) {
    validate_taper_span(taper.entry);
    validate_taper_span(taper.exit);
    if ((taper.entry.unit != TaperUnit::none || taper.exit.unit != TaperUnit::none) &&
        !taper.affect_radius && !taper.affect_opacity) {
        throw StrokeResolutionError("an active taper must affect radius or opacity");
    }
}

void validate_constraint(const ConstraintSettings& constraint) {
    if (constraint.mode != ConstraintMode::none &&
        constraint.mode != ConstraintMode::straight_line &&
        constraint.mode != ConstraintMode::dominant_axis &&
        constraint.mode != ConstraintMode::grid) {
        throw StrokeResolutionError("stroke constraint mode is invalid");
    }
}

void validate_symmetry(const SymmetrySettings& symmetry) {
    if (symmetry.radial_axis != SymmetryAxis::x && symmetry.radial_axis != SymmetryAxis::y &&
        symmetry.radial_axis != SymmetryAxis::z) {
        throw StrokeResolutionError("radial symmetry axis is invalid");
    }
}

void validate_settings(const StrokeSettings& settings) {
    if (settings.reconstruction_version != canonical_stroke_reconstruction_version) {
        throw StrokeResolutionError("unsupported stroke reconstruction version " +
                                    std::to_string(settings.reconstruction_version));
    }
    if (settings.tip_mode != TipMode::continuous_sweep &&
        settings.tip_mode != TipMode::discrete_alpha) {
        throw StrokeResolutionError("stroke tip mode is invalid");
    }
    if (settings.tip_resource_identity.empty()) {
        throw StrokeResolutionError("stroke properties are invalid");
    }
    validate_input_mapping(settings.input_mapping);
    validate_taper(settings.taper);
    validate_constraint(settings.constraint);
    validate_symmetry(settings.symmetry);
}

void resolve_base_parameters(StrokeSettings& settings, ToolParameterReport& report) {
    settings.spacing_fraction =
        validate_tool_parameter(stroke_spacing_parameter, settings.spacing_fraction, report);
    settings.radius = validate_tool_parameter(stroke_radius_parameter, settings.radius, report);
    settings.opacity = validate_tool_parameter(stroke_opacity_parameter, settings.opacity, report);
    settings.hardness =
        validate_tool_parameter(stroke_hardness_parameter, settings.hardness, report);
    settings.rotation_radians =
        validate_tool_parameter(stroke_rotation_parameter, settings.rotation_radians, report);
    settings.elongation =
        validate_tool_parameter(stroke_elongation_parameter, settings.elongation, report);
    settings.flow = validate_tool_parameter(stroke_flow_parameter, settings.flow, report);
    settings.stabilizer.radius = validate_tool_parameter(stroke_stabilizer_radius_parameter,
                                                         settings.stabilizer.radius, report);
    settings.stabilizer.time_constant_seconds = validate_tool_parameter(
        stroke_stabilizer_time_parameter, settings.stabilizer.time_constant_seconds, report);
}

void resolve_modifier_parameters(StrokeSettings& settings, ToolParameterReport& report) {
    settings.jitter.position_fraction = validate_tool_parameter(
        stroke_jitter_position_parameter, settings.jitter.position_fraction, report);
    settings.jitter.radius_fraction = validate_tool_parameter(
        stroke_jitter_radius_parameter, settings.jitter.radius_fraction, report);
    settings.jitter.rotation_radians = validate_tool_parameter(
        stroke_jitter_rotation_parameter, settings.jitter.rotation_radians, report);
    settings.jitter.opacity =
        validate_tool_parameter(stroke_jitter_opacity_parameter, settings.jitter.opacity, report);
    settings.jitter.flow =
        validate_tool_parameter(stroke_jitter_flow_parameter, settings.jitter.flow, report);
    settings.taper.floor =
        validate_tool_parameter(stroke_taper_floor_parameter, settings.taper.floor, report);
    settings.constraint.grid_step =
        validate_tool_parameter(stroke_grid_step_parameter, settings.constraint.grid_step, report);
    settings.symmetry.radial_count = static_cast<std::uint32_t>(validate_tool_parameter(
        stroke_radial_count_parameter, settings.symmetry.radial_count, report));
}

void validate_sample(const StrokeInputSample& sample) {
    if (!finite(sample.position) || !finite(sample.frame.tangent) ||
        !finite(sample.frame.bitangent) || !finite(sample.frame.normal)) {
        throw StrokeResolutionError("stroke sample contains a non-finite value");
    }
    if ((sample.pressure &&
         (!finite(*sample.pressure) || *sample.pressure < 0.0 || *sample.pressure > 1.0)) ||
        !finite(sample.tilt) || std::hypot(sample.tilt.x, sample.tilt.y) > 1.0) {
        throw StrokeResolutionError("stroke pressure and tilt must be normalized");
    }
    static_cast<void>(orthonormalized(sample.frame));
}

bool redundant(const StrokeInputSample& start, const StrokeInputSample& middle,
               const StrokeInputSample& end) {
    const double amount =
        static_cast<double>(middle.timestamp_nanoseconds - start.timestamp_nanoseconds) /
        static_cast<double>(end.timestamp_nanoseconds - start.timestamp_nanoseconds);
    const StrokeFrame expected_frame{
        .tangent = interpolate(start.frame.tangent, end.frame.tangent, amount),
        .bitangent = interpolate(start.frame.bitangent, end.frame.bitangent, amount),
        .normal = interpolate(start.frame.normal, end.frame.normal, amount),
    };
    return near(middle.position, interpolate(start.position, end.position, amount)) &&
           near(middle.frame, expected_frame) &&
           near(effective_pressure(middle),
                interpolate(effective_pressure(start), effective_pressure(end), amount)) &&
           near(middle.tilt, interpolate(start.tilt, end.tilt, amount));
}

std::vector<StrokeInputSample> remove_redundant_samples(
    std::span<const StrokeInputSample> samples) {
    std::vector<StrokeInputSample> result;
    result.reserve(samples.size());
    for (const StrokeInputSample& sample : samples) {
        result.push_back(sample);
        while (result.size() >= 3 &&
               redundant(result[result.size() - 3], result[result.size() - 2], result.back())) {
            result.erase(result.end() - 2);
        }
    }
    return result;
}

PathPoint interpolate(const StrokeInputSample& start, const StrokeInputSample& end,
                      std::uint64_t timestamp) {
    const double amount =
        static_cast<double>(timestamp - start.timestamp_nanoseconds) /
        static_cast<double>(end.timestamp_nanoseconds - start.timestamp_nanoseconds);
    return {
        .position = interpolate(start.position, end.position, amount),
        .frame = interpolate(start.frame, end.frame, amount),
        .timestamp_nanoseconds = timestamp,
        .pressure = interpolate(effective_pressure(start), effective_pressure(end), amount),
        .tilt = interpolate(start.tilt, end.tilt, amount),
    };
}

std::optional<std::uint64_t> next_grid_time(std::uint64_t timestamp) {
    if (timestamp >
        std::numeric_limits<std::uint64_t>::max() - stabilization_time_step_nanoseconds) {
        return std::nullopt;
    }
    return timestamp + stabilization_time_step_nanoseconds;
}

std::vector<PathPoint> reconstruct_time_grid(std::span<const StrokeInputSample> samples) {
    std::vector<PathPoint> result;
    result.push_back({samples.front().position, orthonormalized(samples.front().frame),
                      samples.front().timestamp_nanoseconds, effective_pressure(samples.front()),
                      samples.front().tilt});
    std::optional<std::uint64_t> grid = next_grid_time(samples.front().timestamp_nanoseconds);
    for (std::size_t index = 1; index < samples.size(); ++index) {
        const StrokeInputSample& start = samples[index - 1];
        const StrokeInputSample& end = samples[index];
        while (grid && *grid < end.timestamp_nanoseconds) {
            result.push_back(interpolate(start, end, *grid));
            grid = next_grid_time(*grid);
        }
        result.push_back({end.position, orthonormalized(end.frame), end.timestamp_nanoseconds,
                          effective_pressure(end), end.tilt});
        if (grid && *grid == end.timestamp_nanoseconds) {
            grid = next_grid_time(*grid);
        }
    }
    return result;
}

void apply_straight_line_constraint(std::vector<PathPoint>& path) {
    if (path.size() < 2) {
        return;
    }
    const Vec3d origin = path.front().position;
    const Vec3d current = path.back().position;
    const std::uint64_t first_time = path.front().timestamp_nanoseconds;
    const double duration = static_cast<double>(path.back().timestamp_nanoseconds - first_time);
    for (PathPoint& point : path) {
        const double amount =
            static_cast<double>(point.timestamp_nanoseconds - first_time) / duration;
        point.position = interpolate(origin, current, amount);
    }
}

void apply_dominant_axis_constraint(std::vector<PathPoint>& path) {
    const Vec3d origin = path.front().position;
    const Vec3d displacement = subtract(path.back().position, origin);
    std::size_t axis = 0;
    double largest = std::abs(displacement.x);
    if (std::abs(displacement.y) > largest) {
        axis = 1;
        largest = std::abs(displacement.y);
    }
    if (std::abs(displacement.z) > largest) {
        axis = 2;
    }
    for (PathPoint& point : path) {
        const double component = axis == 0   ? point.position.x
                                 : axis == 1 ? point.position.y
                                             : point.position.z;
        point.position = origin;
        if (axis == 0) {
            point.position.x = component;
        } else if (axis == 1) {
            point.position.y = component;
        } else {
            point.position.z = component;
        }
    }
}

void apply_directional_constraint(std::vector<PathPoint>& path, ConstraintMode mode) {
    if (mode == ConstraintMode::straight_line) {
        apply_straight_line_constraint(path);
    } else if (mode == ConstraintMode::dominant_axis) {
        apply_dominant_axis_constraint(path);
    }
}

void apply_stabilizer(std::vector<PathPoint>& path, StabilizerSettings settings) {
    Vec3d cursor = path.front().position;
    for (std::size_t index = 1; index < path.size(); ++index) {
        const Vec3d offset = subtract(path[index].position, cursor);
        const double distance = length(offset);
        if (distance <= settings.radius) {
            path[index].position = cursor;
            continue;
        }
        const Vec3d boundary =
            subtract(path[index].position, multiply(offset, settings.radius / distance));
        const double elapsed_seconds = static_cast<double>(path[index].timestamp_nanoseconds -
                                                           path[index - 1].timestamp_nanoseconds) /
                                       1'000'000'000.0;
        const double factor = settings.time_constant_seconds == 0.0
                                  ? 1.0
                                  : -std::expm1(-elapsed_seconds / settings.time_constant_seconds);
        cursor = add(cursor, multiply(subtract(boundary, cursor), factor));
        path[index].position = cursor;
    }
}

void apply_grid_constraint(std::vector<PathPoint>& path, ConstraintSettings settings) {
    if (settings.mode != ConstraintMode::grid) {
        return;
    }
    const auto snapped = [step = settings.grid_step](double value) {
        return std::round(value / step) * step;
    };
    for (PathPoint& point : path) {
        point.position = {
            snapped(point.position.x),
            snapped(point.position.y),
            snapped(point.position.z),
        };
    }
}

PathPoint interpolate(const PathPoint& start, const PathPoint& end, double amount) {
    return {
        .position = interpolate(start.position, end.position, amount),
        .frame = interpolate(start.frame, end.frame, amount),
        .timestamp_nanoseconds = start.timestamp_nanoseconds,
        .pressure = interpolate(start.pressure, end.pressure, amount),
        .tilt = interpolate(start.tilt, end.tilt, amount),
    };
}

double evaluate(const ResponseCurve& curve, double input) {
    const auto upper = std::upper_bound(
        curve.points.begin(), curve.points.end(), input,
        [](double value, const ResponseCurvePoint& point) { return value < point.input; });
    if (upper == curve.points.begin()) {
        return upper->output;
    }
    if (upper == curve.points.end()) {
        return curve.points.back().output;
    }
    const ResponseCurvePoint& end = *upper;
    const ResponseCurvePoint& start = *(upper - 1);
    const double amount = (input - start.input) / (end.input - start.input);
    return interpolate(start.output, end.output, amount);
}

double mapped(const ResponseMapping& mapping, double input) {
    return interpolate(mapping.minimum_output, mapping.maximum_output,
                       evaluate(mapping.curve, input));
}

double multiplied(double base, const ResponseMapping& mapping, double input) {
    return mapping.enabled ? base * mapped(mapping, input) : base;
}

Stamp make_stamp(const PathPoint& point, const StrokeSettings& settings, std::uint64_t ordinal) {
    const StrokeInputMapping& mapping = settings.input_mapping;
    const double tilt_amount = std::hypot(point.tilt.x, point.tilt.y);
    double rotation = settings.rotation_radians;
    if (mapping.pressure_rotation.enabled) {
        rotation += mapped(mapping.pressure_rotation, point.pressure);
    }
    if (mapping.tilt_rotation.enabled && tilt_amount > stroke_position_tolerance) {
        rotation +=
            std::atan2(point.tilt.y, point.tilt.x) * mapped(mapping.tilt_rotation, tilt_amount);
    }
    return {
        .position = point.position,
        .frame = point.frame,
        .radius = multiplied(settings.radius, mapping.pressure_radius, point.pressure),
        .opacity = multiplied(settings.opacity, mapping.pressure_opacity, point.pressure),
        .hardness = multiplied(settings.hardness, mapping.pressure_hardness, point.pressure),
        .rotation_radians = rotation,
        .elongation = multiplied(settings.elongation, mapping.tilt_elongation, tilt_amount),
        .flow = multiplied(settings.flow, mapping.pressure_flow, point.pressure),
        .tip_resource_identity = settings.tip_resource_identity,
        .source_ordinal = ordinal,
        .symmetry_instance = 0,
        .ordinal = ordinal,
    };
}

void append_stamp(ResolvedStroke& result, const PathPoint& point, const StrokeSettings& settings) {
    const std::uint64_t ordinal = result.stamps.size();
    result.stamps.push_back(make_stamp(point, settings, ordinal));
    if (settings.tip_mode == TipMode::continuous_sweep && ordinal != 0) {
        result.swept_segments.push_back({ordinal - 1, ordinal});
    }
}

ResolvedStroke space_stamps(std::span<const PathPoint> path, const StrokeSettings& settings) {
    ResolvedStroke result{
        .reconstruction_version = settings.reconstruction_version,
        .tip_mode = settings.tip_mode,
        .symmetry_instance_count = 1,
        .stamps = {},
        .swept_segments = {},
    };
    append_stamp(result, path.front(), settings);
    double distance_to_next = settings.spacing_fraction * result.stamps.back().radius;
    for (std::size_t index = 1; index < path.size(); ++index) {
        const PathPoint& start = path[index - 1];
        const PathPoint& end = path[index];
        const double segment_length = length(subtract(end.position, start.position));
        if (segment_length <= stroke_position_tolerance) {
            continue;
        }
        double consumed = 0.0;
        while (distance_to_next <= segment_length - consumed) {
            const double event_distance = std::min(segment_length, consumed + distance_to_next);
            append_stamp(result, interpolate(start, end, event_distance / segment_length),
                         settings);
            consumed = event_distance;
            distance_to_next = settings.spacing_fraction * result.stamps.back().radius;
        }
        distance_to_next -= segment_length - consumed;
    }
    if (settings.tip_mode == TipMode::continuous_sweep &&
        !near(result.stamps.back().position, path.back().position)) {
        append_stamp(result, path.back(), settings);
    }
    return result;
}

double taper_progress(TaperSpan span, std::uint64_t stamp_index, double distance) {
    if (span.unit == TaperUnit::none) {
        return 1.0;
    }
    if (span.unit == TaperUnit::stamp_count) {
        return std::min(static_cast<double>(stamp_index) / (span.extent - 1.0), 1.0);
    }
    return std::min(distance / span.extent, 1.0);
}

void apply_taper(ResolvedStroke& stroke, const TaperSettings& taper) {
    if (taper.entry.unit == TaperUnit::none && taper.exit.unit == TaperUnit::none) {
        return;
    }
    std::vector<double> distances(stroke.stamps.size());
    for (std::size_t index = 1; index < stroke.stamps.size(); ++index) {
        distances[index] =
            distances[index - 1] +
            length(subtract(stroke.stamps[index].position, stroke.stamps[index - 1].position));
    }
    const double total_distance = distances.back();
    for (std::size_t index = 0; index < stroke.stamps.size(); ++index) {
        const double entry = taper_progress(taper.entry, index, distances[index]);
        const double exit = taper_progress(taper.exit, stroke.stamps.size() - index - 1,
                                           total_distance - distances[index]);
        const double factor = interpolate(taper.floor, 1.0, std::min(entry, exit));
        if (taper.affect_radius) {
            stroke.stamps[index].radius *= factor;
        }
        if (taper.affect_opacity) {
            stroke.stamps[index].opacity *= factor;
        }
    }
}

std::uint64_t mixed(std::uint64_t value) {
    value += 0x9e3779b97f4a7c15ULL;
    value = (value ^ (value >> 30U)) * 0xbf58476d1ce4e5b9ULL;
    value = (value ^ (value >> 27U)) * 0x94d049bb133111ebULL;
    return value ^ (value >> 31U);
}

double jitter_value(std::uint64_t seed, std::uint64_t ordinal, std::uint64_t channel) {
    const std::uint64_t bits = mixed(seed ^ (0x9e3779b97f4a7c15ULL * (ordinal + 1U)) ^
                                     (0xd1b54a32d192ed03ULL * (channel + 1U)));
    constexpr double inverse_53_bits = 1.0 / 9'007'199'254'740'992.0;
    return 2.0 * static_cast<double>(bits >> 11U) * inverse_53_bits - 1.0;
}

void apply_jitter(ResolvedStroke& stroke, const JitterSettings& jitter) {
    for (Stamp& stamp : stroke.stamps) {
        const double position_scale = jitter.position_fraction * stamp.radius;
        stamp.position =
            add(stamp.position,
                add(multiply(stamp.frame.tangent,
                             position_scale * jitter_value(jitter.seed, stamp.ordinal, 0)),
                    multiply(stamp.frame.bitangent,
                             position_scale * jitter_value(jitter.seed, stamp.ordinal, 1))));
        stamp.radius *= 1.0 + jitter.radius_fraction * jitter_value(jitter.seed, stamp.ordinal, 2);
        stamp.rotation_radians +=
            jitter.rotation_radians * jitter_value(jitter.seed, stamp.ordinal, 3);
        stamp.opacity = std::clamp(
            stamp.opacity + jitter.opacity * jitter_value(jitter.seed, stamp.ordinal, 4), 0.0, 1.0);
        stamp.flow = std::clamp(
            stamp.flow + jitter.flow * jitter_value(jitter.seed, stamp.ordinal, 5), 0.0, 1.0);
    }
}

void apply_stamp_modifiers(ResolvedStroke& stroke, const StrokeSettings& settings) {
    apply_taper(stroke, settings.taper);
    apply_jitter(stroke, settings.jitter);
}

std::vector<std::uint8_t> mirror_masks(const SymmetrySettings& symmetry) {
    std::vector<std::uint8_t> result;
    for (std::uint8_t mask = 0; mask < 8; ++mask) {
        const bool uses_disabled_plane = ((mask & 1U) != 0U && !symmetry.mirror_x) ||
                                         ((mask & 2U) != 0U && !symmetry.mirror_y) ||
                                         ((mask & 4U) != 0U && !symmetry.mirror_z);
        if (!uses_disabled_plane) {
            result.push_back(mask);
        }
    }
    return result;
}

Vec3d mirrored(Vec3d value, std::uint8_t mask) {
    if ((mask & 1U) != 0U) {
        value.x = -value.x;
    }
    if ((mask & 2U) != 0U) {
        value.y = -value.y;
    }
    if ((mask & 4U) != 0U) {
        value.z = -value.z;
    }
    return value;
}

Vec3d rotated(Vec3d value, SymmetryAxis axis, double cosine, double sine) {
    if (axis == SymmetryAxis::x) {
        return {value.x, value.y * cosine - value.z * sine, value.y * sine + value.z * cosine};
    }
    if (axis == SymmetryAxis::y) {
        return {value.x * cosine + value.z * sine, value.y, -value.x * sine + value.z * cosine};
    }
    return {value.x * cosine - value.y * sine, value.x * sine + value.y * cosine, value.z};
}

Vec3d transformed(Vec3d value, std::uint8_t mirror_mask, SymmetryAxis axis, double cosine,
                  double sine) {
    return rotated(mirrored(value, mirror_mask), axis, cosine, sine);
}

Stamp symmetry_copy(const Stamp& source, std::uint8_t mirror_mask, SymmetryAxis axis, double cosine,
                    double sine, std::uint64_t instance, std::uint64_t ordinal) {
    Stamp copy = source;
    copy.position = transformed(copy.position, mirror_mask, axis, cosine, sine);
    copy.frame.tangent = transformed(copy.frame.tangent, mirror_mask, axis, cosine, sine);
    copy.frame.bitangent = transformed(copy.frame.bitangent, mirror_mask, axis, cosine, sine);
    copy.frame.normal = transformed(copy.frame.normal, mirror_mask, axis, cosine, sine);
    copy.symmetry_instance = instance;
    copy.ordinal = ordinal;
    return copy;
}

void reserve_symmetry_output(ResolvedStroke& output, const ResolvedStroke& source,
                             std::size_t instance_count) {
    if (source.stamps.size() > output.stamps.max_size() / instance_count ||
        source.swept_segments.size() > output.swept_segments.max_size() / instance_count) {
        throw StrokeResolutionError("symmetry expansion exceeds the addressable output size");
    }
    output.stamps.reserve(source.stamps.size() * instance_count);
    output.swept_segments.reserve(source.swept_segments.size() * instance_count);
}

ResolvedStroke expand_symmetry(const ResolvedStroke& source, SymmetrySettings symmetry) {
    const std::vector<std::uint8_t> masks = mirror_masks(symmetry);
    if (symmetry.radial_count > std::numeric_limits<std::size_t>::max() / masks.size()) {
        throw StrokeResolutionError("symmetry instance count exceeds the addressable output size");
    }
    const std::size_t instance_count = masks.size() * symmetry.radial_count;
    ResolvedStroke output{
        .reconstruction_version = source.reconstruction_version,
        .tip_mode = source.tip_mode,
        .symmetry_instance_count = instance_count,
        .stamps = {},
        .swept_segments = {},
    };
    reserve_symmetry_output(output, source, instance_count);
    std::uint64_t instance = 0;
    for (std::uint32_t radial_index = 0; radial_index < symmetry.radial_count; ++radial_index) {
        const double angle = 2.0 * std::numbers::pi * static_cast<double>(radial_index) /
                             static_cast<double>(symmetry.radial_count);
        const double cosine = std::cos(angle);
        const double sine = std::sin(angle);
        for (const std::uint8_t mirror_mask : masks) {
            const std::uint64_t offset = output.stamps.size();
            for (const Stamp& stamp : source.stamps) {
                output.stamps.push_back(symmetry_copy(stamp, mirror_mask, symmetry.radial_axis,
                                                      cosine, sine, instance,
                                                      output.stamps.size()));
            }
            for (const SweptSegment segment : source.swept_segments) {
                output.swept_segments.push_back(
                    {offset + segment.start_stamp_ordinal, offset + segment.end_stamp_ordinal});
            }
            ++instance;
        }
    }
    return output;
}

bool valid_external_frame(StrokeFrame frame) {
    const double tangent_length = length(frame.tangent);
    const double bitangent_length = length(frame.bitangent);
    const double normal_length = length(frame.normal);
    return finite(frame.tangent) && finite(frame.bitangent) && finite(frame.normal) &&
           std::abs(tangent_length - 1.0) <= stroke_position_tolerance &&
           std::abs(bitangent_length - 1.0) <= stroke_position_tolerance &&
           std::abs(normal_length - 1.0) <= stroke_position_tolerance &&
           std::abs(dot(frame.tangent, frame.bitangent)) <= stroke_position_tolerance &&
           std::abs(dot(frame.tangent, frame.normal)) <= stroke_position_tolerance &&
           std::abs(dot(frame.bitangent, frame.normal)) <= stroke_position_tolerance;
}

void validate_external_stamp(const Stamp& stamp, std::size_t index,
                             std::uint64_t symmetry_instance_count) {
    if (stamp.ordinal != index || stamp.source_ordinal > index ||
        stamp.symmetry_instance >= symmetry_instance_count || !finite(stamp.position) ||
        !valid_external_frame(stamp.frame) || !finite(stamp.radius) || stamp.radius < 0.0 ||
        !finite(stamp.opacity) || stamp.opacity < 0.0 || stamp.opacity > 1.0 ||
        !finite(stamp.hardness) || stamp.hardness < 0.0 || stamp.hardness > 1.0 ||
        !finite(stamp.rotation_radians) || !finite(stamp.elongation) || stamp.elongation <= 0.0 ||
        !finite(stamp.flow) || stamp.flow < 0.0 || stamp.flow > 1.0 ||
        stamp.tip_resource_identity.empty()) {
        throw StrokeResolutionError("external resolved stamp " + std::to_string(index) +
                                    " is invalid");
    }
}

void validate_external_order(std::span<const Stamp> stamps, std::uint64_t symmetry_instance_count) {
    if (stamps.front().symmetry_instance != 0 || stamps.front().source_ordinal != 0) {
        throw StrokeResolutionError(
            "external resolved stamps must begin with instance and source zero");
    }
    for (std::size_t index = 1; index < stamps.size(); ++index) {
        const Stamp& previous = stamps[index - 1];
        const Stamp& current = stamps[index];
        const bool continues_instance = current.symmetry_instance == previous.symmetry_instance &&
                                        current.source_ordinal == previous.source_ordinal + 1;
        const bool starts_instance = current.symmetry_instance == previous.symmetry_instance + 1 &&
                                     current.source_ordinal == 0;
        if (!continues_instance && !starts_instance) {
            throw StrokeResolutionError(
                "external resolved stamps require contiguous instances and source ordinals");
        }
    }
    if (stamps.back().symmetry_instance + 1 != symmetry_instance_count) {
        throw StrokeResolutionError("external resolved symmetry instance count is inconsistent");
    }
}

void validate_external_sweeps(const ResolvedStroke& stroke) {
    if (stroke.tip_mode == TipMode::discrete_alpha) {
        if (!stroke.swept_segments.empty()) {
            throw StrokeResolutionError("external discrete-alpha stroke cannot contain sweeps");
        }
        return;
    }
    std::size_t segment_index = 0;
    for (std::size_t stamp_index = 1; stamp_index < stroke.stamps.size(); ++stamp_index) {
        if (stroke.stamps[stamp_index].symmetry_instance !=
            stroke.stamps[stamp_index - 1].symmetry_instance) {
            continue;
        }
        const SweptSegment expected{stamp_index - 1, stamp_index};
        if (segment_index >= stroke.swept_segments.size() ||
            stroke.swept_segments[segment_index] != expected) {
            throw StrokeResolutionError(
                "external continuous stroke requires one ordered sweep per adjacent source pair");
        }
        ++segment_index;
    }
    if (segment_index != stroke.swept_segments.size()) {
        throw StrokeResolutionError("external continuous stroke contains an extra sweep");
    }
}

void validate_external_resolved_stroke(const ResolvedStroke& stroke) {
    if (stroke.reconstruction_version != canonical_stroke_reconstruction_version) {
        throw StrokeResolutionError("unsupported external resolved stroke version " +
                                    std::to_string(stroke.reconstruction_version));
    }
    if (stroke.tip_mode != TipMode::continuous_sweep &&
        stroke.tip_mode != TipMode::discrete_alpha) {
        throw StrokeResolutionError("external resolved stroke tip mode is invalid");
    }
    if (stroke.symmetry_instance_count == 0 || stroke.stamps.empty()) {
        throw StrokeResolutionError("external resolved stroke must contain stamps and an instance");
    }
    for (std::size_t index = 0; index < stroke.stamps.size(); ++index) {
        validate_external_stamp(stroke.stamps[index], index, stroke.symmetry_instance_count);
    }
    validate_external_order(stroke.stamps, stroke.symmetry_instance_count);
    validate_external_sweeps(stroke);
}

}  // namespace

ResolvedStroke ingest_resolved_stroke(const ResolvedStroke& stroke) {
    validate_external_resolved_stroke(stroke);
    return stroke;
}

StrokeResolver::StrokeResolver(StrokeSettings settings) : settings_(std::move(settings)) {
    try {
        resolve_base_parameters(settings_, parameter_report_);
        resolve_modifier_parameters(settings_, parameter_report_);
        resolve_input_mapping_parameters(settings_.input_mapping, parameter_report_);
    } catch (const std::invalid_argument& error) {
        throw StrokeResolutionError(error.what());
    }
    validate_settings(settings_);
}

void StrokeResolver::append_samples(std::span<const StrokeInputSample> samples) {
    if (resolved_) {
        throw std::logic_error("cannot append samples after stroke resolution");
    }
    std::uint64_t previous = samples_.empty() ? 0 : samples_.back().timestamp_nanoseconds;
    bool has_previous = !samples_.empty();
    for (const StrokeInputSample& sample : samples) {
        validate_sample(sample);
        if (has_previous && sample.timestamp_nanoseconds <= previous) {
            throw StrokeResolutionError("stroke sample timestamps must be strictly increasing");
        }
        previous = sample.timestamp_nanoseconds;
        has_previous = true;
    }
    samples_.insert(samples_.end(), samples.begin(), samples.end());
}

ResolvedStroke StrokeResolver::resolve() {
    if (resolved_) {
        throw std::logic_error("stroke has already been resolved");
    }
    if (samples_.empty()) {
        throw StrokeResolutionError("cannot resolve a stroke without samples");
    }
    const std::vector<StrokeInputSample> canonical = remove_redundant_samples(samples_);
    std::vector<PathPoint> path = reconstruct_time_grid(canonical);
    apply_directional_constraint(path, settings_.constraint.mode);
    apply_stabilizer(path, settings_.stabilizer);
    apply_grid_constraint(path, settings_.constraint);
    ResolvedStroke result = space_stamps(path, settings_);
    apply_stamp_modifiers(result, settings_);
    result = expand_symmetry(result, settings_.symmetry);
    resolved_ = true;
    return result;
}

}  // namespace ctex::paint
