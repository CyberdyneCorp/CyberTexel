#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <ctex/doc/mesh_reprojection.hpp>
#include <limits>
#include <map>
#include <optional>
#include <span>
#include <string_view>
#include <utility>

namespace ctex::doc {
namespace {

constexpr double geometry_epsilon = 1.0e-12;
constexpr std::uint32_t no_triangle = std::numeric_limits<std::uint32_t>::max();

struct Vec2 {
    double x{};
    double y{};
};

struct Vec3 {
    double x{};
    double y{};
    double z{};
};

struct ClosestPoint {
    Vec3 position;
    std::array<double, 3> barycentric{};
    double squared_distance{};
};

struct SurfaceCandidate {
    std::uint32_t triangle{};
    ClosestPoint closest;
    Vec3 normal;
    double normal_angle{};
};

struct TargetSample {
    std::uint32_t triangle{no_triangle};
    std::array<double, 3> barycentric{};
    Vec3 position;
    Vec3 normal;
};

struct WorkControl {
    const MeshReprojectionLimits& limits;
    const MeshReprojectionControl& callbacks;
    std::size_t completed{};

    void poll() const {
        if (callbacks.is_cancelled && callbacks.is_cancelled()) {
            throw MeshReprojectionError(MeshReprojectionErrorCode::cancelled,
                                        "mesh reprojection was cancelled");
        }
    }

    void require_capacity(std::size_t count) const {
        if (count > limits.maximum_work_items - completed) {
            throw MeshReprojectionError(MeshReprojectionErrorCode::over_budget,
                                        "mesh reprojection exceeded its work-item budget");
        }
    }

