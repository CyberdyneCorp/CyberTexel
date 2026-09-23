#include <algorithm>
#include <cmath>
#include <ctex/paint/editable_surface_path.hpp>

namespace ctex::paint {
namespace {

Vec3d vec(const std::array<double, 3>& value) { return {value[0], value[1], value[2]}; }

Vec3d subtract(Vec3d left, Vec3d right) {
    return {left.x - right.x, left.y - right.y, left.z - right.z};
}

double dot(Vec3d left, Vec3d right) {
    return left.x * right.x + left.y * right.y + left.z * right.z;
}

Vec3d cross(Vec3d left, Vec3d right) {
    return {left.y * right.z - left.z * right.y, left.z * right.x - left.x * right.z,
            left.x * right.y - left.y * right.x};
}

Vec3d normalized(Vec3d value, std::string_view role) {
    const double length = std::sqrt(dot(value, value));
    if (!std::isfinite(length) || length <= stroke_position_tolerance) {
        throw StrokeResolutionError(std::string("editable surface path ") + std::string(role) +
                                    " is degenerate");
    }
    return {value.x / length, value.y / length, value.z / length};
}

StrokeFrame frame_at(const doc::EditableAuthoringEntry& entry, std::size_t index) {
    const Vec3d normal = normalized(vec(entry.surface_points[index].normal), "normal");
    const std::size_t neighbour = index + 1 < entry.surface_points.size() ? index + 1 : index - 1;
    Vec3d direction = subtract(vec(entry.surface_points[neighbour].position),
                               vec(entry.surface_points[index].position));
    if (neighbour < index) {
        direction = {-direction.x, -direction.y, -direction.z};
    }
    const double normal_component = dot(direction, normal);
    direction = {direction.x - normal.x * normal_component,
                 direction.y - normal.y * normal_component,
                 direction.z - normal.z * normal_component};
    const Vec3d tangent = normalized(direction, "tangent");
    const Vec3d bitangent = normalized(cross(normal, tangent), "bitangent");
    return {.tangent = tangent, .bitangent = bitangent, .normal = normal};
}

}  // namespace

std::vector<StrokeInputSample> editable_surface_path_samples(
    const doc::EditableAuthoringEntry& entry) {
    doc::validate_editable_authoring_entry(entry);
    if (entry.kind != doc::EditableEntryKind::surface_path) {
        throw StrokeResolutionError("editable entry is not a surface path");
    }
    const auto widths =
        std::ranges::minmax(entry.surface_points, {}, &doc::EditableSurfacePoint::width);
    const double minimum_width = widths.min.width;
    const double maximum_width = widths.max.width;
    if (minimum_width / maximum_width < 0.01) {
        throw StrokeResolutionError(
            "editable surface path width range exceeds the stroke-model response range");
    }
    std::vector<StrokeInputSample> result;
    result.reserve(entry.surface_points.size());
    for (std::size_t index = 0; index < entry.surface_points.size(); ++index) {
        const doc::EditableSurfacePoint& point = entry.surface_points[index];
        const double pressure =
            minimum_width == maximum_width
                ? 1.0
                : (point.width - minimum_width) / (maximum_width - minimum_width);
        result.push_back({.position = vec(point.position),
                          .frame = frame_at(entry, index),
                          .timestamp_nanoseconds = index * stabilization_time_step_nanoseconds,
                          .pressure = pressure,
                          .tilt = {}});
    }
    return result;
}

ResolvedStroke evaluate_editable_surface_path(const doc::EditableAuthoringEntry& entry,
                                              StrokeSettings settings) {
    const std::vector samples = editable_surface_path_samples(entry);
    const auto widths =
        std::ranges::minmax(entry.surface_points, {}, &doc::EditableSurfacePoint::width);
    settings.radius = widths.max.width;
    settings.input_mapping.pressure_radius.enabled = widths.min.width != widths.max.width;
    settings.input_mapping.pressure_radius.minimum_output = widths.min.width / widths.max.width;
    settings.input_mapping.pressure_radius.maximum_output = 1.0;
    StrokeResolver resolver(std::move(settings));
    resolver.append_samples(samples);
    return resolver.resolve();
}

}  // namespace ctex::paint
