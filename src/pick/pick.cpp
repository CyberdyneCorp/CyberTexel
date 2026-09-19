#include <algorithm>
#include <array>
#include <cmath>
#include <ctex/pick/spatial_index.hpp>
#include <limits>
#include <stdexcept>
#include <utility>

namespace ctex::pick {
namespace {

using Bounds = detail::SpatialBounds;
using Node = detail::SpatialNode;
using TriangleBuildData = detail::TriangleBuildData;

constexpr std::uint32_t leaf_triangle_capacity = 4;

float component(mesh::Vec3f value, std::size_t axis) {
    if (axis == 0) {
        return value.x;
    }
    if (axis == 1) {
        return value.y;
    }
    return value.z;
}

mesh::Vec3f minimum(mesh::Vec3f left, mesh::Vec3f right) {
    return {
        std::min(left.x, right.x),
        std::min(left.y, right.y),
        std::min(left.z, right.z),
    };
}

mesh::Vec3f maximum(mesh::Vec3f left, mesh::Vec3f right) {
    return {
        std::max(left.x, right.x),
        std::max(left.y, right.y),
        std::max(left.z, right.z),
    };
}

mesh::Vec3f midpoint(mesh::Vec3f minimum_value, mesh::Vec3f maximum_value) {
    return {
        (minimum_value.x + maximum_value.x) * 0.5F,
        (minimum_value.y + maximum_value.y) * 0.5F,
        (minimum_value.z + maximum_value.z) * 0.5F,
    };
}

Bounds empty_bounds() {
    const float infinity = std::numeric_limits<float>::infinity();
    return {{infinity, infinity, infinity}, {-infinity, -infinity, -infinity}};
}

void include(Bounds& destination, const Bounds& source) {
    destination.minimum = minimum(destination.minimum, source.minimum);
    destination.maximum = maximum(destination.maximum, source.maximum);
}

std::size_t longest_axis(const Bounds& bounds) {
    const std::array extents{
        bounds.maximum.x - bounds.minimum.x,
        bounds.maximum.y - bounds.minimum.y,
        bounds.maximum.z - bounds.minimum.z,
    };
    return static_cast<std::size_t>(
        std::distance(extents.begin(), std::max_element(extents.begin(), extents.end())));
}

bool ray_intersects_bounds(Ray ray, const Bounds& bounds, float maximum_distance) {
    float near_distance = 0.0F;
    float far_distance = maximum_distance;
    for (std::size_t axis = 0; axis < 3; ++axis) {
        const float origin = component(ray.origin, axis);
        const float direction = component(ray.direction, axis);
        const float lower = component(bounds.minimum, axis);
        const float upper = component(bounds.maximum, axis);
        if (direction == 0.0F) {
            if (origin < lower || origin > upper) {
                return false;
            }
            continue;
        }
        float first = (lower - origin) / direction;
        float second = (upper - origin) / direction;
        if (first > second) {
            std::swap(first, second);
        }
        near_distance = std::max(near_distance, first);
        far_distance = std::min(far_distance, second);
        if (near_distance > far_distance) {
            return false;
        }
    }
    return true;
}

void validate_ray_query(Ray ray, float maximum_distance) {
    const bool finite_ray = std::isfinite(ray.origin.x) && std::isfinite(ray.origin.y) &&
                            std::isfinite(ray.origin.z) && std::isfinite(ray.direction.x) &&
                            std::isfinite(ray.direction.y) && std::isfinite(ray.direction.z);
    if (!finite_ray ||
        (ray.direction.x == 0.0F && ray.direction.y == 0.0F && ray.direction.z == 0.0F) ||
        std::isnan(maximum_distance) || maximum_distance < 0.0F) {
        throw std::invalid_argument("ray query requires a direction and non-negative distance");
    }
}

double squared_distance(mesh::Vec3f point, const Bounds& bounds) {
    double result = 0.0;
    for (std::size_t axis = 0; axis < 3; ++axis) {
        const double value = component(point, axis);
        const double lower = component(bounds.minimum, axis);
        const double upper = component(bounds.maximum, axis);
        if (value < lower) {
            const double delta = lower - value;
            result += delta * delta;
        } else if (value > upper) {
            const double delta = value - upper;
            result += delta * delta;
        }
    }
    return result;
}

void validate_point_query(mesh::Vec3f point, float maximum_distance) {
    const bool finite_point =
        std::isfinite(point.x) && std::isfinite(point.y) && std::isfinite(point.z);
    if (!finite_point || std::isnan(maximum_distance) || maximum_distance < 0.0F) {
        throw std::invalid_argument(
            "point query requires a finite point and non-negative distance");
    }
}

bool finite(mesh::Vec3f value) {
    return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z);
}

void validate_box_query(mesh::Vec3f minimum_value, mesh::Vec3f maximum_value) {
    if (!finite(minimum_value) || !finite(maximum_value) || minimum_value.x > maximum_value.x ||
        minimum_value.y > maximum_value.y || minimum_value.z > maximum_value.z) {
        throw std::invalid_argument("box query requires finite ordered bounds");
    }
}

bool bounds_overlap(const Bounds& left, const Bounds& right) {
    return left.minimum.x <= right.maximum.x && left.maximum.x >= right.minimum.x &&
           left.minimum.y <= right.maximum.y && left.maximum.y >= right.minimum.y &&
           left.minimum.z <= right.maximum.z && left.maximum.z >= right.minimum.z;
}

void validate_half_spaces(std::span<const HalfSpace> half_spaces) {
    if (half_spaces.empty()) {
        throw std::invalid_argument("half-space query requires at least one plane");
    }
    for (const HalfSpace& plane : half_spaces) {
        if (!finite(plane.normal) || !std::isfinite(plane.offset) ||
            (plane.normal.x == 0.0F && plane.normal.y == 0.0F && plane.normal.z == 0.0F)) {
            throw std::invalid_argument("half-space query requires finite non-zero planes");
        }
    }
}

bool bounds_inside_half_spaces(const Bounds& bounds, std::span<const HalfSpace> half_spaces) {
    for (const HalfSpace& plane : half_spaces) {
        const mesh::Vec3f positive{
            plane.normal.x >= 0.0F ? bounds.maximum.x : bounds.minimum.x,
            plane.normal.y >= 0.0F ? bounds.maximum.y : bounds.minimum.y,
            plane.normal.z >= 0.0F ? bounds.maximum.z : bounds.minimum.z,
        };
        const double maximum_distance = static_cast<double>(plane.normal.x) * positive.x +
                                        static_cast<double>(plane.normal.y) * positive.y +
                                        static_cast<double>(plane.normal.z) * positive.z +
                                        plane.offset;
        if (maximum_distance < 0.0) {
            return false;
        }
    }
    return true;
}

}  // namespace