    void step() {
        if (completed == limits.maximum_work_items) {
            throw MeshReprojectionError(MeshReprojectionErrorCode::over_budget,
                                        "mesh reprojection exceeded its work-item budget");
        }
        ++completed;
        poll();
        if (callbacks.report_progress && completed % limits.progress_interval == 0) {
            callbacks.report_progress(completed);
        }
    }
};

[[noreturn]] void invalid(std::string message) {
    throw MeshReprojectionError(MeshReprojectionErrorCode::invalid_request, std::move(message));
}

Vec3 as_vec(mesh::Vec3f value) { return {value.x, value.y, value.z}; }
Vec2 as_vec(mesh::Vec2f value) { return {value.x, value.y}; }
Vec3 add(Vec3 left, Vec3 right) { return {left.x + right.x, left.y + right.y, left.z + right.z}; }
Vec3 subtract(Vec3 left, Vec3 right) {
    return {left.x - right.x, left.y - right.y, left.z - right.z};
}
Vec3 multiply(Vec3 value, double scale) {
    return {value.x * scale, value.y * scale, value.z * scale};
}
double dot(Vec3 left, Vec3 right) { return left.x * right.x + left.y * right.y + left.z * right.z; }
Vec3 cross(Vec3 left, Vec3 right) {
    return {left.y * right.z - left.z * right.y, left.z * right.x - left.x * right.z,
            left.x * right.y - left.y * right.x};
}
double length(Vec3 value) { return std::sqrt(dot(value, value)); }
Vec3 normalized(Vec3 value) {
    const double magnitude = length(value);
    if (!std::isfinite(magnitude) || magnitude <= geometry_epsilon) {
        invalid("mesh reprojection encountered a degenerate vector");
    }
    return multiply(value, 1.0 / magnitude);
}

Vec3 interpolate(const std::array<Vec3, 3>& values, const std::array<double, 3>& weights) {
    return add(add(multiply(values[0], weights[0]), multiply(values[1], weights[1])),
               multiply(values[2], weights[2]));
}

Vec2 interpolate(const std::array<Vec2, 3>& values, const std::array<double, 3>& weights) {
    return {values[0].x * weights[0] + values[1].x * weights[1] + values[2].x * weights[2],
            values[0].y * weights[0] + values[1].y * weights[1] + values[2].y * weights[2]};
}

std::array<std::uint32_t, 3> triangle_vertices(const mesh::MeshView& view, std::uint32_t triangle) {
    const auto indices = view.descriptor().triangle_indices;
    const std::size_t first = static_cast<std::size_t>(triangle) * 3;
    return {indices[first], indices[first + 1], indices[first + 2]};
}

std::array<Vec3, 3> triangle_positions(const mesh::MeshView& view, std::uint32_t triangle) {
    const auto vertices = triangle_vertices(view, triangle);
    const auto positions = view.descriptor().positions;
    return {as_vec(positions[vertices[0]]), as_vec(positions[vertices[1]]),
            as_vec(positions[vertices[2]])};
}

std::array<Vec3, 3> triangle_normals(const mesh::MeshView& view, std::uint32_t triangle) {
    const auto vertices = triangle_vertices(view, triangle);
    const auto normals = view.descriptor().normals;
    return {as_vec(normals[vertices[0]]), as_vec(normals[vertices[1]]),
            as_vec(normals[vertices[2]])};
}

std::array<Vec2, 3> triangle_uv(const mesh::MeshView& view, const mesh::UvSetView& uv,
                                std::uint32_t triangle) {
    const auto vertices = triangle_vertices(view, triangle);
    return {as_vec(uv.values[vertices[0]]), as_vec(uv.values[vertices[1]]),
            as_vec(uv.values[vertices[2]])};
}

ClosestPoint closest_on_segment(Vec3 point, Vec3 first, Vec3 second,
                                std::array<double, 3> first_weights,
                                std::array<double, 3> second_weights) {
    const Vec3 direction = subtract(second, first);
    const double extent = dot(direction, direction);
    const double t = extent <= geometry_epsilon
                         ? 0.0
                         : std::clamp(dot(subtract(point, first), direction) / extent, 0.0, 1.0);
    const Vec3 position = add(first, multiply(direction, t));
    std::array<double, 3> weights{};
    for (std::size_t index = 0; index < weights.size(); ++index) {
        weights[index] = first_weights[index] * (1.0 - t) + second_weights[index] * t;
    }
    const Vec3 delta = subtract(point, position);
    return {.position = position, .barycentric = weights, .squared_distance = dot(delta, delta)};
}

ClosestPoint closest_on_triangle(Vec3 point, const std::array<Vec3, 3>& vertex) {
    const Vec3 first_edge = subtract(vertex[1], vertex[0]);
    const Vec3 second_edge = subtract(vertex[2], vertex[0]);
    const Vec3 relative = subtract(point, vertex[0]);
    const double d00 = dot(first_edge, first_edge);
    const double d01 = dot(first_edge, second_edge);
    const double d11 = dot(second_edge, second_edge);
    const double d20 = dot(relative, first_edge);
    const double d21 = dot(relative, second_edge);
    const double denominator = d00 * d11 - d01 * d01;
    if (std::abs(denominator) > geometry_epsilon) {
        const double second = (d11 * d20 - d01 * d21) / denominator;
        const double third = (d00 * d21 - d01 * d20) / denominator;
        const double first = 1.0 - second - third;
        if (first >= 0.0 && second >= 0.0 && third >= 0.0) {
            const std::array weights{first, second, third};
            const Vec3 position = interpolate(vertex, weights);
            const Vec3 delta = subtract(point, position);
            return {.position = position,
                    .barycentric = weights,
                    .squared_distance = dot(delta, delta)};
        }
    }
    const std::array<ClosestPoint, 3> edges{
        closest_on_segment(point, vertex[0], vertex[1], {1.0, 0.0, 0.0}, {0.0, 1.0, 0.0}),
        closest_on_segment(point, vertex[1], vertex[2], {0.0, 1.0, 0.0}, {0.0, 0.0, 1.0}),
        closest_on_segment(point, vertex[2], vertex[0], {0.0, 0.0, 1.0}, {1.0, 0.0, 0.0}),
    };
    return *std::ranges::min_element(edges, {}, &ClosestPoint::squared_distance);
}

double edge(Vec2 first, Vec2 second, Vec2 point) {
    return (point.x - first.x) * (second.y - first.y) - (point.y - first.y) * (second.x - first.x);
}

std::array<double, 3> barycentric_2d(const std::array<Vec2, 3>& points, Vec2 point, double area) {
    return {edge(points[1], points[2], point) / area, edge(points[2], points[0], point) / area,
            edge(points[0], points[1], point) / area};
}

bool contains(const std::array<double, 3>& weights) {
    return std::ranges::all_of(weights, [](double value) { return value >= -geometry_epsilon; });
}

std::optional<std::uint32_t> partition_index(const mesh::MeshView& view,
                                             const TextureSetDescriptor& texture_set) {
    mesh::PartitionKind kind{};
    switch (texture_set.partition_kind) {
        case PartitionSourceKind::material:
            kind = mesh::PartitionKind::material;
            break;
        case PartitionSourceKind::object:
            kind = mesh::PartitionKind::object;
            break;
        case PartitionSourceKind::submesh:
            kind = mesh::PartitionKind::submesh;
            break;
        case PartitionSourceKind::explicit_faces:
            kind = mesh::PartitionKind::explicit_faces;
            break;
    }
    const auto found = std::ranges::find_if(view.descriptor().partitions, [&](const auto& value) {
        return value.kind == kind && value.stable_key == texture_set.partition_key;
    });
    if (found == view.descriptor().partitions.end()) {
        return std::nullopt;
    }
    return static_cast<std::uint32_t>(std::distance(view.descriptor().partitions.begin(), found));
}

std::vector<std::uint32_t> partition_triangles(const mesh::MeshView& view,
                                               std::uint32_t partition) {
    std::vector<std::uint32_t> result;
    for (std::uint32_t triangle = 0; triangle < view.triangle_count(); ++triangle) {
        if (view.descriptor().face_partition_indices[triangle] == partition) {
            result.push_back(triangle);
        }
    }
    return result;
}

void validate_limits(const MeshReprojectionLimits& limits) {
    constexpr double pi = 3.14159265358979323846;
    if (!std::isfinite(limits.maximum_distance) || limits.maximum_distance <= 0.0 ||
        !std::isfinite(limits.maximum_normal_angle_radians) ||
        limits.maximum_normal_angle_radians < 0.0 || limits.maximum_normal_angle_radians > pi ||
        !std::isfinite(limits.visibility_epsilon) || limits.visibility_epsilon <= 0.0 ||
        !std::isfinite(limits.ambiguity_distance_epsilon) ||
        limits.ambiguity_distance_epsilon < 0.0 || limits.maximum_work_items == 0 ||
        limits.progress_interval == 0) {
        invalid("mesh reprojection limits are invalid");
    }
}

bool segment_intersects_triangle(Vec3 origin, Vec3 destination, const std::array<Vec3, 3>& vertex,
                                 double epsilon) {
    const Vec3 direction = subtract(destination, origin);
    const Vec3 first_edge = subtract(vertex[1], vertex[0]);
    const Vec3 second_edge = subtract(vertex[2], vertex[0]);
    const Vec3 p = cross(direction, second_edge);
    const double determinant = dot(first_edge, p);
    if (std::abs(determinant) <= geometry_epsilon) {
        return false;
    }
    const double inverse = 1.0 / determinant;
    const Vec3 translated = subtract(origin, vertex[0]);
    const double u = dot(translated, p) * inverse;
    const Vec3 q = cross(translated, first_edge);
    const double v = dot(direction, q) * inverse;
    const double t = dot(second_edge, q) * inverse;
    return u >= 0.0 && v >= 0.0 && u + v <= 1.0 && t > epsilon && t < 1.0 - epsilon;
}

bool visible(Vec3 origin, const SurfaceCandidate& candidate, const mesh::MeshView& view,
             std::span<const std::uint32_t> triangles, const MeshReprojectionLimits& limits,
             WorkControl& work) {
    if (!limits.require_visibility) {
        return true;
    }
    for (std::uint32_t triangle : triangles) {
        if (triangle == candidate.triangle) {
            continue;
        }
        work.step();
        if (segment_intersects_triangle(origin, candidate.closest.position,
                                        triangle_positions(view, triangle),
                                        limits.visibility_epsilon)) {
            return false;
        }
    }
    return true;
}

std::optional<SurfaceCandidate> map_point(Vec3 point, Vec3 normal, const mesh::MeshView& target,
                                          std::span<const std::uint32_t> triangles,
                                          const MeshReprojectionLimits& limits, WorkControl& work,
                                          std::size_t& close_candidate_count) {
    std::optional<SurfaceCandidate> nearest;
    const double distance_limit_squared = limits.maximum_distance * limits.maximum_distance;
    for (std::uint32_t triangle : triangles) {
        work.step();
        const ClosestPoint closest =
            closest_on_triangle(point, triangle_positions(target, triangle));
        if (closest.squared_distance > distance_limit_squared) {
            continue;
        }
        const Vec3 candidate_normal =
            normalized(interpolate(triangle_normals(target, triangle), closest.barycentric));
        const double cosine = std::clamp(dot(normalized(normal), candidate_normal), -1.0, 1.0);
        const double angle = std::acos(cosine);
        if (angle > limits.maximum_normal_angle_radians) {
            continue;
        }
        SurfaceCandidate candidate{.triangle = triangle,
                                   .closest = closest,
                                   .normal = candidate_normal,
                                   .normal_angle = angle};
        if (!visible(point, candidate, target, triangles, limits, work)) {
            continue;
        }
        const double distance = std::sqrt(closest.squared_distance);
        if (!nearest) {
            nearest = candidate;
            close_candidate_count = 1;
            continue;
        }
        const double nearest_distance = std::sqrt(nearest->closest.squared_distance);
        if (distance + limits.ambiguity_distance_epsilon < nearest_distance) {
            nearest = candidate;
            close_candidate_count = 1;
        } else if (std::abs(distance - nearest_distance) <= limits.ambiguity_distance_epsilon) {
            ++close_candidate_count;
            if (candidate.triangle < nearest->triangle) {
                nearest = candidate;
            }
        }
    }
    return nearest;
}

std::vector<TargetSample> raster_target(const mesh::MeshView& view, const mesh::UvSetView& uv,
                                        std::span<const std::uint32_t> triangles,
                                        std::uint32_t width, std::uint32_t height,
                                        WorkControl& work) {
    const std::size_t texel_count = static_cast<std::size_t>(width) * height;
    work.poll();
    work.require_capacity(texel_count);
    std::vector<TargetSample> result(texel_count);
    for (std::uint32_t triangle : triangles) {
        const auto source_uv = triangle_uv(view, uv, triangle);
        std::array<Vec2, 3> pixel{};
        for (std::size_t corner = 0; corner < 3; ++corner) {
            pixel[corner] = {source_uv[corner].x * width, (1.0 - source_uv[corner].y) * height};
        }
        const double area = edge(pixel[0], pixel[1], pixel[2]);
        if (std::abs(area) <= geometry_epsilon) {
            continue;
        }
        const auto x_coordinate = [width](double value) {
            return static_cast<std::uint32_t>(
                std::clamp(std::floor(value), 0.0, static_cast<double>(width - 1)));
        };
        const auto y_coordinate = [height](double value) {
            return static_cast<std::uint32_t>(
                std::clamp(std::floor(value), 0.0, static_cast<double>(height - 1)));
        };
        const std::uint32_t minimum_x =
            x_coordinate(std::min({pixel[0].x, pixel[1].x, pixel[2].x}));
        const std::uint32_t maximum_x =
            x_coordinate(std::max({pixel[0].x, pixel[1].x, pixel[2].x}));
        const std::uint32_t minimum_y =
            y_coordinate(std::min({pixel[0].y, pixel[1].y, pixel[2].y}));
        const std::uint32_t maximum_y =
            y_coordinate(std::max({pixel[0].y, pixel[1].y, pixel[2].y}));
        const auto positions = triangle_positions(view, triangle);
        const auto normals = triangle_normals(view, triangle);
        for (std::uint32_t y = minimum_y; y <= maximum_y; ++y) {
            for (std::uint32_t x = minimum_x; x <= maximum_x; ++x) {
                work.step();
                const auto weights = barycentric_2d(
                    pixel, {static_cast<double>(x) + 0.5, static_cast<double>(y) + 0.5}, area);
                TargetSample& sample = result[static_cast<std::size_t>(y) * width + x];
                if (!contains(weights) ||
                    (sample.triangle != no_triangle && sample.triangle < triangle)) {
                    continue;
                }
                sample = {.triangle = triangle,
                          .barycentric = weights,
                          .position = interpolate(positions, weights),
                          .normal = normalized(interpolate(normals, weights))};
            }
        }
    }
    return result;
}

std::array<double, 3> array(Vec3 value) { return {value.x, value.y, value.z}; }

ReprojectionTexelMapping map_texel(std::string_view texture_set_id, std::uint32_t x,
                                   std::uint32_t y, const TargetSample& target_sample,
                                   const mesh::MeshView& source,
                                   std::span<const std::uint32_t> source_triangles,
                                   const mesh::UvSetView& source_uv,
                                   const MeshReprojectionLimits& limits, WorkControl& work) {
    std::size_t candidate_count = 0;
    const auto candidate = map_point(target_sample.position, target_sample.normal, source,
                                     source_triangles, limits, work, candidate_count);
    ReprojectionTexelMapping mapping{.texture_set_id = std::string(texture_set_id),
                                     .x = x,
                                     .y = y,
                                     .status = ReprojectionMappingStatus::unmapped,
                                     .target_triangle = target_sample.triangle,
                                     .source_triangle = no_triangle,
                                     .target_barycentric = target_sample.barycentric};
    if (!candidate) {
        return mapping;
    }
    const Vec2 uv = interpolate(triangle_uv(source, source_uv, candidate->triangle),
                                candidate->closest.barycentric);
    mapping.status = candidate_count > 1 ? ReprojectionMappingStatus::ambiguous
                                         : ReprojectionMappingStatus::mapped;
    mapping.source_triangle = candidate->triangle;
    mapping.source_barycentric = candidate->closest.barycentric;
    mapping.source_uv = {uv.x, uv.y};
    mapping.distance = std::sqrt(candidate->closest.squared_distance);
    mapping.normal_angle_radians = candidate->normal_angle;
    mapping.candidate_count = candidate_count;
    return mapping;
}

ReprojectionEntryMapping map_entry(const TextureSet& texture_set,
                                   const EditableAuthoringEntry& entry,
                                   const mesh::MeshView& replacement,
                                   std::span<const std::uint32_t> replacement_triangles,
                                   const MeshReprojectionLimits& limits, WorkControl& work) {
    ReprojectionEntryMapping result{.texture_set_id = texture_set.id(),
                                    .entry_id = entry.identifier,
                                    .entry_revision = entry.revision,
                                    .point_count = entry.surface_points.size()};
    result.points.reserve(entry.surface_points.size());
    for (const EditableSurfacePoint& point : entry.surface_points) {
        std::size_t candidate_count = 0;
        const auto candidate =
            map_point({point.position[0], point.position[1], point.position[2]},
                      {point.normal[0], point.normal[1], point.normal[2]}, replacement,
                      replacement_triangles, limits, work, candidate_count);
        ReprojectionEntryMapping::Point mapped{.status = ReprojectionMappingStatus::unmapped,
                                               .triangle = no_triangle};
        if (!candidate) {
            ++result.unmapped_point_count;
        } else {
            mapped.status = candidate_count > 1 ? ReprojectionMappingStatus::ambiguous
                                                : ReprojectionMappingStatus::mapped;
            mapped.position = array(candidate->closest.position);
            mapped.normal = array(candidate->normal);
            mapped.triangle = candidate->triangle;
            mapped.barycentric = candidate->closest.barycentric;
            mapped.distance = std::sqrt(candidate->closest.squared_distance);
            mapped.candidate_count = candidate_count;
            result.ambiguous_point_count += candidate_count > 1 ? 1 : 0;
        }
        result.points.push_back(mapped);
    }
    return result;
}

std::uint32_t source_pixel_coordinate(double uv, std::uint32_t extent) {
    return std::min(static_cast<std::uint32_t>(std::floor(std::clamp(uv, 0.0, 1.0) * extent)),
                    extent - 1);
}

double read_component(std::span<const std::byte> pixel, image::PixelFormat format,
                      std::size_t component) {
    const std::byte* source = pixel.data() + component * format.bytes_per_channel();
    if (format.channel_type == image::ChannelType::uint8_unorm) {
        return std::to_integer<std::uint8_t>(*source) / 255.0;
    }
    if (format.channel_type == image::ChannelType::uint16_unorm) {
        std::uint16_t value{};
        std::memcpy(&value, source, sizeof(value));
        return static_cast<double>(value) / 65535.0;
    }
    float value{};
    std::memcpy(&value, source, sizeof(value));
    return value;
}

void write_component(std::span<std::byte> pixel, image::PixelFormat format, std::size_t component,
                     double value) {
    std::byte* destination = pixel.data() + component * format.bytes_per_channel();
    if (format.channel_type == image::ChannelType::uint8_unorm) {
        const auto encoded =
            static_cast<std::uint8_t>(std::llround(std::clamp(value, 0.0, 1.0) * 255.0));
        std::memcpy(destination, &encoded, sizeof(encoded));
    } else if (format.channel_type == image::ChannelType::uint16_unorm) {
        const auto encoded =
            static_cast<std::uint16_t>(std::llround(std::clamp(value, 0.0, 1.0) * 65535.0));
        std::memcpy(destination, &encoded, sizeof(encoded));
    } else {
        const float encoded = static_cast<float>(value);
        std::memcpy(destination, &encoded, sizeof(encoded));
    }
}

struct TangentFrame {
    Vec3 tangent;
    Vec3 bitangent;
    Vec3 normal;
};

TangentFrame tangent_frame(const mesh::MeshView& view, std::uint32_t triangle,
                           const std::array<double, 3>& barycentric) {
    const auto tangents = view.tangent_frames().corner_tangents;
    const std::size_t first = static_cast<std::size_t>(triangle) * 3;
    Vec3 tangent{};
    double handedness = 0.0;
    for (std::size_t corner = 0; corner < 3; ++corner) {
        tangent = add(tangent, multiply({tangents[first + corner].x, tangents[first + corner].y,
                                         tangents[first + corner].z},
                                        barycentric[corner]));
        handedness += tangents[first + corner].w * barycentric[corner];
    }
    const Vec3 normal = normalized(interpolate(triangle_normals(view, triangle), barycentric));
    tangent = normalized(subtract(tangent, multiply(normal, dot(tangent, normal))));
    return {.tangent = tangent,
            .bitangent = multiply(cross(normal, tangent), handedness < 0.0 ? -1.0 : 1.0),
            .normal = normal};
}

std::vector<std::byte> transformed_normal_pixel(std::span<const std::byte> source,
                                                image::PixelFormat format,
                                                const TangentFrame& source_frame,
                                                const TangentFrame& target_frame) {
    std::vector<std::byte> result(source.begin(), source.end());
    Vec3 tangent_normal{read_component(source, format, 0) * 2.0 - 1.0,
                        read_component(source, format, 1) * 2.0 - 1.0,
                        read_component(source, format, 2) * 2.0 - 1.0};
    tangent_normal = normalized(tangent_normal);
    const Vec3 object = normalized(add(add(multiply(source_frame.tangent, tangent_normal.x),
                                           multiply(source_frame.bitangent, tangent_normal.y)),
                                       multiply(source_frame.normal, tangent_normal.z)));
    const Vec3 target{dot(object, target_frame.tangent), dot(object, target_frame.bitangent),
                      dot(object, target_frame.normal)};
    write_component(result, format, 0, target.x * 0.5 + 0.5);
    write_component(result, format, 1, target.y * 0.5 + 0.5);
    write_component(result, format, 2, target.z * 0.5 + 0.5);
    return result;
}

struct StagedChannel {
    TextureChannels* channels{};
    std::string semantic_id;
    image::TiledImage image;
};

}  // namespace

MeshReprojectionError::MeshReprojectionError(MeshReprojectionErrorCode code, std::string message)
    : std::runtime_error(std::move(message)), code_(code) {}

MeshReprojectionPreflight preflight_mesh_reprojection(
    const TextureDocument& document, const mesh::MeshBinding& source,
    const mesh::MeshBinding& replacement, const MeshReprojectionLimits& limits,
    const MeshReprojectionControl& control, std::span<const std::string_view> texture_set_ids) {
    validate_limits(limits);
    if (replacement.revision() <= source.revision()) {
        invalid("replacement mesh revision must advance the source revision");
    }
    MeshReprojectionPreflight result{.source_revision = source.revision(),
                                     .replacement_revision = replacement.revision(),
                                     .published_revision = replacement.revision(),
                                     .limits = limits};
    WorkControl work{limits, control};
    for (std::string_view texture_set_id : texture_set_ids) {
        if (texture_set_id.empty() || !document.contains_texture_set(texture_set_id) ||
            std::ranges::count(texture_set_ids, texture_set_id) != 1) {
            invalid("mesh reprojection texture-set selection is invalid");
        }
    }
    for (const std::string& texture_set_id : document.texture_set_ids()) {
        if (!texture_set_ids.empty() &&
            std::ranges::find(texture_set_ids, texture_set_id) == texture_set_ids.end()) {
            continue;
        }
        const TextureSet& texture_set = document.texture_set(texture_set_id);
        const TextureSetDescriptor descriptor = texture_set.descriptor();
        const auto source_partition = partition_index(source.view(), descriptor);
        const auto replacement_partition = partition_index(replacement.view(), descriptor);
        if (!source_partition || !replacement_partition) {
            invalid("mesh reprojection requires matching source and replacement partitions: " +
                    texture_set_id);
        }
        const mesh::UvSetView& source_uv = source.view().uv_set(descriptor.uv_set);
        const mesh::UvSetView& replacement_uv = replacement.view().uv_set(descriptor.uv_set);
        const auto source_triangles = partition_triangles(source.view(), *source_partition);
        const auto replacement_triangles =
            partition_triangles(replacement.view(), *replacement_partition);
        const auto target = raster_target(replacement.view(), replacement_uv, replacement_triangles,
                                          descriptor.width, descriptor.height, work);
        for (const std::string& semantic_id : texture_set.channels().semantic_ids()) {
            const bool enabled = texture_set.channels().is_enabled(semantic_id);
            result.source_channels.push_back(
                {.texture_set_id = texture_set_id,
                 .semantic_id = semantic_id,
                 .enabled = enabled,
                 .revision = enabled ? texture_set.channels().channel_revision_cursor(semantic_id)
                                     : ChannelRevisionCursor{}});
        }
        for (std::uint32_t y = 0; y < descriptor.height; ++y) {
            for (std::uint32_t x = 0; x < descriptor.width; ++x) {
                const TargetSample& sample =
                    target[static_cast<std::size_t>(y) * descriptor.width + x];
                if (sample.triangle == no_triangle) {
                    continue;
                }
                ReprojectionTexelMapping mapping =
                    map_texel(texture_set_id, x, y, sample, source.view(), source_triangles,
                              source_uv, limits, work);
                result.mapped_texel_count +=
                    mapping.status == ReprojectionMappingStatus::mapped ? 1 : 0;
                result.unmapped_texel_count +=
                    mapping.status == ReprojectionMappingStatus::unmapped ? 1 : 0;
                result.ambiguous_texel_count +=
                    mapping.status == ReprojectionMappingStatus::ambiguous ? 1 : 0;
                result.texels.push_back(std::move(mapping));
            }
        }
        for (const EditableAuthoringEntry& entry : texture_set.editable_authoring().entries()) {
            if (entry.kind == EditableEntryKind::surface_path &&
                entry.mesh_revision == source.revision()) {
                result.affected_entries.push_back(map_entry(texture_set, entry, replacement.view(),
                                                            replacement_triangles, limits, work));
            }
        }
    }
    result.tested_candidate_count = work.completed;
    if (control.report_progress) {
        control.report_progress(work.completed);
    }
    return result;
}

MeshReprojectionCommitReport commit_mesh_reprojection(
    TextureDocument& document, const mesh::MeshBinding& source,
    const mesh::MeshBinding& replacement, const MeshReprojectionPreflight& preflight,
    ReprojectionHolePolicy hole_policy, ReprojectionAmbiguityPolicy ambiguity_policy) {
    if (preflight.source_revision != source.revision() ||
        preflight.replacement_revision != replacement.revision() ||
        preflight.published_revision != replacement.revision()) {
        throw MeshReprojectionError(MeshReprojectionErrorCode::stale_preflight,
                                    "mesh reprojection preflight is stale");
    }
    if (hole_policy != ReprojectionHolePolicy::retain_target &&
        hole_policy != ReprojectionHolePolicy::channel_default) {
        invalid("mesh reprojection hole policy is invalid");
    }
    if (ambiguity_policy != ReprojectionAmbiguityPolicy::refuse &&
        ambiguity_policy != ReprojectionAmbiguityPolicy::nearest_then_lowest_triangle) {
        invalid("mesh reprojection ambiguity policy is invalid");
    }
    if (preflight.ambiguous_texel_count != 0 &&
        ambiguity_policy == ReprojectionAmbiguityPolicy::refuse) {
        throw MeshReprojectionError(MeshReprojectionErrorCode::unresolved_ambiguity,
                                    "mesh reprojection has unresolved ambiguous texels");
    }
    for (const ReprojectionEntryMapping& entry : preflight.affected_entries) {
        if (entry.unmapped_point_count != 0) {
            throw MeshReprojectionError(
                MeshReprojectionErrorCode::invalid_attachment,
                "mesh reprojection has an unmapped editable attachment: " + entry.entry_id);
        }
        if (!document.contains_texture_set(entry.texture_set_id) ||
            document.texture_set(entry.texture_set_id)
                    .editable_authoring()
                    .entry(entry.entry_id)
                    .revision != entry.entry_revision) {
            throw MeshReprojectionError(
                MeshReprojectionErrorCode::stale_preflight,
                "editable attachment changed after reprojection preflight: " + entry.entry_id);
        }
        if (entry.ambiguous_point_count != 0 &&
            ambiguity_policy == ReprojectionAmbiguityPolicy::refuse) {
            throw MeshReprojectionError(
                MeshReprojectionErrorCode::unresolved_ambiguity,
                "mesh reprojection has an ambiguous editable attachment: " + entry.entry_id);
        }
    }
    for (const ReprojectionChannelState& state : preflight.source_channels) {
        if (!document.contains_texture_set(state.texture_set_id)) {
            throw MeshReprojectionError(MeshReprojectionErrorCode::stale_preflight,
                                        "texture set changed after reprojection preflight");
        }
        const TextureChannels& channels = document.texture_set(state.texture_set_id).channels();
        if (!channels.contains_descriptor(state.semantic_id) ||
            channels.is_enabled(state.semantic_id) != state.enabled ||
            (state.enabled &&
             channels.channel_revision_cursor(state.semantic_id) != state.revision)) {
            throw MeshReprojectionError(
                MeshReprojectionErrorCode::stale_preflight,
                "channel changed after reprojection preflight: " + state.semantic_id);
        }
    }

    MeshReprojectionCommitReport report;
    std::vector<StagedChannel> staged_channels;
    for (const std::string& texture_set_id : document.texture_set_ids()) {
        if (std::ranges::none_of(preflight.source_channels, [&](const auto& state) {
                return state.texture_set_id == texture_set_id;
            })) {
            continue;
        }
        TextureSet& texture_set = document.texture_set(texture_set_id);
        TextureChannels& channels = texture_set.channels();
        for (const std::string& semantic_id : channels.semantic_ids()) {
            if (!channels.is_enabled(semantic_id)) {
                continue;
            }
            const image::TiledImage& source_image = channels.pixels(semantic_id);
            image::TiledImage staged(source_image);
            for (const ReprojectionTexelMapping& mapping : preflight.texels) {
                if (mapping.texture_set_id != texture_set_id) {
                    continue;
                }
                if (mapping.status == ReprojectionMappingStatus::unmapped) {
                    if (hole_policy == ReprojectionHolePolicy::channel_default) {
                        staged.write_pixel(mapping.x, mapping.y, staged.clear_pixel());
                        ++report.defaulted_hole_count;
                    } else {
                        ++report.retained_hole_count;
                    }
                    continue;
                }
                const std::uint32_t source_x =
                    source_pixel_coordinate(mapping.source_uv[0], source_image.width());
                const std::uint32_t source_y =
                    source_pixel_coordinate(1.0 - mapping.source_uv[1], source_image.height());
                const auto source_pixel = source_image.read_pixel(source_x, source_y);
                if (channels.descriptor(semantic_id).blending_policy ==
                        BlendingPolicy::normal_vector &&
                    source_image.format().channel_count >= 3) {
                    const auto converted = transformed_normal_pixel(
                        source_pixel, source_image.format(),
                        tangent_frame(source.view(), mapping.source_triangle,
                                      mapping.source_barycentric),
                        tangent_frame(replacement.view(), mapping.target_triangle,
                                      mapping.target_barycentric));
                    staged.write_pixel(mapping.x, mapping.y, converted);
                    ++report.transformed_tangent_normal_count;
                } else {
                    staged.write_pixel(mapping.x, mapping.y, source_pixel);
                }
                ++report.reprojected_texel_count;
                report.resolved_ambiguity_count +=
                    mapping.status == ReprojectionMappingStatus::ambiguous ? 1 : 0;
            }
            staged_channels.push_back(
                {.channels = &channels, .semantic_id = semantic_id, .image = std::move(staged)});
        }
    }

    struct StagedEditable {
        EditableAuthoringStore* destination{};
        EditableAuthoringStore value;
    };
    std::vector<StagedEditable> staged_editable;
    for (const std::string& texture_set_id : document.texture_set_ids()) {
        TextureSet& texture_set = document.texture_set(texture_set_id);
        EditableAuthoringStore candidate = texture_set.editable_authoring();
        bool changed = false;
        for (const ReprojectionEntryMapping& mapping : preflight.affected_entries) {
            if (mapping.texture_set_id != texture_set_id) {
                continue;
            }
            EditableAuthoringEntry replacement_entry = candidate.entry(mapping.entry_id);
            for (std::size_t index = 0; index < mapping.points.size(); ++index) {
                replacement_entry.surface_points[index].position = mapping.points[index].position;
                replacement_entry.surface_points[index].normal = mapping.points[index].normal;
                replacement_entry.surface_points[index].triangle = mapping.points[index].triangle;
                replacement_entry.surface_points[index].barycentric =
                    mapping.points[index].barycentric;
            }
            replacement_entry.mesh_revision = preflight.published_revision;
            static_cast<void>(candidate.edit(std::move(replacement_entry),
                                             candidate.entry(mapping.entry_id).revision));
            report.reprojected_entries.push_back(mapping.entry_id);
            changed = true;
        }
        if (changed) {
            staged_editable.push_back(
                {.destination = &texture_set.editable_authoring(), .value = std::move(candidate)});
        }
    }

    for (StagedChannel& channel : staged_channels) {
        channel.channels->replace_pixels(channel.semantic_id, std::move(channel.image));
    }
    for (StagedEditable& editable : staged_editable) {
        *editable.destination = std::move(editable.value);
    }
    return report;
}

}  // namespace ctex::doc
