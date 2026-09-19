#include <algorithm>
#include <array>
#include <cmath>
#include <ctex/paint/selection.hpp>
#include <limits>
#include <optional>
#include <set>
#include <stdexcept>

namespace ctex::paint {
namespace {

constexpr double screen_epsilon = 1.0e-9;

struct Point2d {
    double x;
    double y;
};

struct Vec4d {
    double x;
    double y;
    double z;
    double w;
};

Vec4d transform(const pick::Mat4f& matrix, Vec4d vector) {
    const std::array input{vector.x, vector.y, vector.z, vector.w};
    std::array<double, 4> output{};
    for (std::size_t row = 0; row < 4; ++row) {
        for (std::size_t column = 0; column < 4; ++column) {
            output[row] += matrix.values[column * 4 + row] * input[column];
        }
    }
    return {output[0], output[1], output[2], output[3]};
}

std::optional<Point2d> project(Vec3d position, const pick::ScreenRegionView& view) {
    const Vec4d viewed = transform(view.view, {position.x, position.y, position.z, 1.0});
    const Vec4d clip = transform(view.projection, viewed);
    if (!std::isfinite(clip.x) || !std::isfinite(clip.y) || !std::isfinite(clip.z) ||
        !std::isfinite(clip.w) || clip.w <= screen_epsilon || clip.x < -clip.w || clip.x > clip.w ||
        clip.y < -clip.w || clip.y > clip.w || clip.z < -clip.w || clip.z > clip.w) {
        return std::nullopt;
    }
    return Point2d{(clip.x / clip.w + 1.0) * 0.5 * view.viewport.width,
                   (1.0 - clip.y / clip.w) * 0.5 * view.viewport.height};
}

double orientation(Point2d first, Point2d second, Point2d third) {
    return (second.x - first.x) * (third.y - first.y) - (second.y - first.y) * (third.x - first.x);
}

bool on_segment(Point2d point, Point2d first, Point2d second) {
    return std::abs(orientation(first, second, point)) <= screen_epsilon &&
           point.x >= std::min(first.x, second.x) - screen_epsilon &&
           point.x <= std::max(first.x, second.x) + screen_epsilon &&
           point.y >= std::min(first.y, second.y) - screen_epsilon &&
           point.y <= std::max(first.y, second.y) + screen_epsilon;
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
        if ((first.y > point.y) != (second.y > point.y) &&
            point.x < (second.x - first.x) * (point.y - first.y) / (second.y - first.y) + first.x) {
            inside = !inside;
        }
    }
    return inside;
}

std::size_t validate_surface(const CachedSurfaceMaps& surface) {
    const std::uint32_t width = surface.surface.width;
    const std::uint32_t height = surface.surface.height;
    if (surface.texture_set_id.empty() || surface.uv_set.empty() || width == 0 || height == 0 ||
        !std::isfinite(surface.surface.tile_origin.x) ||
        !std::isfinite(surface.surface.tile_origin.y) ||
        static_cast<std::size_t>(width) > std::numeric_limits<std::size_t>::max() / height) {
        throw std::invalid_argument("selection surface identity or dimensions are invalid");
    }
    const std::size_t count = static_cast<std::size_t>(width) * height;
    if (surface.surface.texels.size() != count || surface.coverage.size() != count ||
        surface.triangle_identity.size() != count || surface.uv_island_identity.size() != count) {
        throw std::invalid_argument("selection surface maps have inconsistent dimensions");
    }
    for (std::size_t texel = 0; texel < count; ++texel) {
        const bool covered = surface.coverage[texel] == 1;
        const Vec3d position = surface.surface.texels[texel].position;
        if (surface.coverage[texel] > 1 ||
            covered != (surface.triangle_identity[texel] != no_surface_triangle) ||
            covered != (surface.uv_island_identity[texel] != no_uv_island) ||
            surface.surface.texels[texel].triangle != surface.triangle_identity[texel] ||
            !std::isfinite(position.x) || !std::isfinite(position.y) ||
            !std::isfinite(position.z)) {
            throw std::invalid_argument("selection surface identities disagree with coverage");
        }
    }
    return count;
}

SelectionResult select_triangles(const CachedSurfaceMaps& surface, SelectionKind kind,
                                 const pick::RegionQueryResult& query,
                                 std::span<const Point2d> screen_region,
                                 const pick::ScreenRegionView& view) {
    const std::size_t count = validate_surface(surface);
    SelectionResult result{.width = surface.surface.width,
                           .height = surface.surface.height,
                           .kind = kind,
                           .parameter_report = {},
                           .values = std::vector<double>(count, 0.0),
                           .selected_triangle_ids = {},
                           .selected_texel_count = 0,
                           .visited_nodes = query.visited_nodes,
                           .tested_leaf_triangles = query.tested_leaf_triangles};
    std::set<std::uint32_t> represented;
    for (std::size_t texel = 0; texel < count; ++texel) {
        const std::uint32_t triangle = surface.triangle_identity[texel];
        const bool candidate = surface.coverage[texel] != 0 &&
                               std::binary_search(query.triangle_indices.begin(),
                                                  query.triangle_indices.end(), triangle);
        if (!candidate) {
            continue;
        }
        const std::optional<Point2d> screen = project(surface.surface.texels[texel].position, view);
        if (screen && point_in_polygon(*screen, screen_region)) {
            result.values[texel] = 1.0;
            ++result.selected_texel_count;
            represented.insert(triangle);
        }
    }
    result.selected_triangle_ids.assign(represented.begin(), represented.end());
    return result;
}

SelectionKind selection_kind(PolygonSelectionMode mode) {
    switch (mode) {
        case PolygonSelectionMode::triangle:
            return SelectionKind::polygon_triangle;
        case PolygonSelectionMode::uv_island:
            return SelectionKind::polygon_uv_island;
        case PolygonSelectionMode::connected_by_angle:
            return SelectionKind::polygon_connected_by_angle;
    }
    throw std::invalid_argument("polygon selection mode is invalid");
}

FillScope fill_scope(PolygonSelectionMode mode) {
    switch (mode) {
        case PolygonSelectionMode::triangle:
            return FillScope::triangle;
        case PolygonSelectionMode::uv_island:
            return FillScope::uv_island;
        case PolygonSelectionMode::connected_by_angle:
            return FillScope::connected_by_angle;
    }
    throw std::invalid_argument("polygon selection mode is invalid");
}

std::vector<std::uint32_t> represented_triangles(const CachedSurfaceMaps& surface,
                                                 std::span<const double> values) {
    std::set<std::uint32_t> triangles;
    for (std::size_t texel = 0; texel < values.size(); ++texel) {
        if (values[texel] != 0.0) {
            triangles.insert(surface.triangle_identity[texel]);
        }
    }
    return {triangles.begin(), triangles.end()};
}

void validate_selection(const SelectionResult& selection) {
    if (selection.width == 0 || selection.height == 0 ||
        static_cast<std::size_t>(selection.width) >
            std::numeric_limits<std::size_t>::max() / selection.height ||
        selection.values.size() != static_cast<std::size_t>(selection.width) * selection.height ||
        !std::all_of(selection.values.begin(), selection.values.end(),
                     [](double value) { return value == 0.0 || value == 1.0; }) ||
        selection.selected_texel_count !=
            static_cast<std::size_t>(
                std::count(selection.values.begin(), selection.values.end(), 1.0))) {
        throw std::invalid_argument("selection cannot be stored as an inconsistent mask");
    }
}

}  // namespace

