#include <algorithm>
#include <array>
#include <cmath>
#include <ctex/exec/cpu_reference.hpp>
#include <limits>
#include <stdexcept>
#include <string>
#include <utility>

namespace ctex::exec {
namespace {

constexpr float coverage_epsilon = 1.0e-6F;

struct ClipVertex {
    std::array<float, 4> position;
    CpuVec2f uv;
};

struct ScreenVertex {
    CpuVec2f position;
    float depth;
    float inverse_w;
    CpuVec2f uv_over_w;
};

struct UvVertex {
    CpuVec2f position;
    CpuVec2f screen;
    float depth;
    float clip_w;
};

struct PixelBounds {
    std::uint32_t minimum_x;
    std::uint32_t maximum_x;
    std::uint32_t minimum_y;
    std::uint32_t maximum_y;
};

using ClipPolygon = std::vector<ClipVertex>;

bool finite(float value) { return std::isfinite(value); }

bool finite(CpuVec2f value) { return finite(value.x) && finite(value.y); }

bool finite(CpuVec3f value) { return finite(value.x) && finite(value.y) && finite(value.z); }

std::array<float, 4> transform(const CpuMat4f& matrix, CpuVec3f point) {
    const auto& m = matrix.values;
    return {m[0] * point.x + m[4] * point.y + m[8] * point.z + m[12],
            m[1] * point.x + m[5] * point.y + m[9] * point.z + m[13],
            m[2] * point.x + m[6] * point.y + m[10] * point.z + m[14],
            m[3] * point.x + m[7] * point.y + m[11] * point.z + m[15]};
}

float plane_distance(const ClipVertex& vertex, std::size_t plane) {
    const auto& p = vertex.position;
    switch (plane) {
        case 0:
            return p[3] + p[0];
        case 1:
            return p[3] - p[0];
        case 2:
            return p[3] + p[1];
        case 3:
            return p[3] - p[1];
        case 4:
            return p[3] + p[2];
        default:
            return p[3] - p[2];
    }
}

ClipVertex interpolate(const ClipVertex& first, const ClipVertex& second, float amount) {
    ClipVertex result{};
    for (std::size_t component = 0; component < result.position.size(); ++component) {
        result.position[component] =
            std::lerp(first.position[component], second.position[component], amount);
    }
    result.uv = {std::lerp(first.uv.x, second.uv.x, amount),
                 std::lerp(first.uv.y, second.uv.y, amount)};
    return result;
}

ClipPolygon clip_against_plane(const ClipPolygon& input, std::size_t plane) {
    ClipPolygon output;
    if (input.empty()) {
        return output;
    }
    output.reserve(input.size() + 1);
    ClipVertex previous = input.back();
    float previous_distance = plane_distance(previous, plane);
    for (const ClipVertex& current : input) {
        const float current_distance = plane_distance(current, plane);
        const bool previous_inside = previous_distance >= 0.0F;
        const bool current_inside = current_distance >= 0.0F;
        if (previous_inside != current_inside) {
            const float amount = previous_distance / (previous_distance - current_distance);
            output.push_back(interpolate(previous, current, amount));
        }
        if (current_inside) {
            output.push_back(current);
        }
        previous = current;
        previous_distance = current_distance;
    }
    return output;
}

ClipPolygon clip_triangle(const std::array<ClipVertex, 3>& triangle) {
    ClipPolygon polygon(triangle.begin(), triangle.end());
    for (std::size_t plane = 0; plane < 6 && !polygon.empty(); ++plane) {
        polygon = clip_against_plane(polygon, plane);
    }
    return polygon;
}

float edge(CpuVec2f first, CpuVec2f second, CpuVec2f point) {
    return (point.x - first.x) * (second.y - first.y) - (point.y - first.y) * (second.x - first.x);
}

PixelBounds pixel_bounds(const std::array<CpuVec2f, 3>& points, std::uint32_t width,
                         std::uint32_t height) {
    const float minimum_x = std::min({points[0].x, points[1].x, points[2].x});
    const float maximum_x = std::max({points[0].x, points[1].x, points[2].x});
    const float minimum_y = std::min({points[0].y, points[1].y, points[2].y});
    const float maximum_y = std::max({points[0].y, points[1].y, points[2].y});
    const auto clamp_x = [width](float value) {
        return static_cast<std::uint32_t>(std::clamp(value, 0.0F, static_cast<float>(width - 1)));
    };
    const auto clamp_y = [height](float value) {
        return static_cast<std::uint32_t>(std::clamp(value, 0.0F, static_cast<float>(height - 1)));
    };
    return {.minimum_x = clamp_x(std::floor(minimum_x)),
            .maximum_x = clamp_x(std::floor(maximum_x)),
            .minimum_y = clamp_y(std::floor(minimum_y)),
            .maximum_y = clamp_y(std::floor(maximum_y))};
}

std::array<float, 3> barycentric(const std::array<CpuVec2f, 3>& points, CpuVec2f point,
                                 float area) {
    return {edge(points[1], points[2], point) / area, edge(points[2], points[0], point) / area,
            edge(points[0], points[1], point) / area};
}

bool contains(const std::array<float, 3>& weights) {
    return weights[0] >= -coverage_epsilon && weights[1] >= -coverage_epsilon &&
           weights[2] >= -coverage_epsilon;
}

bool wins_depth(float depth, std::uint32_t triangle, float stored_depth,
                std::uint32_t stored_triangle) {
    return depth < stored_depth || (depth == stored_depth && triangle < stored_triangle);
}

std::size_t checked_pixel_count(std::uint32_t width, std::uint32_t height, std::string_view label) {
    if (width == 0 || height == 0) {
        throw std::invalid_argument(std::string(label) + " dimensions must be non-zero");
    }
    if (static_cast<std::size_t>(width) >
        std::numeric_limits<std::size_t>::max() / static_cast<std::size_t>(height)) {
        throw std::length_error(std::string(label) + " dimensions exceed addressable storage");
    }
    return static_cast<std::size_t>(width) * height;
}

void validate_mesh(const CpuRasterMeshView& mesh) {
    if (mesh.positions.empty() || mesh.positions.size() != mesh.uv.size() ||
        mesh.triangle_indices.empty() || mesh.triangle_indices.size() % 3 != 0) {
        throw std::invalid_argument(
            "CPU raster mesh requires matching positions and UVs and complete triangles");
    }
    if (!std::all_of(mesh.positions.begin(), mesh.positions.end(),
                     [](CpuVec3f value) { return finite(value); }) ||
        !std::all_of(mesh.uv.begin(), mesh.uv.end(),
                     [](CpuVec2f value) { return finite(value); })) {
        throw std::invalid_argument("CPU raster mesh attributes must be finite");
    }
    for (const std::uint32_t index : mesh.triangle_indices) {
        if (index >= mesh.positions.size()) {
            throw std::out_of_range("CPU raster mesh triangle index is outside its vertices");
        }
    }
}

void validate_camera(const CpuRasterCamera& camera) {
    static_cast<void>(checked_pixel_count(camera.width, camera.height, "viewport"));
    if (!std::all_of(camera.view_projection.values.begin(), camera.view_projection.values.end(),
                     [](float value) { return finite(value); })) {
        throw std::invalid_argument("CPU raster camera matrix must be finite");
    }
}

ScreenVertex screen_vertex(const ClipVertex& vertex, const CpuRasterCamera& camera) {
    const float inverse_w = 1.0F / vertex.position[3];
    const float ndc_x = vertex.position[0] * inverse_w;
    const float ndc_y = vertex.position[1] * inverse_w;
    const float ndc_z = vertex.position[2] * inverse_w;
    return {.position = {(ndc_x * 0.5F + 0.5F) * static_cast<float>(camera.width),
                         (0.5F - ndc_y * 0.5F) * static_cast<float>(camera.height)},
            .depth = ndc_z * 0.5F + 0.5F,
            .inverse_w = inverse_w,
            .uv_over_w = {vertex.uv.x * inverse_w, vertex.uv.y * inverse_w}};
}

void rasterize_screen_triangle(const std::array<ScreenVertex, 3>& vertices, std::uint32_t triangle,
                               CpuViewportRaster& output) {
    const std::array<CpuVec2f, 3> points{vertices[0].position, vertices[1].position,
                                         vertices[2].position};
    const float area = edge(points[0], points[1], points[2]);
    if (std::abs(area) <= coverage_epsilon) {
        return;
    }
    const PixelBounds bounds = pixel_bounds(points, output.width, output.height);
    for (std::uint32_t y = bounds.minimum_y; y <= bounds.maximum_y; ++y) {
        for (std::uint32_t x = bounds.minimum_x; x <= bounds.maximum_x; ++x) {
            const auto weights = barycentric(
                points, {static_cast<float>(x) + 0.5F, static_cast<float>(y) + 0.5F}, area);
            if (!contains(weights)) {
                continue;
            }
            const float depth = weights[0] * vertices[0].depth + weights[1] * vertices[1].depth +
                                weights[2] * vertices[2].depth;
            const std::size_t pixel = static_cast<std::size_t>(y) * output.width + x;
            if (!wins_depth(depth, triangle, output.depth[pixel], output.triangle[pixel])) {
                continue;
            }
            const float reciprocal = weights[0] * vertices[0].inverse_w +
                                     weights[1] * vertices[1].inverse_w +
                                     weights[2] * vertices[2].inverse_w;
            output.depth[pixel] = depth;
            output.uv[pixel] = {
                (weights[0] * vertices[0].uv_over_w.x + weights[1] * vertices[1].uv_over_w.x +
                 weights[2] * vertices[2].uv_over_w.x) /
                    reciprocal,
                (weights[0] * vertices[0].uv_over_w.y + weights[1] * vertices[1].uv_over_w.y +
                 weights[2] * vertices[2].uv_over_w.y) /
                    reciprocal};
            output.coverage[pixel] = 1;
            output.triangle[pixel] = triangle;
        }
    }
}

std::array<ClipVertex, 3> source_triangle(const CpuRasterMeshView& mesh,
                                          const CpuRasterCamera& camera, std::size_t first_index) {
    std::array<ClipVertex, 3> result{};
    for (std::size_t corner = 0; corner < 3; ++corner) {
        const std::uint32_t vertex = mesh.triangle_indices[first_index + corner];
        result[corner] = {.position = transform(camera.view_projection, mesh.positions[vertex]),
                          .uv = mesh.uv[vertex]};
        if (!std::all_of(result[corner].position.begin(), result[corner].position.end(),
                         [](float value) { return finite(value); })) {
            throw std::invalid_argument("CPU raster camera transform must remain finite");
        }
    }
    return result;
}

CpuViewportRaster make_viewport_output(const CpuRasterCamera& camera) {
    const std::size_t count = checked_pixel_count(camera.width, camera.height, "viewport");
    return {.width = camera.width,
            .height = camera.height,
            .depth = std::vector<float>(count, 1.0F),
            .uv = std::vector<CpuVec2f>(count, CpuVec2f{}),
            .coverage = std::vector<std::uint8_t>(count, 0),
            .triangle = std::vector<std::uint32_t>(count, no_raster_triangle)};
}

CpuVec2f projected_screen(const std::array<float, 4>& clip, const CpuRasterCamera& camera) {
    const float inverse_w = 1.0F / clip[3];
    return {(clip[0] * inverse_w * 0.5F + 0.5F) * static_cast<float>(camera.width),
            (0.5F - clip[1] * inverse_w * 0.5F) * static_cast<float>(camera.height)};
}

float projected_depth(const std::array<float, 4>& clip) { return clip[2] / clip[3] * 0.5F + 0.5F; }

std::array<UvVertex, 3> uv_triangle(const CpuRasterMeshView& mesh, const CpuRasterCamera& camera,
                                    const CpuUvRasterRequest& request, std::size_t first_index) {
    std::array<UvVertex, 3> result{};
    for (std::size_t corner = 0; corner < 3; ++corner) {
        const std::uint32_t vertex = mesh.triangle_indices[first_index + corner];
        const auto clip = transform(camera.view_projection, mesh.positions[vertex]);
        const CpuVec2f uv = mesh.uv[vertex];
        result[corner] = {
            .position = {(uv.x - request.tile_origin.x) * static_cast<float>(request.width),
                         (1.0F - (uv.y - request.tile_origin.y)) *
                             static_cast<float>(request.height)},
            .screen = projected_screen(clip, camera),
            .depth = projected_depth(clip),
            .clip_w = clip[3],
        };
    }
    return result;
}

bool valid_projection(const std::array<ClipVertex, 3>& triangle) {
    return std::all_of(triangle.begin(), triangle.end(), [](const ClipVertex& vertex) {
        return vertex.position[3] > coverage_epsilon;
    });
}

void rasterize_uv_triangle(const std::array<UvVertex, 3>& vertices, std::uint32_t triangle,
                           CpuUvRaster& output) {
    const std::array<CpuVec2f, 3> points{vertices[0].position, vertices[1].position,
                                         vertices[2].position};
    const float area = edge(points[0], points[1], points[2]);
    if (std::abs(area) <= coverage_epsilon) {
        return;
    }
    const PixelBounds bounds = pixel_bounds(points, output.width, output.height);
    for (std::uint32_t y = bounds.minimum_y; y <= bounds.maximum_y; ++y) {
        for (std::uint32_t x = bounds.minimum_x; x <= bounds.maximum_x; ++x) {
            const auto weights = barycentric(
                points, {static_cast<float>(x) + 0.5F, static_cast<float>(y) + 0.5F}, area);
            if (!contains(weights)) {
                continue;
            }
            const std::size_t pixel = static_cast<std::size_t>(y) * output.width + x;
            if (triangle > output.triangle[pixel]) {
                continue;
            }
            const float clip_w = weights[0] * vertices[0].clip_w + weights[1] * vertices[1].clip_w +
                                 weights[2] * vertices[2].clip_w;
            output.screen_position[pixel] = {
                (weights[0] * vertices[0].screen.x * vertices[0].clip_w +
                 weights[1] * vertices[1].screen.x * vertices[1].clip_w +
                 weights[2] * vertices[2].screen.x * vertices[2].clip_w) /
                    clip_w,
                (weights[0] * vertices[0].screen.y * vertices[0].clip_w +
                 weights[1] * vertices[1].screen.y * vertices[1].clip_w +
                 weights[2] * vertices[2].screen.y * vertices[2].clip_w) /
                    clip_w};
            output.depth[pixel] = (weights[0] * vertices[0].depth * vertices[0].clip_w +
                                   weights[1] * vertices[1].depth * vertices[1].clip_w +
                                   weights[2] * vertices[2].depth * vertices[2].clip_w) /
                                  clip_w;
            output.coverage[pixel] = 1;
            output.triangle[pixel] = triangle;
        }
    }
}

CpuUvRaster make_uv_output(CpuUvRasterRequest request) {
    const std::size_t count = checked_pixel_count(request.width, request.height, "UV raster");
    if (!finite(request.tile_origin)) {
        throw std::invalid_argument("UV raster tile origin must be finite");
    }
    return {.width = request.width,
            .height = request.height,
            .tile_origin = request.tile_origin,
            .depth = std::vector<float>(count, 1.0F),
            .screen_position = std::vector<CpuVec2f>(count, CpuVec2f{}),
            .coverage = std::vector<std::uint8_t>(count, 0),
            .triangle = std::vector<std::uint32_t>(count, no_raster_triangle)};
}

}  // namespace

CpuReferenceExecutor::CpuReferenceExecutor()
    : descriptor_{.identifier = "cpu",
                  .display_name = "CPU reference",
                  .device_name = "System CPU",
                  .route = ExecutorRoute::cpu_reference,
                  .availability = ExecutorAvailability::available} {}

const ExecutorDescriptor& CpuReferenceExecutor::descriptor() const noexcept { return descriptor_; }

CpuExecutionRecord CpuReferenceExecutor::execute(CpuOperation& operation) const {
    const std::string identifier(operation.identifier());
    if (identifier.empty()) {
        throw std::invalid_argument("CPU operation identifier must not be empty");
    }
    operation.execute(*this);
    return {.operation = identifier, .completed = true};
}

CpuViewportRaster CpuReferenceExecutor::rasterize_viewport(const CpuRasterMeshView& mesh,
                                                           const CpuRasterCamera& camera) const {
    validate_mesh(mesh);
    validate_camera(camera);
    CpuViewportRaster output = make_viewport_output(camera);
    for (std::size_t first = 0; first < mesh.triangle_indices.size(); first += 3) {
        const auto source = source_triangle(mesh, camera, first);
        const ClipPolygon clipped = clip_triangle(source);
        for (std::size_t corner = 1; corner + 1 < clipped.size(); ++corner) {
            if (clipped[0].position[3] <= coverage_epsilon ||
                clipped[corner].position[3] <= coverage_epsilon ||
                clipped[corner + 1].position[3] <= coverage_epsilon) {
                continue;
            }
            const std::array<ScreenVertex, 3> triangle{screen_vertex(clipped[0], camera),
                                                       screen_vertex(clipped[corner], camera),
                                                       screen_vertex(clipped[corner + 1], camera)};
            rasterize_screen_triangle(triangle, static_cast<std::uint32_t>(first / 3), output);
        }
    }
    return output;
}

CpuUvRaster CpuReferenceExecutor::rasterize_uv(const CpuRasterMeshView& mesh,
                                               const CpuRasterCamera& camera,
                                               CpuUvRasterRequest request) const {
    validate_mesh(mesh);
    validate_camera(camera);
    CpuUvRaster output = make_uv_output(request);
    for (std::size_t first = 0; first < mesh.triangle_indices.size(); first += 3) {
        const auto source = source_triangle(mesh, camera, first);
        if (!valid_projection(source)) {
            continue;
        }
        rasterize_uv_triangle(uv_triangle(mesh, camera, request, first),
                              static_cast<std::uint32_t>(first / 3), output);
    }
    return output;
}

}  // namespace ctex::exec
