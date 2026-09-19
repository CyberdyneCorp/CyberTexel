#include <algorithm>
#include <cmath>
#include <ctex/paint/stroke.hpp>
#include <limits>
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
    if (!finite(mapping.minimum_output) || !finite(mapping.maximum_output) ||
        mapping.minimum_output > mapping.maximum_output) {
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
    if (mapping.pressure_radius.minimum_output <= 0.0 ||
        mapping.pressure_opacity.minimum_output < 0.0 ||
        mapping.pressure_opacity.maximum_output > 1.0 ||
        mapping.pressure_hardness.minimum_output < 0.0 ||
        mapping.pressure_hardness.maximum_output > 1.0 ||
        mapping.pressure_flow.minimum_output < 0.0 || mapping.pressure_flow.maximum_output > 1.0 ||
        mapping.tilt_rotation.minimum_output < 0.0 || mapping.tilt_rotation.maximum_output > 1.0 ||
        mapping.tilt_elongation.minimum_output <= 0.0) {
        throw StrokeResolutionError("response mapping range is invalid for its property");
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
    if (!finite(settings.spacing_fraction) ||
        settings.spacing_fraction < minimum_spacing_fraction ||
        settings.spacing_fraction > maximum_spacing_fraction) {
        throw StrokeResolutionError("stroke spacing fraction must be between 0.01 and 4.0");
    }
    if (!finite(settings.radius) || settings.radius <= 0.0 || !finite(settings.opacity) ||
        settings.opacity < 0.0 || settings.opacity > 1.0 || !finite(settings.hardness) ||
        settings.hardness < 0.0 || settings.hardness > 1.0 || !finite(settings.rotation_radians) ||
        !finite(settings.elongation) || settings.elongation <= 0.0 || !finite(settings.flow) ||
        settings.flow < 0.0 || settings.flow > 1.0 || settings.tip_resource_identity.empty()) {
        throw StrokeResolutionError("stroke properties are invalid");
    }
    if (!finite(settings.stabilizer.radius) || settings.stabilizer.radius < 0.0 ||
        !finite(settings.stabilizer.time_constant_seconds) ||
        settings.stabilizer.time_constant_seconds < 0.0) {
        throw StrokeResolutionError(
            "stabilizer radius and time constant must be finite and non-negative");
    }
    validate_input_mapping(settings.input_mapping);
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

}  // namespace

StrokeResolver::StrokeResolver(StrokeSettings settings) : settings_(std::move(settings)) {
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
    apply_stabilizer(path, settings_.stabilizer);
    ResolvedStroke result = space_stamps(path, settings_);
    resolved_ = true;
    return result;
}

}  // namespace ctex::paint
