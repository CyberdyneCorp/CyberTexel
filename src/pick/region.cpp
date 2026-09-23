#include <algorithm>
#include <array>
#include <cmath>
#include <ctex/pick/region.hpp>
#include <ctex/pick/spatial_index.hpp>
#include <iterator>
#include <stdexcept>

namespace ctex::pick {
namespace {

constexpr double epsilon = 1.0e-10;
constexpr double geometry_epsilon = 1.0e-24;
constexpr std::size_t maximum_clipped_vertices = 12;

struct Vec3d {
    double x;
    double y;
    double z;
};

struct Vec4d {
    double x;
    double y;
    double z;
    double w;
};

struct Point2d {
    double x;
    double y;
};

using ClipPlane = std::array<double, 4>;
using ClipPolygon = std::array<Vec4d, maximum_clipped_vertices>;
using WorldPolygon = std::array<Vec3d, maximum_clipped_vertices>;

constexpr std::array canonical_clip_planes{
    ClipPlane{1.0, 0.0, 0.0, 1.0},  ClipPlane{-1.0, 0.0, 0.0, 1.0}, ClipPlane{0.0, 1.0, 0.0, 1.0},
    ClipPlane{0.0, -1.0, 0.0, 1.0}, ClipPlane{0.0, 0.0, 1.0, 1.0},  ClipPlane{0.0, 0.0, -1.0, 1.0},
};

bool finite(mesh::Vec3f value) {
    return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z);
}

bool finite(const Mat4f& matrix) {
    return std::all_of(matrix.values.begin(), matrix.values.end(),
                       [](float value) { return std::isfinite(value); });
}

Mat4f multiply(const Mat4f& left, const Mat4f& right) {
    Mat4f result{};
    for (std::size_t column = 0; column < 4; ++column) {
        for (std::size_t row = 0; row < 4; ++row) {
            double value = 0.0;
            for (std::size_t inner = 0; inner < 4; ++inner) {
                value += static_cast<double>(left.values[inner * 4 + row]) *
                         right.values[column * 4 + inner];
            }
            result.values[column * 4 + row] = static_cast<float>(value);
        }
    }
    return result;
}

Vec4d transform(const Mat4f& matrix, mesh::Vec3f point) {
    return {
        matrix.values[0] * point.x + matrix.values[4] * point.y + matrix.values[8] * point.z +
            matrix.values[12],
        matrix.values[1] * point.x + matrix.values[5] * point.y + matrix.values[9] * point.z +
            matrix.values[13],
        matrix.values[2] * point.x + matrix.values[6] * point.y + matrix.values[10] * point.z +
            matrix.values[14],
        matrix.values[3] * point.x + matrix.values[7] * point.y + matrix.values[11] * point.z +
            matrix.values[15],
    };
}

double evaluate(Vec4d point, const ClipPlane& plane) {
    return plane[0] * point.x + plane[1] * point.y + plane[2] * point.z + plane[3] * point.w;
}

Vec4d interpolate(Vec4d first, Vec4d second, double fraction) {
    return {
        first.x + (second.x - first.x) * fraction,
        first.y + (second.y - first.y) * fraction,
        first.z + (second.z - first.z) * fraction,
        first.w + (second.w - first.w) * fraction,
    };
}

std::size_t clip_once(const ClipPolygon& input, std::size_t input_count, ClipPolygon& output,
                      const ClipPlane& plane) {
    std::size_t output_count = 0;
    Vec4d previous = input[input_count - 1];
    double previous_distance = evaluate(previous, plane);
    for (std::size_t index = 0; index < input_count; ++index) {
        const Vec4d current = input[index];
        const double current_distance = evaluate(current, plane);
        const bool previous_inside = previous_distance >= -epsilon;
        const bool current_inside = current_distance >= -epsilon;
        if (previous_inside != current_inside) {
            const double fraction = previous_distance / (previous_distance - current_distance);
            output[output_count++] = interpolate(previous, current, fraction);
        }
        if (current_inside) {
            output[output_count++] = current;
        }
        previous = current;
        previous_distance = current_distance;
    }
    return output_count;
}

std::pair<ClipPolygon, std::size_t> clipped_triangle(const Mat4f& view_projection,
                                                     mesh::Vec3f first, mesh::Vec3f second,
                                                     mesh::Vec3f third) {
    ClipPolygon current{};
    ClipPolygon scratch{};
    current[0] = transform(view_projection, first);
    current[1] = transform(view_projection, second);
    current[2] = transform(view_projection, third);
    std::size_t count = 3;
    for (const ClipPlane& plane : canonical_clip_planes) {
        count = clip_once(current, count, scratch, plane);
        if (count == 0) {
            break;
        }
        std::swap(current, scratch);
    }
    return {current, count};
}

Point2d screen_point(Vec4d clip, ViewportSize viewport) {
    const double ndc_x = clip.x / clip.w;
    const double ndc_y = clip.y / clip.w;
    return {(ndc_x + 1.0) * 0.5 * viewport.width, (1.0 - ndc_y) * 0.5 * viewport.height};
}

double orientation(Point2d first, Point2d second, Point2d third) {
    return (second.x - first.x) * (third.y - first.y) - (second.y - first.y) * (third.x - first.x);
}

bool on_segment(Point2d point, Point2d first, Point2d second) {
    if (std::abs(orientation(first, second, point)) > epsilon) {
        return false;
    }
    return point.x >= std::min(first.x, second.x) - epsilon &&
           point.x <= std::max(first.x, second.x) + epsilon &&
           point.y >= std::min(first.y, second.y) - epsilon &&
           point.y <= std::max(first.y, second.y) + epsilon;
}

bool segments_intersect(Point2d first_a, Point2d second_a, Point2d first_b, Point2d second_b) {
    const double first_side = orientation(first_a, second_a, first_b);
    const double second_side = orientation(first_a, second_a, second_b);
    const double third_side = orientation(first_b, second_b, first_a);
    const double fourth_side = orientation(first_b, second_b, second_a);
    if (((first_side > epsilon && second_side < -epsilon) ||
         (first_side < -epsilon && second_side > epsilon)) &&
        ((third_side > epsilon && fourth_side < -epsilon) ||
         (third_side < -epsilon && fourth_side > epsilon))) {
        return true;
    }
    return (std::abs(first_side) <= epsilon && on_segment(first_b, first_a, second_a)) ||
           (std::abs(second_side) <= epsilon && on_segment(second_b, first_a, second_a)) ||
           (std::abs(third_side) <= epsilon && on_segment(first_a, first_b, second_b)) ||
           (std::abs(fourth_side) <= epsilon && on_segment(second_a, first_b, second_b));
}

bool point_in_polygon(Point2d point, std::span<const Point2d> polygon) {
    bool inside = false;
    for (std::size_t index = 0, previous = polygon.size() - 1; index < polygon.size();
         previous = index++) {
        const Point2d first = polygon[previous];
        const Point2d second = polygon[index];
        if (on_segment(point, first, second)) {
            return true;
        }
        const bool crosses = (first.y > point.y) != (second.y > point.y);
        if (crosses &&
            point.x < (second.x - first.x) * (point.y - first.y) / (second.y - first.y) + first.x) {
            inside = !inside;
        }
    }
    return inside;
}

bool polygons_intersect(std::span<const Point2d> left, std::span<const Point2d> right) {
    for (Point2d point : left) {
        if (point_in_polygon(point, right)) {
            return true;
        }
    }
    for (Point2d point : right) {
        if (point_in_polygon(point, left)) {
            return true;
        }
    }
    for (std::size_t left_index = 0; left_index < left.size(); ++left_index) {
        const Point2d left_next = left[(left_index + 1) % left.size()];
        for (std::size_t right_index = 0; right_index < right.size(); ++right_index) {
            const Point2d right_next = right[(right_index + 1) % right.size()];
            if (segments_intersect(left[left_index], left_next, right[right_index], right_next)) {
                return true;
            }
        }
    }
    return false;
}

HalfSpace world_half_space(const Mat4f& view_projection, const ClipPlane& clip_plane) {
    std::array<double, 4> coefficients{};
    for (std::size_t column = 0; column < 4; ++column) {
        for (std::size_t row = 0; row < 4; ++row) {
            coefficients[column] += clip_plane[row] * view_projection.values[column * 4 + row];
        }
    }
    return {{static_cast<float>(coefficients[0]), static_cast<float>(coefficients[1]),
             static_cast<float>(coefficients[2])},
            static_cast<float>(coefficients[3])};
}

std::array<HalfSpace, 6> selection_frustum(const Mat4f& view_projection, ScreenRectangle bounds,
                                           ViewportSize viewport) {
    const double left = 2.0 * bounds.minimum.x / viewport.width - 1.0;
    const double right = 2.0 * bounds.maximum.x / viewport.width - 1.0;
    const double top = 1.0 - 2.0 * bounds.minimum.y / viewport.height;
    const double bottom = 1.0 - 2.0 * bounds.maximum.y / viewport.height;
    const std::array clip_planes{
        ClipPlane{1.0, 0.0, 0.0, -left},   ClipPlane{-1.0, 0.0, 0.0, right},
        ClipPlane{0.0, 1.0, 0.0, -bottom}, ClipPlane{0.0, -1.0, 0.0, top},
        canonical_clip_planes[4],          canonical_clip_planes[5],
    };
    std::array<HalfSpace, 6> result{};
    std::transform(
        clip_planes.begin(), clip_planes.end(), result.begin(),
        [&](const ClipPlane& plane) { return world_half_space(view_projection, plane); });
    return result;
}

void validate_screen_view(const ScreenRegionView& view) {
    if (view.viewport.width == 0 || view.viewport.height == 0 || !finite(view.view) ||
        !finite(view.projection)) {
        throw std::invalid_argument("screen region query requires a finite view and viewport");
    }
}

bool valid_screen_point(ScreenPosition point, ViewportSize viewport) {
    return std::isfinite(point.x) && std::isfinite(point.y) && point.x >= 0.0F && point.y >= 0.0F &&
           point.x <= viewport.width && point.y <= viewport.height;
}

void validate_rectangle(ScreenRectangle rectangle, const ScreenRegionView& view) {
    validate_screen_view(view);
    if (!valid_screen_point(rectangle.minimum, view.viewport) ||
        !valid_screen_point(rectangle.maximum, view.viewport) ||
        rectangle.minimum.x >= rectangle.maximum.x || rectangle.minimum.y >= rectangle.maximum.y) {
        throw std::invalid_argument("screen rectangle requires positive ordered viewport bounds");
    }
}

ScreenRectangle lasso_bounds(std::span<const ScreenPosition> points, const ScreenRegionView& view) {
    validate_screen_view(view);
    if (points.size() < 3) {
        throw std::invalid_argument("screen lasso requires at least three points");
    }
    ScreenRectangle result{points.front(), points.front()};
    double twice_area = 0.0;
    for (std::size_t index = 0; index < points.size(); ++index) {
        const ScreenPosition point = points[index];
        if (!valid_screen_point(point, view.viewport)) {
            throw std::invalid_argument(
                "screen lasso points must be finite and inside the viewport");
        }
        result.minimum.x = std::min(result.minimum.x, point.x);
        result.minimum.y = std::min(result.minimum.y, point.y);
        result.maximum.x = std::max(result.maximum.x, point.x);
        result.maximum.y = std::max(result.maximum.y, point.y);
        const ScreenPosition next = points[(index + 1) % points.size()];
        twice_area += static_cast<double>(point.x) * next.y - static_cast<double>(next.x) * point.y;
    }
    if (std::abs(twice_area) <= epsilon) {
        throw std::invalid_argument("screen lasso requires a non-zero enclosed area");
    }
    return result;
}

RegionQueryResult screen_query(SpatialIndex& index, const mesh::MeshBinding& mesh,
                               std::span<const Point2d> region, ScreenRectangle bounds,
                               const ScreenRegionView& view) {
    const Mat4f view_projection = multiply(view.projection, view.view);
    const auto frustum = selection_frustum(view_projection, bounds, view.viewport);
    const RegionCandidateQuery broad = index.query_half_space_candidates(mesh, frustum);
    RegionQueryResult result{{}, broad.visited_nodes, broad.tested_leaf_triangles};
    const auto& descriptor = mesh.view().descriptor();
    std::array<Point2d, maximum_clipped_vertices> projected{};
    for (std::uint32_t triangle : broad.triangle_indices) {
        const std::size_t first_index = static_cast<std::size_t>(triangle) * 3;
        const auto [clipped, count] = clipped_triangle(
            view_projection, descriptor.positions[descriptor.triangle_indices[first_index]],
            descriptor.positions[descriptor.triangle_indices[first_index + 1]],
            descriptor.positions[descriptor.triangle_indices[first_index + 2]]);
        for (std::size_t point = 0; point < count; ++point) {
            projected[point] = screen_point(clipped[point], view.viewport);
        }
        if (count != 0 && polygons_intersect(region, {projected.data(), count})) {
            result.triangle_indices.push_back(triangle);
        }
    }
    std::sort(result.triangle_indices.begin(), result.triangle_indices.end());
    return result;
}

Vec3d as_double(mesh::Vec3f value) { return {value.x, value.y, value.z}; }

Vec3d subtract(Vec3d left, Vec3d right) {
    return {left.x - right.x, left.y - right.y, left.z - right.z};
}

Vec3d add(Vec3d left, Vec3d right) {
    return {left.x + right.x, left.y + right.y, left.z + right.z};
}

Vec3d multiply(Vec3d value, double factor) {
    return {value.x * factor, value.y * factor, value.z * factor};
}

double dot(Vec3d left, Vec3d right) {
    return left.x * right.x + left.y * right.y + left.z * right.z;
}

double segment_distance_squared(Vec3d point, Vec3d first, Vec3d second) {
    const Vec3d edge = subtract(second, first);
    const double length_squared = dot(edge, edge);
    const double fraction =
        length_squared <= geometry_epsilon
            ? 0.0
            : std::clamp(dot(subtract(point, first), edge) / length_squared, 0.0, 1.0);
    const Vec3d delta = subtract(point, add(first, multiply(edge, fraction)));
    return dot(delta, delta);
}

double triangle_distance_squared(Vec3d point, Vec3d first, Vec3d second, Vec3d third) {
    const Vec3d first_edge = subtract(second, first);
    const Vec3d second_edge = subtract(third, first);
    const Vec3d offset = subtract(point, first);
    const double a = dot(first_edge, first_edge);
    const double b = dot(first_edge, second_edge);
    const double c = dot(second_edge, second_edge);
    const double d = dot(first_edge, offset);
    const double e = dot(second_edge, offset);
    const double determinant = a * c - b * b;
    if (determinant <= geometry_epsilon) {
        return std::min({segment_distance_squared(point, first, second),
                         segment_distance_squared(point, second, third),
                         segment_distance_squared(point, third, first)});
    }
    const double first_parameter = (d * c - e * b) / determinant;
    const double second_parameter = (e * a - d * b) / determinant;
    if (first_parameter >= 0.0 && second_parameter >= 0.0 &&
        first_parameter + second_parameter <= 1.0) {
        const Vec3d closest = add(first, add(multiply(first_edge, first_parameter),
                                             multiply(second_edge, second_parameter)));
        const Vec3d delta = subtract(point, closest);
        return dot(delta, delta);
    }
    return std::min({segment_distance_squared(point, first, second),
                     segment_distance_squared(point, second, third),
                     segment_distance_squared(point, third, first)});
}

double component(Vec3d value, std::size_t axis) {
    return axis == 0 ? value.x : (axis == 1 ? value.y : value.z);
}

std::size_t clip_world_once(const WorldPolygon& input, std::size_t input_count,
                            WorldPolygon& output, std::size_t axis, double boundary,
                            bool keep_above) {
    std::size_t output_count = 0;
    Vec3d previous = input[input_count - 1];
    double previous_distance =
        keep_above ? component(previous, axis) - boundary : boundary - component(previous, axis);
    for (std::size_t index = 0; index < input_count; ++index) {
        const Vec3d current = input[index];
        const double current_distance =
            keep_above ? component(current, axis) - boundary : boundary - component(current, axis);
        const bool previous_inside = previous_distance >= -epsilon;
        const bool current_inside = current_distance >= -epsilon;
        if (previous_inside != current_inside) {
            const double fraction = previous_distance / (previous_distance - current_distance);
            output[output_count++] = add(previous, multiply(subtract(current, previous), fraction));
        }
        if (current_inside) {
            output[output_count++] = current;
        }
        previous = current;
        previous_distance = current_distance;
    }
    return output_count;
}

bool triangle_intersects_box(Vec3d first, Vec3d second, Vec3d third, WorldBox box) {
    WorldPolygon current{};
    WorldPolygon scratch{};
    current[0] = first;
    current[1] = second;
    current[2] = third;
    std::size_t count = 3;
    for (std::size_t axis = 0; axis < 3 && count != 0; ++axis) {
        count = clip_world_once(current, count, scratch, axis,
                                component(as_double(box.minimum), axis), true);
        std::swap(current, scratch);
        if (count != 0) {
            count = clip_world_once(current, count, scratch, axis,
                                    component(as_double(box.maximum), axis), false);
            std::swap(current, scratch);
        }
    }
    return count != 0;
}

void validate_sphere(WorldSphere sphere) {
    if (!finite(sphere.center) || !std::isfinite(sphere.radius) || sphere.radius < 0.0F) {
        throw std::invalid_argument(
            "world sphere requires a finite center and non-negative radius");
    }
}

void validate_box(WorldBox box) {
    if (!finite(box.minimum) || !finite(box.maximum) || box.minimum.x > box.maximum.x ||
        box.minimum.y > box.maximum.y || box.minimum.z > box.maximum.z) {
        throw std::invalid_argument("world box requires finite ordered bounds");
    }
}

}  // namespace