SelectionResult select_screen_rectangle(pick::SpatialIndex& index, const mesh::MeshBinding& mesh,
                                        const CachedSurfaceMaps& surface,
                                        pick::ScreenRectangle rectangle,
                                        const pick::ScreenRegionView& view) {
    if (surface.mesh_revision != mesh.revision()) {
        throw std::invalid_argument("screen selection surface uses a stale mesh revision");
    }
    const std::array region{Point2d{rectangle.minimum.x, rectangle.minimum.y},
                            Point2d{rectangle.minimum.x, rectangle.maximum.y},
                            Point2d{rectangle.maximum.x, rectangle.maximum.y},
                            Point2d{rectangle.maximum.x, rectangle.minimum.y}};
    return select_triangles(surface, SelectionKind::screen_rectangle,
                            pick::query_screen_rectangle(index, mesh, rectangle, view), region,
                            view);
}

SelectionResult select_screen_lasso(pick::SpatialIndex& index, const mesh::MeshBinding& mesh,
                                    const CachedSurfaceMaps& surface,
                                    std::span<const pick::ScreenPosition> points,
                                    const pick::ScreenRegionView& view) {
    if (surface.mesh_revision != mesh.revision()) {
        throw std::invalid_argument("screen selection surface uses a stale mesh revision");
    }
    std::vector<Point2d> region;
    region.reserve(points.size());
    for (const pick::ScreenPosition point : points) {
        region.push_back({point.x, point.y});
    }
    return select_triangles(surface, SelectionKind::screen_lasso,
                            pick::query_screen_lasso(index, mesh, points, view), region, view);
}

SelectionResult select_polygon(const CachedSurfaceMaps& surface,
                               const PolygonSelectionRequest& request) {
    validate_surface(surface);
    const SelectionKind kind = selection_kind(request.mode);
    const FillScopeResult scope =
        resolve_fill_scope(surface, {.scope = fill_scope(request.mode),
                                     .picked_texel = request.picked_texel,
                                     .maximum_angle_degrees = request.maximum_angle_degrees,
                                     .triangle_topology = request.triangle_topology,
                                     .selection = std::nullopt});
    return {.width = scope.width,
            .height = scope.height,
            .kind = kind,
            .parameter_report = scope.parameter_report,
            .values = scope.values,
            .selected_triangle_ids = represented_triangles(surface, scope.values),
            .selected_texel_count = scope.selected_texel_count,
            .visited_nodes = 0,
            .tested_leaf_triangles = 0};
}

StoredSelectionMask store_selection_mask(const SelectionResult& selection) {
    validate_selection(selection);
    return {.width = selection.width, .height = selection.height, .values = selection.values};
}

}  // namespace ctex::paint
