#include <algorithm>
#include <cmath>
#include <ctex/pick/hit.hpp>
#include <ctex/pick/spatial_index.hpp>
#include <ctex/pick/uv_index.hpp>
#include <limits>
#include <optional>
#include <stdexcept>
#include <utility>

namespace ctex::pick {
namespace {

struct Vec3d {
    double x;
    double y;
    double z;
};

struct TriangleIntersection {
    double distance;
    Vec3d barycentric;
};

struct ExactCandidate {
    std::uint32_t triangle;
    TriangleIntersection intersection;
};

struct ClosestPoint {
    Vec3d position;
    Vec3d barycentric;
    double squared_distance;
};

struct SnapCandidate {
    std::uint32_t triangle;
    ClosestPoint closest;
};

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

Vec3d cross(Vec3d left, Vec3d right) {
    return {
        left.y * right.z - left.z * right.y,
        left.z * right.x - left.x * right.z,
        left.x * right.y - left.y * right.x,
    };
}

double dot(Vec3d left, Vec3d right) {
    return left.x * right.x + left.y * right.y + left.z * right.z;
}

mesh::Vec3f normalize(Vec3d value, mesh::Vec3f fallback = {}) {
    const double length = std::sqrt(dot(value, value));
    if (!std::isfinite(length) || length <= 1.0e-12) {
        return fallback;
    }
    return {
        static_cast<float>(value.x / length),
        static_cast<float>(value.y / length),
        static_cast<float>(value.z / length),
    };
}

Ray normalized_ray(Ray ray) {
    const mesh::Vec3f direction = normalize(as_double(ray.direction));
    if (direction == mesh::Vec3f{}) {
        throw std::invalid_argument("exact pick requires a finite non-zero ray direction");
    }
    ray.direction = direction;
    return ray;
}

std::optional<TriangleIntersection> intersect_triangle(Ray ray, mesh::Vec3f first,
                                                       mesh::Vec3f second, mesh::Vec3f third,
                                                       float maximum_distance,
                                                       BackfacePolicy backfaces) {
    constexpr double epsilon = 1.0e-10;
    const Vec3d origin = as_double(ray.origin);
    const Vec3d direction = as_double(ray.direction);
    const Vec3d first_d = as_double(first);
    const Vec3d edge_one = subtract(as_double(second), first_d);
    const Vec3d edge_two = subtract(as_double(third), first_d);
    const Vec3d perpendicular = cross(direction, edge_two);
    const double determinant = dot(edge_one, perpendicular);
    const bool rejected = backfaces == BackfacePolicy::reject ? determinant <= epsilon
                                                              : std::abs(determinant) <= epsilon;
    if (rejected) {
        return std::nullopt;
    }

    const double inverse_determinant = 1.0 / determinant;
    const Vec3d origin_offset = subtract(origin, first_d);
    const double second_weight = dot(origin_offset, perpendicular) * inverse_determinant;
    if (second_weight < -epsilon || second_weight > 1.0 + epsilon) {
        return std::nullopt;
    }
    const Vec3d direction_cross = cross(origin_offset, edge_one);
    const double third_weight = dot(direction, direction_cross) * inverse_determinant;
    if (third_weight < -epsilon || second_weight + third_weight > 1.0 + epsilon) {
        return std::nullopt;
    }
    const double distance = dot(edge_two, direction_cross) * inverse_determinant;
    if (distance < 0.0 || distance > maximum_distance) {
        return std::nullopt;
    }

    const double clamped_second = std::clamp(second_weight, 0.0, 1.0);
    const double clamped_third = std::clamp(third_weight, 0.0, 1.0);
    const double first_weight = std::max(0.0, 1.0 - clamped_second - clamped_third);
    const double weight_sum = first_weight + clamped_second + clamped_third;
    return TriangleIntersection{
        distance,
        {first_weight / weight_sum, clamped_second / weight_sum, clamped_third / weight_sum},
    };
}

bool comes_before(const ExactCandidate& left, const ExactCandidate& right) {
    return left.intersection.distance < right.intersection.distance ||
           (left.intersection.distance == right.intersection.distance &&
            left.triangle < right.triangle);
}

bool equivalent_distance(double left, double right) {
    constexpr double relative_tolerance = 1.0e-10;
    return std::abs(left - right) <=
           relative_tolerance * std::max({1.0, std::abs(left), std::abs(right)});
}

bool preferred_nearest(const ExactCandidate& candidate, const ExactCandidate& current) {
    if (equivalent_distance(candidate.intersection.distance, current.intersection.distance)) {
        return candidate.triangle < current.triangle;
    }
    return candidate.intersection.distance < current.intersection.distance;
}

mesh::Vec3f interpolate(mesh::Vec3f first, mesh::Vec3f second, mesh::Vec3f third, Vec3d weights) {
    return {
        static_cast<float>(first.x * weights.x + second.x * weights.y + third.x * weights.z),
        static_cast<float>(first.y * weights.x + second.y * weights.y + third.y * weights.z),
        static_cast<float>(first.z * weights.x + second.z * weights.y + third.z * weights.z),
    };
}

mesh::Vec2f interpolate(mesh::Vec2f first, mesh::Vec2f second, mesh::Vec2f third, Vec3d weights) {
    return {
        static_cast<float>(first.x * weights.x + second.x * weights.y + third.x * weights.z),
        static_cast<float>(first.y * weights.x + second.y * weights.y + third.y * weights.z),
    };
}

mesh::Vec3f as_float(Vec3d value) {
    return {static_cast<float>(value.x), static_cast<float>(value.y), static_cast<float>(value.z)};
}

ClosestPoint closest_with_barycentric(Vec3d point, Vec3d first, Vec3d second, Vec3d third,
                                      Vec3d barycentric) {
    const Vec3d position = add(add(multiply(first, barycentric.x), multiply(second, barycentric.y)),
                               multiply(third, barycentric.z));
    const Vec3d delta = subtract(point, position);
    return {position, barycentric, dot(delta, delta)};
}

ClosestPoint closest_on_segment(Vec3d point, Vec3d first, Vec3d second, Vec3d first_barycentric,
                                Vec3d second_barycentric) {
    const Vec3d edge = subtract(second, first);
    const double length_squared = dot(edge, edge);
    const double fraction =
        length_squared <= 1.0e-24
            ? 0.0
            : std::clamp(dot(subtract(point, first), edge) / length_squared, 0.0, 1.0);
    const Vec3d barycentric =
        add(multiply(first_barycentric, 1.0 - fraction), multiply(second_barycentric, fraction));
    const Vec3d position = add(first, multiply(edge, fraction));
    const Vec3d delta = subtract(point, position);
    return {position, barycentric, dot(delta, delta)};
}

ClosestPoint closest_on_degenerate_triangle(Vec3d point, Vec3d first, Vec3d second, Vec3d third) {
    ClosestPoint result =
        closest_on_segment(point, first, second, {1.0, 0.0, 0.0}, {0.0, 1.0, 0.0});
    const ClosestPoint second_edge =
        closest_on_segment(point, second, third, {0.0, 1.0, 0.0}, {0.0, 0.0, 1.0});
    if (second_edge.squared_distance < result.squared_distance) {
        result = second_edge;
    }
    const ClosestPoint third_edge =
        closest_on_segment(point, third, first, {0.0, 0.0, 1.0}, {1.0, 0.0, 0.0});
    if (third_edge.squared_distance < result.squared_distance) {
        result = third_edge;
    }
    return result;
}

ClosestPoint closest_on_triangle(Vec3d point, Vec3d first, Vec3d second, Vec3d third) {
    const Vec3d edge_one = subtract(second, first);
    const Vec3d edge_two = subtract(third, first);
    if (dot(cross(edge_one, edge_two), cross(edge_one, edge_two)) <= 1.0e-24) {
        return closest_on_degenerate_triangle(point, first, second, third);
    }

    const Vec3d first_offset = subtract(point, first);
    const double d1 = dot(edge_one, first_offset);
    const double d2 = dot(edge_two, first_offset);
    if (d1 <= 0.0 && d2 <= 0.0) {
        return closest_with_barycentric(point, first, second, third, {1.0, 0.0, 0.0});
    }

    const Vec3d second_offset = subtract(point, second);
    const double d3 = dot(edge_one, second_offset);
    const double d4 = dot(edge_two, second_offset);
    if (d3 >= 0.0 && d4 <= d3) {
        return closest_with_barycentric(point, first, second, third, {0.0, 1.0, 0.0});
    }
    const double vc = d1 * d4 - d3 * d2;
    if (vc <= 0.0 && d1 >= 0.0 && d3 <= 0.0) {
        const double fraction = d1 / (d1 - d3);
        return closest_with_barycentric(point, first, second, third,
                                        {1.0 - fraction, fraction, 0.0});
    }

    const Vec3d third_offset = subtract(point, third);
    const double d5 = dot(edge_one, third_offset);
    const double d6 = dot(edge_two, third_offset);
    if (d6 >= 0.0 && d5 <= d6) {
        return closest_with_barycentric(point, first, second, third, {0.0, 0.0, 1.0});
    }
    const double vb = d5 * d2 - d1 * d6;
    if (vb <= 0.0 && d2 >= 0.0 && d6 <= 0.0) {
        const double fraction = d2 / (d2 - d6);
        return closest_with_barycentric(point, first, second, third,
                                        {1.0 - fraction, 0.0, fraction});
    }
    const double va = d3 * d6 - d5 * d4;
    if (va <= 0.0 && d4 - d3 >= 0.0 && d5 - d6 >= 0.0) {
        const double fraction = (d4 - d3) / ((d4 - d3) + (d5 - d6));
        return closest_with_barycentric(point, first, second, third,
                                        {0.0, 1.0 - fraction, fraction});
    }

    const double denominator = 1.0 / (va + vb + vc);
    const double second_weight = vb * denominator;
    const double third_weight = vc * denominator;
    return closest_with_barycentric(
        point, first, second, third,
        {1.0 - second_weight - third_weight, second_weight, third_weight});
}

const TextureSetBindingView& texture_binding_for_partition(
    std::span<const TextureSetBindingView> bindings, std::uint32_t partition_index) {
    const TextureSetBindingView* result = nullptr;
    for (const TextureSetBindingView& binding : bindings) {
        if (binding.partition_index != partition_index) {
            continue;
        }
        if (result != nullptr) {
            throw std::invalid_argument("pick has duplicate texture-set bindings for a partition");
        }
        result = &binding;
    }
    if (result == nullptr) {
        throw std::invalid_argument("pick has no texture-set binding for the hit partition");
    }
    return *result;
}

UdimTile udim_tile(mesh::Vec2f uv) {
    const double tile_u = std::floor(uv.x);
    const double tile_v = std::floor(uv.y);
    if (tile_u < std::numeric_limits<std::int32_t>::min() ||
        tile_u > std::numeric_limits<std::int32_t>::max() ||
        tile_v < std::numeric_limits<std::int32_t>::min() ||
        tile_v > std::numeric_limits<std::int32_t>::max()) {
        throw std::overflow_error("hit UV is outside the supported UDIM coordinate range");
    }
    const auto u = static_cast<std::int32_t>(tile_u);
    const auto v = static_cast<std::int32_t>(tile_v);
    return {u, v, 1001LL + u + 10LL * v};
}

std::vector<ExactCandidate> exact_candidates(const RayCandidateQuery& broad_phase,
                                             const mesh::MeshDescriptor& descriptor, Ray ray,
                                             RayPickOptions options) {
    std::vector<ExactCandidate> result;
    if (options.occlusion == OcclusionPolicy::all_hits) {
        result.reserve(broad_phase.triangle_indices.size());
    } else {
        result.reserve(1);
    }
    for (std::uint32_t triangle : broad_phase.triangle_indices) {
        const std::size_t first_index = static_cast<std::size_t>(triangle) * 3;
        const mesh::Vec3f first = descriptor.positions[descriptor.triangle_indices[first_index]];
        const mesh::Vec3f second =
            descriptor.positions[descriptor.triangle_indices[first_index + 1]];
        const mesh::Vec3f third =
            descriptor.positions[descriptor.triangle_indices[first_index + 2]];
        const auto intersection = intersect_triangle(ray, first, second, third,
                                                     options.maximum_distance, options.backfaces);
        if (!intersection) {
            continue;
        }
        const ExactCandidate candidate{triangle, *intersection};
        if (options.occlusion == OcclusionPolicy::all_hits) {
            result.push_back(candidate);
        } else if (result.empty() || preferred_nearest(candidate, result.front())) {
            if (result.empty()) {
                result.push_back(candidate);
            } else {
                result.front() = candidate;
            }
        }
    }
    if (options.occlusion == OcclusionPolicy::all_hits) {
        std::sort(result.begin(), result.end(), comes_before);
    }
    return result;
}

HitRecord make_hit_record(const mesh::MeshBinding& mesh, std::uint32_t triangle, Vec3d barycentric,
                          mesh::Vec3f position, float distance,
                          std::span<const TextureSetBindingView> texture_sets) {
    const auto& descriptor = mesh.view().descriptor();
    const std::size_t first_index = static_cast<std::size_t>(triangle) * 3;
    const std::uint32_t first_vertex = descriptor.triangle_indices[first_index];
    const std::uint32_t second_vertex = descriptor.triangle_indices[first_index + 1];
    const std::uint32_t third_vertex = descriptor.triangle_indices[first_index + 2];
    const mesh::Vec3f first = descriptor.positions[first_vertex];
    const mesh::Vec3f second = descriptor.positions[second_vertex];
    const mesh::Vec3f third = descriptor.positions[third_vertex];
    const mesh::Vec3f geometric_normal =
        normalize(cross(subtract(as_double(second), as_double(first)),
                        subtract(as_double(third), as_double(first))));
    const mesh::Vec3f interpolated_normal = normalize(
        as_double(interpolate(descriptor.normals[first_vertex], descriptor.normals[second_vertex],
                              descriptor.normals[third_vertex], barycentric)),
        geometric_normal);

    const std::uint32_t partition_index = descriptor.face_partition_indices[triangle];
    const auto& binding = texture_binding_for_partition(texture_sets, partition_index);
    const auto& uv_set = mesh.view().uv_set(binding.uv_set);
    const mesh::Vec2f uv = interpolate(uv_set.values[first_vertex], uv_set.values[second_vertex],
                                       uv_set.values[third_vertex], barycentric);
    const mesh::MeshPartition& partition = descriptor.partitions[partition_index];

    return {
        .position = position,
        .interpolated_normal = interpolated_normal,
        .geometric_normal = geometric_normal,
        .uv = uv,
        .texture_set_id =
            mesh::texture_set_stable_id(partition.kind, partition.stable_key, binding.uv_set),
        .udim_tile = udim_tile(uv),
        .triangle_index = triangle,
        .barycentric = {static_cast<float>(barycentric.x), static_cast<float>(barycentric.y),
                        static_cast<float>(barycentric.z)},
        .material_id = descriptor.face_material_ids[triangle],
        .distance = distance,
    };
}

std::optional<Vec3d> uv_barycentric(mesh::Vec2f point, mesh::Vec2f first, mesh::Vec2f second,
                                    mesh::Vec2f third) {
    constexpr double epsilon = 1.0e-10;
    const double denominator =
        (second.y - third.y) * (first.x - third.x) + (third.x - second.x) * (first.y - third.y);
    if (std::abs(denominator) <= epsilon) {
        return std::nullopt;
    }
    const double first_weight =
        ((second.y - third.y) * (point.x - third.x) + (third.x - second.x) * (point.y - third.y)) /
        denominator;
    const double second_weight =
        ((third.y - first.y) * (point.x - third.x) + (first.x - third.x) * (point.y - third.y)) /
        denominator;
    const double third_weight = 1.0 - first_weight - second_weight;
    if (first_weight < -epsilon || second_weight < -epsilon || third_weight < -epsilon) {
        return std::nullopt;
    }
    const Vec3d clamped{
        std::max(0.0, first_weight),
        std::max(0.0, second_weight),
        std::max(0.0, third_weight),
    };
    const double sum = clamped.x + clamped.y + clamped.z;
    return Vec3d{clamped.x / sum, clamped.y / sum, clamped.z / sum};
}

UvHitRecord make_uv_hit_record(const mesh::MeshBinding& mesh, std::uint32_t triangle,
                               Vec3d barycentric, TextureSetBindingView texture_set) {
    const auto& descriptor = mesh.view().descriptor();
    const std::size_t first_index = static_cast<std::size_t>(triangle) * 3;
    const std::uint32_t first_vertex = descriptor.triangle_indices[first_index];
    const std::uint32_t second_vertex = descriptor.triangle_indices[first_index + 1];
    const std::uint32_t third_vertex = descriptor.triangle_indices[first_index + 2];
    const mesh::Vec3f first = descriptor.positions[first_vertex];
    const mesh::Vec3f second = descriptor.positions[second_vertex];
    const mesh::Vec3f third = descriptor.positions[third_vertex];
    const mesh::Vec3f geometric_normal =
        normalize(cross(subtract(as_double(second), as_double(first)),
                        subtract(as_double(third), as_double(first))));
    const mesh::Vec3f interpolated_normal = normalize(
        as_double(interpolate(descriptor.normals[first_vertex], descriptor.normals[second_vertex],
                              descriptor.normals[third_vertex], barycentric)),
        geometric_normal);
    const auto& uv_set = mesh.view().uv_set(texture_set.uv_set);
    const mesh::Vec2f uv = interpolate(uv_set.values[first_vertex], uv_set.values[second_vertex],
                                       uv_set.values[third_vertex], barycentric);
    const mesh::MeshPartition& partition = descriptor.partitions[texture_set.partition_index];
    return {
        .position = interpolate(first, second, third, barycentric),
        .interpolated_normal = interpolated_normal,
        .geometric_normal = geometric_normal,
        .uv = uv,
        .texture_set_id =
            mesh::texture_set_stable_id(partition.kind, partition.stable_key, texture_set.uv_set),
        .udim_tile = udim_tile(uv),
        .triangle_index = triangle,
        .barycentric = {static_cast<float>(barycentric.x), static_cast<float>(barycentric.y),
                        static_cast<float>(barycentric.z)},
        .material_id = descriptor.face_material_ids[triangle],
    };
}

}  // namespace

std::vector<HitRecord> pick_ray(SpatialIndex& index, const mesh::MeshBinding& mesh, Ray ray,
                                RayPickOptions options,
                                std::span<const TextureSetBindingView> texture_sets,
                                PickQueryCost* cost) {
    ray = normalized_ray(ray);
    const auto broad_phase = index.query_ray_candidates(mesh, ray, options.maximum_distance);
    if (cost != nullptr) {
        *cost = {broad_phase.visited_nodes, broad_phase.tested_leaf_triangles};
    }
    const auto& descriptor = mesh.view().descriptor();
    const auto exact = exact_candidates(broad_phase, descriptor, ray, options);
    std::vector<HitRecord> result;
    result.reserve(exact.size());
    for (const ExactCandidate& candidate : exact) {
        const float distance = static_cast<float>(candidate.intersection.distance);
        const mesh::Vec3f position{
            ray.origin.x + ray.direction.x * distance,
            ray.origin.y + ray.direction.y * distance,
            ray.origin.z + ray.direction.z * distance,
        };
        result.push_back(make_hit_record(mesh, candidate.triangle,
                                         candidate.intersection.barycentric, position, distance,
                                         texture_sets));
    }
    return result;
}

std::optional<HitRecord> pick_nearest(SpatialIndex& index, const mesh::MeshBinding& mesh, Ray ray,
                                      float maximum_distance,
                                      std::span<const TextureSetBindingView> texture_sets,
                                      BackfacePolicy backfaces, PickQueryCost* cost) {
    auto hits = pick_ray(index, mesh, ray,
                         {.maximum_distance = maximum_distance,
                          .occlusion = OcclusionPolicy::nearest,
                          .backfaces = backfaces},
                         texture_sets, cost);
    if (hits.empty()) {
        return std::nullopt;
    }
    return std::move(hits.front());
}

std::optional<UvHitRecord> pick_uv(UvSpatialIndex& index, const mesh::MeshBinding& mesh,
                                   mesh::Vec2f coordinate, TextureSetBindingView texture_set,
                                   PickQueryCost* cost) {
    const auto& descriptor = mesh.view().descriptor();
    if (texture_set.partition_index >= descriptor.partitions.size()) {
        throw std::invalid_argument("UV pick texture-set partition is not present in the mesh");
    }
    if (index.uv_set() != texture_set.uv_set) {
        throw std::invalid_argument("UV pick binding does not match the spatial index UV set");
    }
    const auto& uv = mesh.view().uv_set(texture_set.uv_set).values;
    const auto candidates = index.query_candidates(mesh, coordinate);
    if (cost != nullptr) {
        *cost = {candidates.visited_nodes, candidates.tested_leaf_triangles};
    }
    std::optional<Vec3d> selected_barycentric;
    std::uint32_t selected_triangle = 0;
    for (std::uint32_t triangle : candidates.triangle_indices) {
        if (descriptor.face_partition_indices[triangle] != texture_set.partition_index) {
            continue;
        }
        const std::size_t first_index = static_cast<std::size_t>(triangle) * 3;
        const auto barycentric =
            uv_barycentric(coordinate, uv[descriptor.triangle_indices[first_index]],
                           uv[descriptor.triangle_indices[first_index + 1]],
                           uv[descriptor.triangle_indices[first_index + 2]]);
        if (barycentric && (!selected_barycentric || triangle < selected_triangle)) {
            selected_barycentric = barycentric;
            selected_triangle = triangle;
        }
    }
    if (!selected_barycentric) {
        return std::nullopt;
    }
    return make_uv_hit_record(mesh, selected_triangle, *selected_barycentric, texture_set);
}

std::optional<HitRecord> snap_to_surface(SpatialIndex& index, const mesh::MeshBinding& mesh,
                                         mesh::Vec3f point, float maximum_distance,
                                         std::span<const TextureSetBindingView> texture_sets,
                                         PickQueryCost* cost) {
    const auto broad_phase = index.query_point_candidates(mesh, point, maximum_distance);
    if (cost != nullptr) {
        *cost = {broad_phase.visited_nodes, broad_phase.tested_leaf_triangles};
    }
    const auto& descriptor = mesh.view().descriptor();
    const Vec3d point_d = as_double(point);
    const double limit_squared = static_cast<double>(maximum_distance) * maximum_distance;
    std::optional<SnapCandidate> nearest;
    for (std::uint32_t triangle : broad_phase.triangle_indices) {
        const std::size_t first_index = static_cast<std::size_t>(triangle) * 3;
        const ClosestPoint closest = closest_on_triangle(
            point_d, as_double(descriptor.positions[descriptor.triangle_indices[first_index]]),
            as_double(descriptor.positions[descriptor.triangle_indices[first_index + 1]]),
            as_double(descriptor.positions[descriptor.triangle_indices[first_index + 2]]));
        if (closest.squared_distance > limit_squared) {
            continue;
        }
        if (!nearest ||
            (equivalent_distance(closest.squared_distance, nearest->closest.squared_distance)
                 ? triangle < nearest->triangle
                 : closest.squared_distance < nearest->closest.squared_distance)) {
            nearest = SnapCandidate{triangle, closest};
        }
    }
    if (!nearest) {
        return std::nullopt;
    }
    return make_hit_record(
        mesh, nearest->triangle, nearest->closest.barycentric, as_float(nearest->closest.position),
        static_cast<float>(std::sqrt(nearest->closest.squared_distance)), texture_sets);
}

}  // namespace ctex::pick
