#include <algorithm>
#include <array>
#include <cmath>
#include <ctex/paint/coverage.hpp>
#include <stdexcept>
#include <string>

namespace ctex::paint {
namespace {

constexpr double geometry_epsilon = 1.0e-12;

bool finite(double value) { return std::isfinite(value); }

bool finite(Vec2d value) { return finite(value.x) && finite(value.y); }

bool finite(Vec3d value) { return finite(value.x) && finite(value.y) && finite(value.z); }

Vec3d add(Vec3d left, Vec3d right) {
    return {left.x + right.x, left.y + right.y, left.z + right.z};
}

Vec3d subtract(Vec3d left, Vec3d right) {
    return {left.x - right.x, left.y - right.y, left.z - right.z};
}

Vec3d multiply(Vec3d value, double scale) {
    return {value.x * scale, value.y * scale, value.z * scale};
}

double dot(Vec3d left, Vec3d right) {
    return left.x * right.x + left.y * right.y + left.z * right.z;
}

double length(Vec3d value) { return std::sqrt(dot(value, value)); }

Vec3d normalized(Vec3d value, std::string_view role) {
    const double magnitude = length(value);
    if (!finite(value) || magnitude <= geometry_epsilon) {
        throw std::invalid_argument(std::string(role) + " must be finite and non-zero");
    }
    return multiply(value, 1.0 / magnitude);
}

double edge(Vec2d first, Vec2d second, Vec2d point) {
    return (point.x - first.x) * (second.y - first.y) - (point.y - first.y) * (second.x - first.x);
}

std::size_t checked_texel_count(TextureSpaceRasterRequest request) {
    if (request.width == 0 || request.height == 0) {
        throw std::invalid_argument("texture-space raster dimensions must be non-zero");
    }
    if (!finite(request.tile_origin) ||
        static_cast<std::size_t>(request.width) >
            std::numeric_limits<std::size_t>::max() / static_cast<std::size_t>(request.height)) {
        throw std::invalid_argument("texture-space raster request is invalid");
    }
    return static_cast<std::size_t>(request.width) * request.height;
}

void validate_mesh(TextureSpaceMeshView mesh) {
    const bool complete = !mesh.positions.empty() && mesh.positions.size() == mesh.normals.size() &&
                          mesh.positions.size() == mesh.uv.size() &&
                          !mesh.triangle_indices.empty() && mesh.triangle_indices.size() % 3 == 0;
    if (!complete) {
        throw std::invalid_argument(
            "texture-space mesh requires matching positions, normals, UVs and triangles");
    }
    for (std::size_t index = 0; index < mesh.positions.size(); ++index) {
        if (!finite(mesh.positions[index]) || !finite(mesh.normals[index]) ||
            length(mesh.normals[index]) <= geometry_epsilon || !finite(mesh.uv[index])) {
            throw std::invalid_argument("texture-space mesh attributes must be finite");
        }
    }
    for (const std::uint32_t index : mesh.triangle_indices) {
        if (index >= mesh.positions.size()) {
            throw std::out_of_range("texture-space triangle index is outside its vertices");
        }
    }
}

struct PixelBounds {
    std::uint32_t minimum_x;
    std::uint32_t maximum_x;
    std::uint32_t minimum_y;
    std::uint32_t maximum_y;
};

PixelBounds pixel_bounds(const std::array<Vec2d, 3>& points, std::uint32_t width,
                         std::uint32_t height) {
    const auto x = [width](double value) {
        return static_cast<std::uint32_t>(
            std::clamp(std::floor(value), 0.0, static_cast<double>(width - 1)));
    };
    const auto y = [height](double value) {
        return static_cast<std::uint32_t>(
            std::clamp(std::floor(value), 0.0, static_cast<double>(height - 1)));
    };
    return {.minimum_x = x(std::min({points[0].x, points[1].x, points[2].x})),
            .maximum_x = x(std::max({points[0].x, points[1].x, points[2].x})),
            .minimum_y = y(std::min({points[0].y, points[1].y, points[2].y})),
            .maximum_y = y(std::max({points[0].y, points[1].y, points[2].y}))};
}

std::array<double, 3> barycentric(const std::array<Vec2d, 3>& points, Vec2d point, double area) {
    return {edge(points[1], points[2], point) / area, edge(points[2], points[0], point) / area,
            edge(points[0], points[1], point) / area};
}

bool contains(const std::array<double, 3>& weights) {
    return weights[0] >= -geometry_epsilon && weights[1] >= -geometry_epsilon &&
           weights[2] >= -geometry_epsilon;
}

Vec3d interpolate(const std::array<Vec3d, 3>& values, const std::array<double, 3>& weights) {
    return add(add(multiply(values[0], weights[0]), multiply(values[1], weights[1])),
               multiply(values[2], weights[2]));
}

void rasterize_triangle(TextureSpaceMeshView mesh, TextureSpaceRasterRequest request,
                        std::size_t first_index, TextureSpaceRaster& output) {
    std::array<std::uint32_t, 3> indices{};
    std::array<Vec2d, 3> pixels{};
    std::array<Vec3d, 3> positions{};
    std::array<Vec3d, 3> normals{};
    for (std::size_t corner = 0; corner < 3; ++corner) {
        indices[corner] = mesh.triangle_indices[first_index + corner];
        const Vec2d uv = mesh.uv[indices[corner]];
        pixels[corner] = {
            (uv.x - request.tile_origin.x) * static_cast<double>(request.width),
            (1.0 - (uv.y - request.tile_origin.y)) * static_cast<double>(request.height)};
        positions[corner] = mesh.positions[indices[corner]];
        normals[corner] = mesh.normals[indices[corner]];
    }
    const double area = edge(pixels[0], pixels[1], pixels[2]);
    if (std::abs(area) <= geometry_epsilon) {
        return;
    }
    const PixelBounds bounds = pixel_bounds(pixels, request.width, request.height);
    const std::uint32_t triangle = static_cast<std::uint32_t>(first_index / 3);
    for (std::uint32_t y = bounds.minimum_y; y <= bounds.maximum_y; ++y) {
        for (std::uint32_t x = bounds.minimum_x; x <= bounds.maximum_x; ++x) {
            const auto weights = barycentric(
                pixels, {static_cast<double>(x) + 0.5, static_cast<double>(y) + 0.5}, area);
            const std::size_t texel_index = static_cast<std::size_t>(y) * request.width + x;
            if (!contains(weights) || triangle > output.texels[texel_index].triangle) {
                continue;
            }
            output.texels[texel_index] = {
                .position = interpolate(positions, weights),
                .normal = normalized(interpolate(normals, weights), "interpolated normal"),
                .uv = {request.tile_origin.x + (static_cast<double>(x) + 0.5) / request.width,
                       request.tile_origin.y + 1.0 -
                           (static_cast<double>(y) + 0.5) / request.height},
                .triangle = triangle,
            };
        }
    }
}

void validate_surface(const TextureSpaceRaster& surface) {
    const auto expected = checked_texel_count(
        {.width = surface.width, .height = surface.height, .tile_origin = surface.tile_origin});
    if (surface.texels.size() != expected) {
        throw std::invalid_argument("texture-space raster texel count does not match dimensions");
    }
    for (const SurfaceTexel& texel : surface.texels) {
        if (texel.triangle != no_surface_triangle &&
            (!finite(texel.position) || !finite(texel.normal) || !finite(texel.uv) ||
             length(texel.normal) <= geometry_epsilon)) {
            throw std::invalid_argument("covered surface texels must contain finite geometry");
        }
    }
}

double normalized_stamp_distance(Vec3d point, const Stamp& stamp, bool transformed_tip) {
    const Vec3d delta = subtract(point, stamp.position);
    if (!transformed_tip) {
        return length(delta) / stamp.radius;
    }
    const double cosine = std::cos(stamp.rotation_radians);
    const double sine = std::sin(stamp.rotation_radians);
    const double tangent = dot(delta, stamp.frame.tangent);
    const double bitangent = dot(delta, stamp.frame.bitangent);
    const double normal = dot(delta, stamp.frame.normal);
    const double rotated_tangent = cosine * tangent + sine * bitangent;
    const double rotated_bitangent = -sine * tangent + cosine * bitangent;
    const double major_radius = stamp.radius * stamp.elongation;
    return std::sqrt((rotated_tangent * rotated_tangent) / (major_radius * major_radius) +
                     (rotated_bitangent * rotated_bitangent + normal * normal) /
                         (stamp.radius * stamp.radius));
}

double stamp_coverage(Vec3d point, const Stamp& stamp, bool transformed_tip) {
    if (stamp.radius == 0.0) {
        return length(subtract(point, stamp.position)) <= geometry_epsilon ? 1.0 : 0.0;
    }
    return brush_falloff(normalized_stamp_distance(point, stamp, transformed_tip), stamp.hardness);
}

double segment_coverage(Vec3d point, const Stamp& start, const Stamp& end) {
    const Vec3d axis = subtract(end.position, start.position);
    const double squared_length = dot(axis, axis);
    if (squared_length <= geometry_epsilon) {
        return std::max(stamp_coverage(point, start, false), stamp_coverage(point, end, false));
    }
    const double amount =
        std::clamp(dot(subtract(point, start.position), axis) / squared_length, 0.0, 1.0);
    const Vec3d closest = add(start.position, multiply(axis, amount));
    const double radius = std::lerp(start.radius, end.radius, amount);
    if (radius == 0.0) {
        return length(subtract(point, closest)) <= geometry_epsilon ? 1.0 : 0.0;
    }
    const double hardness = std::lerp(start.hardness, end.hardness, amount);
    return brush_falloff(length(subtract(point, closest)) / radius, hardness);
}

double coverage_at(Vec3d point, const ResolvedStroke& stroke) {
    const bool transformed_tip = stroke.tip_mode == TipMode::discrete_alpha;
    double coverage = 0.0;
    for (const Stamp& stamp : stroke.stamps) {
        coverage = std::max(coverage, stamp_coverage(point, stamp, transformed_tip));
    }
    if (stroke.tip_mode == TipMode::continuous_sweep) {
        for (const SweptSegment segment : stroke.swept_segments) {
            coverage = std::max(coverage,
                                segment_coverage(point, stroke.stamps[segment.start_stamp_ordinal],
                                                 stroke.stamps[segment.end_stamp_ordinal]));
        }
    }
    return coverage;
}

void validate_planar_frame(PlanarProjectionFrame frame) {
    if (!finite(frame.origin) || !finite(frame.u_axis) || !finite(frame.v_axis) ||
        std::abs(length(frame.u_axis) - 1.0) > stroke_position_tolerance ||
        std::abs(length(frame.v_axis) - 1.0) > stroke_position_tolerance ||
        std::abs(dot(frame.u_axis, frame.v_axis)) > stroke_position_tolerance) {
        throw std::invalid_argument("planar projection frame must be finite and orthonormal");
    }
}

}  // namespace

bool TextureSpaceRaster::covered(std::size_t index) const {
    if (index >= texels.size()) {
        throw std::out_of_range("texture-space raster texel index is out of range");
    }
    return texels[index].triangle != no_surface_triangle;
}

TextureSpaceRaster rasterize_texture_space(TextureSpaceMeshView mesh,
                                           TextureSpaceRasterRequest request) {
    validate_mesh(mesh);
    TextureSpaceRaster output{
        .width = request.width,
        .height = request.height,
        .tile_origin = request.tile_origin,
        .texels = std::vector<SurfaceTexel>(checked_texel_count(request)),
    };
    for (std::size_t first = 0; first < mesh.triangle_indices.size(); first += 3) {
        rasterize_triangle(mesh, request, first, output);
    }
    return output;
}

double brush_falloff(double normalized_distance, double hardness) {
    if (!finite(normalized_distance) || normalized_distance < 0.0 || !finite(hardness) ||
        hardness < 0.0 || hardness > 1.0) {
        throw std::invalid_argument("falloff distance and hardness must be normalized and finite");
    }
    if (normalized_distance > 1.0) {
        return 0.0;
    }
    if (hardness == 1.0 || normalized_distance <= hardness) {
        return 1.0;
    }
    const double t2 = std::clamp((normalized_distance - hardness) / (1.0 - hardness), 0.0, 1.0);
    return 1.0 - t2 * t2 * (3.0 - 2.0 * t2);
}

CoverageRaster evaluate_stroke_coverage(const TextureSpaceRaster& surface,
                                        const ResolvedStroke& stroke) {
    validate_surface(surface);
    const ResolvedStroke validated = ingest_resolved_stroke(stroke);
    CoverageRaster output{.width = surface.width,
                          .height = surface.height,
                          .values = std::vector<double>(surface.texels.size(), 0.0)};
    for (std::size_t index = 0; index < surface.texels.size(); ++index) {
        if (surface.covered(index)) {
            output.values[index] = coverage_at(surface.texels[index].position, validated);
        }
    }
    return output;
}

MaterialCoordinates material_coordinates(const SurfaceTexel& texel,
                                         const MaterialCoordinateRequest& request) {
    if (texel.triangle == no_surface_triangle || !finite(texel.position) || !finite(texel.uv)) {
        throw std::invalid_argument("material coordinates require a covered finite texel");
    }
    if (request.mode == MaterialCoordinateMode::uv) {
        return {.projections = {{{.coordinate = texel.uv, .weight = 1.0}}}, .count = 1};
    }
    if (request.mode == MaterialCoordinateMode::planar) {
        validate_planar_frame(request.planar);
        const Vec3d relative = subtract(texel.position, request.planar.origin);
        return {.projections = {{{.coordinate = {dot(relative, request.planar.u_axis),
                                                 dot(relative, request.planar.v_axis)},
                                  .weight = 1.0}}},
                .count = 1};
    }
    if (request.mode != MaterialCoordinateMode::triplanar || !finite(texel.normal)) {
        throw std::invalid_argument("material coordinate mode or surface normal is invalid");
    }
    const Vec3d normal = normalized(texel.normal, "surface normal");
    const std::array<double, 3> weights{normal.x * normal.x, normal.y * normal.y,
                                        normal.z * normal.z};
    return {
        .projections = {{{.coordinate = {texel.position.y, texel.position.z}, .weight = weights[0]},
                         {.coordinate = {texel.position.x, texel.position.z}, .weight = weights[1]},
                         {.coordinate = {texel.position.x, texel.position.y},
                          .weight = weights[2]}}},
        .count = 3};
}

}  // namespace ctex::paint