SpatialIndex::SpatialIndex(const mesh::MeshBinding& mesh) {
    build(mesh);
    build_count_ = 1;
}

bool SpatialIndex::synchronize(const mesh::MeshBinding& mesh) {
    if (mesh_revision_ == mesh.revision()) {
        return false;
    }
    SpatialIndex replacement(mesh);
    replacement.build_count_ = build_count_ + 1;
    *this = std::move(replacement);
    return true;
}

void SpatialIndex::build(const mesh::MeshBinding& mesh) {
    const auto& descriptor = mesh.view().descriptor();
    const std::size_t triangle_count = mesh.view().triangle_count();
    constexpr std::size_t maximum_triangle_count = std::numeric_limits<std::uint32_t>::max() / 2;
    if (triangle_count > maximum_triangle_count) {
        throw std::length_error("spatial index triangle count exceeds its 32-bit layout");
    }

    std::vector<TriangleBuildData> triangles;
    triangles.reserve(triangle_count);
    triangle_order_.resize(triangle_count);
    for (std::size_t triangle = 0; triangle < triangle_count; ++triangle) {
        const std::size_t first_index = triangle * 3;
        const mesh::Vec3f first = descriptor.positions[descriptor.triangle_indices[first_index]];
        const mesh::Vec3f second =
            descriptor.positions[descriptor.triangle_indices[first_index + 1]];
        const mesh::Vec3f third =
            descriptor.positions[descriptor.triangle_indices[first_index + 2]];
        const Bounds bounds{minimum(minimum(first, second), third),
                            maximum(maximum(first, second), third)};
        triangles.push_back({bounds, midpoint(bounds.minimum, bounds.maximum)});
        triangle_order_[triangle] = static_cast<std::uint32_t>(triangle);
    }

    nodes_.reserve(triangle_count * 2);
    static_cast<void>(build_node(0, static_cast<std::uint32_t>(triangle_count), triangles));
    mesh_revision_ = mesh.revision();
}

std::uint32_t SpatialIndex::build_node(std::uint32_t first, std::uint32_t count,
                                       const std::vector<TriangleBuildData>& triangles) {
    Bounds bounds = empty_bounds();
    Bounds centroid_bounds = empty_bounds();
    for (std::uint32_t offset = 0; offset < count; ++offset) {
        const auto& triangle = triangles[triangle_order_[first + offset]];
        include(bounds, triangle.bounds);
        include(centroid_bounds, {triangle.centroid, triangle.centroid});
    }

    const std::uint32_t node_index = static_cast<std::uint32_t>(nodes_.size());
    nodes_.push_back({bounds, first, count, 0});
    if (count <= leaf_triangle_capacity) {
        return node_index;
    }

    const std::size_t axis = longest_axis(centroid_bounds);
    const std::uint32_t left_count = count / 2;
    const auto begin = triangle_order_.begin() + first;
    const auto middle = begin + left_count;
    const auto end = begin + count;
    std::nth_element(begin, middle, end, [&](std::uint32_t left, std::uint32_t right) {
        const float left_value = component(triangles[left].centroid, axis);
        const float right_value = component(triangles[right].centroid, axis);
        return left_value == right_value ? left < right : left_value < right_value;
    });

    const std::uint32_t left_child = build_node(first, left_count, triangles);
    const std::uint32_t right_child = build_node(first + left_count, count - left_count, triangles);
    nodes_[node_index].first = left_child;
    nodes_[node_index].count = 0;
    nodes_[node_index].right_child = right_child;
    return node_index;
}

