#include <algorithm>
#include <array>
#include <cmath>
#include <ctex/pick/uv_index.hpp>
#include <limits>
#include <stdexcept>
#include <utility>

namespace ctex::pick {
namespace {

using Bounds = detail::UvBounds;
using Node = detail::UvNode;
using TriangleBuildData = detail::UvTriangleBuildData;

constexpr std::uint32_t leaf_triangle_capacity = 4;

float component(mesh::Vec2f value, std::size_t axis) { return axis == 0 ? value.x : value.y; }

mesh::Vec2f minimum(mesh::Vec2f left, mesh::Vec2f right) {
    return {std::min(left.x, right.x), std::min(left.y, right.y)};
}

mesh::Vec2f maximum(mesh::Vec2f left, mesh::Vec2f right) {
    return {std::max(left.x, right.x), std::max(left.y, right.y)};
}

mesh::Vec2f midpoint(mesh::Vec2f minimum_value, mesh::Vec2f maximum_value) {
    return {(minimum_value.x + maximum_value.x) * 0.5F, (minimum_value.y + maximum_value.y) * 0.5F};
}

Bounds empty_bounds() {
    const float infinity = std::numeric_limits<float>::infinity();
    return {{infinity, infinity}, {-infinity, -infinity}};
}

void include(Bounds& destination, const Bounds& source) {
    destination.minimum = minimum(destination.minimum, source.minimum);
    destination.maximum = maximum(destination.maximum, source.maximum);
}

std::size_t longest_axis(const Bounds& bounds) {
    return bounds.maximum.x - bounds.minimum.x >= bounds.maximum.y - bounds.minimum.y ? 0 : 1;
}

bool contains(const Bounds& bounds, mesh::Vec2f point) {
    return point.x >= bounds.minimum.x && point.x <= bounds.maximum.x &&
           point.y >= bounds.minimum.y && point.y <= bounds.maximum.y;
}

void validate_coordinate(mesh::Vec2f coordinate) {
    if (!std::isfinite(coordinate.x) || !std::isfinite(coordinate.y)) {
        throw std::invalid_argument("UV query requires a finite coordinate");
    }
}

}  // namespace

UvSpatialIndex::UvSpatialIndex(const mesh::MeshBinding& mesh, std::string_view uv_set,
                               std::pmr::memory_resource* memory_resource)
    : uv_set_(uv_set, memory_resource), nodes_(memory_resource), triangle_order_(memory_resource) {
    if (uv_set_.empty()) {
        throw std::invalid_argument("UV spatial index requires a named UV set");
    }
    build(mesh);
    build_count_ = 1;
}

bool UvSpatialIndex::synchronize(const mesh::MeshBinding& mesh) {
    if (mesh_revision_ == mesh.revision()) {
        return false;
    }
    UvSpatialIndex replacement(mesh, uv_set_, nodes_.get_allocator().resource());
    replacement.build_count_ = build_count_ + 1;
    *this = std::move(replacement);
    return true;
}

void UvSpatialIndex::build(const mesh::MeshBinding& mesh) {
    const auto& descriptor = mesh.view().descriptor();
    const auto& uv = mesh.view().uv_set(uv_set_).values;
    const std::size_t triangle_count = mesh.view().triangle_count();
    constexpr std::size_t maximum_triangle_count = std::numeric_limits<std::uint32_t>::max() / 2;
    if (triangle_count > maximum_triangle_count) {
        throw std::length_error("UV spatial index triangle count exceeds its 32-bit layout");
    }

    std::vector<TriangleBuildData> triangles;
    triangles.reserve(triangle_count);
    triangle_order_.resize(triangle_count);
    for (std::size_t triangle = 0; triangle < triangle_count; ++triangle) {
        const std::size_t first_index = triangle * 3;
        const mesh::Vec2f first = uv[descriptor.triangle_indices[first_index]];
        const mesh::Vec2f second = uv[descriptor.triangle_indices[first_index + 1]];
        const mesh::Vec2f third = uv[descriptor.triangle_indices[first_index + 2]];
        const Bounds bounds{minimum(minimum(first, second), third),
                            maximum(maximum(first, second), third)};
        triangles.push_back({bounds, midpoint(bounds.minimum, bounds.maximum)});
        triangle_order_[triangle] = static_cast<std::uint32_t>(triangle);
    }

    nodes_.reserve(triangle_count * 2);
    static_cast<void>(build_node(0, static_cast<std::uint32_t>(triangle_count), triangles));
    mesh_revision_ = mesh.revision();
}

std::uint32_t UvSpatialIndex::build_node(std::uint32_t first, std::uint32_t count,
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

UvCandidateQuery UvSpatialIndex::query_candidates(const mesh::MeshBinding& mesh,
                                                  mesh::Vec2f coordinate) {
    validate_coordinate(coordinate);
    static_cast<void>(synchronize(mesh));
    UvCandidateQuery result;
    result.triangle_indices.reserve(leaf_triangle_capacity);

    std::array<std::uint32_t, 64> stack{};
    std::size_t stack_size = 1;
    stack[0] = 0;
    while (stack_size != 0) {
        const Node& node = nodes_[stack[--stack_size]];
        ++result.visited_nodes;
        if (!contains(node.bounds, coordinate)) {
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