RegionQueryResult query_screen_rectangle(SpatialIndex& index, const mesh::MeshBinding& mesh,
                                         ScreenRectangle rectangle, const ScreenRegionView& view) {
    validate_rectangle(rectangle, view);
    const std::array region{
        Point2d{rectangle.minimum.x, rectangle.minimum.y},
        Point2d{rectangle.minimum.x, rectangle.maximum.y},
        Point2d{rectangle.maximum.x, rectangle.maximum.y},
        Point2d{rectangle.maximum.x, rectangle.minimum.y},
    };
    return screen_query(index, mesh, region, rectangle, view);
}

RegionQueryResult query_screen_lasso(SpatialIndex& index, const mesh::MeshBinding& mesh,
                                     std::span<const ScreenPosition> points,
                                     const ScreenRegionView& view) {
    const ScreenRectangle bounds = lasso_bounds(points, view);
    std::vector<Point2d> region;
    region.reserve(points.size());
    std::transform(points.begin(), points.end(), std::back_inserter(region),
                   [](ScreenPosition point) { return Point2d{point.x, point.y}; });
    return screen_query(index, mesh, region, bounds, view);
}

RegionQueryResult query_world_sphere(SpatialIndex& index, const mesh::MeshBinding& mesh,
                                     WorldSphere sphere) {
    validate_sphere(sphere);
    const RegionCandidateQuery broad =
        index.query_point_candidates(mesh, sphere.center, sphere.radius);
    RegionQueryResult result{{}, broad.visited_nodes, broad.tested_leaf_triangles};
    const auto& descriptor = mesh.view().descriptor();
    const double radius_squared = static_cast<double>(sphere.radius) * sphere.radius;
    for (std::uint32_t triangle : broad.triangle_indices) {
        const std::size_t first_index = static_cast<std::size_t>(triangle) * 3;
        if (triangle_distance_squared(
                as_double(sphere.center),
                as_double(descriptor.positions[descriptor.triangle_indices[first_index]]),
                as_double(descriptor.positions[descriptor.triangle_indices[first_index + 1]]),
                as_double(descriptor.positions[descriptor.triangle_indices[first_index + 2]])) <=
            radius_squared) {
            result.triangle_indices.push_back(triangle);
        }
    }
    std::sort(result.triangle_indices.begin(), result.triangle_indices.end());
    return result;
}

RegionQueryResult query_world_box(SpatialIndex& index, const mesh::MeshBinding& mesh,
                                  WorldBox box) {
    validate_box(box);
    const RegionCandidateQuery broad = index.query_box_candidates(mesh, box.minimum, box.maximum);
    RegionQueryResult result{{}, broad.visited_nodes, broad.tested_leaf_triangles};
    const auto& descriptor = mesh.view().descriptor();
    for (std::uint32_t triangle : broad.triangle_indices) {
        const std::size_t first_index = static_cast<std::size_t>(triangle) * 3;
        if (triangle_intersects_box(
                as_double(descriptor.positions[descriptor.triangle_indices[first_index]]),
                as_double(descriptor.positions[descriptor.triangle_indices[first_index + 1]]),
                as_double(descriptor.positions[descriptor.triangle_indices[first_index + 2]]),
                box)) {
            result.triangle_indices.push_back(triangle);
        }
    }
    std::sort(result.triangle_indices.begin(), result.triangle_indices.end());
    return result;
}

}  // namespace ctex::pick