RayCandidateQuery SpatialIndex::query_ray_candidates(const mesh::MeshBinding& mesh, Ray ray,
                                                     float maximum_distance) {
    static_cast<void>(synchronize(mesh));
    validate_ray_query(ray, maximum_distance);
    RayCandidateQuery result;
    result.triangle_indices.reserve(leaf_triangle_capacity);

    std::array<std::uint32_t, 64> stack{};
    std::size_t stack_size = 1;
    stack[0] = 0;
    while (stack_size != 0) {
        const Node& node = nodes_[stack[--stack_size]];
        ++result.visited_nodes;
        if (!ray_intersects_bounds(ray, node.bounds, maximum_distance)) {
            continue;
        }
        if (node.count != 0) {
            result.tested_leaf_triangles += node.count;
            result.triangle_indices.insert(result.triangle_indices.end(),
                                           triangle_order_.begin() + node.first,
                                           triangle_order_.begin() + node.first + node.count);
            continue;
        }
        stack[stack_size++] = node.right_child;
        stack[stack_size++] = node.first;
    }
    return result;
}

PointCandidateQuery SpatialIndex::query_point_candidates(const mesh::MeshBinding& mesh,
                                                         mesh::Vec3f point,
                                                         float maximum_distance) {
    validate_point_query(point, maximum_distance);
    static_cast<void>(synchronize(mesh));
    const double limit_squared = static_cast<double>(maximum_distance) * maximum_distance;
    PointCandidateQuery result;
    result.triangle_indices.reserve(leaf_triangle_capacity);

    std::array<std::uint32_t, 64> stack{};
    std::size_t stack_size = 1;
    stack[0] = 0;
    while (stack_size != 0) {
        const Node& node = nodes_[stack[--stack_size]];
        ++result.visited_nodes;
        if (squared_distance(point, node.bounds) > limit_squared) {
            continue;
        }
        if (node.count != 0) {
            result.tested_leaf_triangles += node.count;
            result.triangle_indices.insert(result.triangle_indices.end(),
                                           triangle_order_.begin() + node.first,
                                           triangle_order_.begin() + node.first + node.count);
            continue;
        }
        stack[stack_size++] = node.right_child;
        stack[stack_size++] = node.first;
    }
    return result;
}

RegionCandidateQuery SpatialIndex::query_box_candidates(const mesh::MeshBinding& mesh,
                                                        mesh::Vec3f minimum_value,
                                                        mesh::Vec3f maximum_value) {
    validate_box_query(minimum_value, maximum_value);
    static_cast<void>(synchronize(mesh));
    const Bounds query_bounds{minimum_value, maximum_value};
    RegionCandidateQuery result;
    result.triangle_indices.reserve(leaf_triangle_capacity);

    std::array<std::uint32_t, 64> stack{};
    std::size_t stack_size = 1;
    stack[0] = 0;
    while (stack_size != 0) {
        const Node& node = nodes_[stack[--stack_size]];
        ++result.visited_nodes;
        if (!bounds_overlap(node.bounds, query_bounds)) {
            continue;
        }
        if (node.count != 0) {
            result.tested_leaf_triangles += node.count;
            result.triangle_indices.insert(result.triangle_indices.end(),
                                           triangle_order_.begin() + node.first,
                                           triangle_order_.begin() + node.first + node.count);
            continue;
        }
        stack[stack_size++] = node.right_child;
        stack[stack_size++] = node.first;
    }
    return result;
}

RegionCandidateQuery SpatialIndex::query_half_space_candidates(
    const mesh::MeshBinding& mesh, std::span<const HalfSpace> half_spaces) {
    validate_half_spaces(half_spaces);
    static_cast<void>(synchronize(mesh));
    RegionCandidateQuery result;
    result.triangle_indices.reserve(leaf_triangle_capacity);

    std::array<std::uint32_t, 64> stack{};
    std::size_t stack_size = 1;
    stack[0] = 0;
    while (stack_size != 0) {
        const Node& node = nodes_[stack[--stack_size]];
        ++result.visited_nodes;
        if (!bounds_inside_half_spaces(node.bounds, half_spaces)) {
            continue;
        }
        if (node.count != 0) {
            result.tested_leaf_triangles += node.count;
            result.triangle_indices.insert(result.triangle_indices.end(),
                                           triangle_order_.begin() + node.first,
                                           triangle_order_.begin() + node.first + node.count);
            continue;
        }
        stack[stack_size++] = node.right_child;
        stack[stack_size++] = node.first;
    }
    return result;
}

}  // namespace ctex::pick
