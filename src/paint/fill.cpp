#include <algorithm>
#include <cmath>
#include <ctex/paint/fill.hpp>
#include <deque>
#include <limits>
#include <map>
#include <numbers>
#include <set>
#include <stdexcept>
#include <string>
#include <utility>

namespace ctex::paint {
namespace {

std::size_t checked_texel_count(const CachedSurfaceMaps& maps) {
    const std::uint32_t width = maps.surface.width;
    const std::uint32_t height = maps.surface.height;
    if (width == 0 || height == 0 ||
        static_cast<std::size_t>(width) > std::numeric_limits<std::size_t>::max() / height) {
        throw std::invalid_argument("fill surface dimensions are invalid");
    }
    const std::size_t count = static_cast<std::size_t>(width) * height;
    if (maps.surface.texels.size() != count || maps.coverage.size() != count ||
        maps.triangle_identity.size() != count || maps.uv_island_identity.size() != count) {
        throw std::invalid_argument("fill surface maps have inconsistent dimensions");
    }
    for (std::size_t texel = 0; texel < count; ++texel) {
        if (maps.coverage[texel] > 1 ||
            (maps.coverage[texel] == 0 && (maps.triangle_identity[texel] != no_surface_triangle ||
                                           maps.uv_island_identity[texel] != no_uv_island)) ||
            (maps.coverage[texel] == 1 && (maps.triangle_identity[texel] == no_surface_triangle ||
                                           maps.uv_island_identity[texel] == no_uv_island))) {
            throw std::invalid_argument("fill surface identities disagree with coverage");
        }
    }
    return count;
}

std::size_t picked_texel(const FillScopeRequest& request, const CachedSurfaceMaps& maps) {
    if (!request.picked_texel || *request.picked_texel >= maps.coverage.size() ||
        maps.coverage[*request.picked_texel] == 0) {
        throw std::invalid_argument("fill scope requires a covered picked texel");
    }
    return *request.picked_texel;
}

bool finite(Vec3d value) {
    return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z);
}

double dot(Vec3d left, Vec3d right) {
    return left.x * right.x + left.y * right.y + left.z * right.z;
}

bool unit(Vec3d value) { return finite(value) && std::abs(dot(value, value) - 1.0) <= 1.0e-6; }

using TopologyIndex = std::map<std::uint32_t, const FillTriangleTopology*>;

TopologyIndex index_topology(std::span<const FillTriangleTopology> topology) {
    TopologyIndex result;
    for (const FillTriangleTopology& triangle : topology) {
        if (triangle.triangle_identity == no_surface_triangle || !unit(triangle.geometric_normal) ||
            !result.emplace(triangle.triangle_identity, &triangle).second) {
            throw std::invalid_argument("fill triangle topology is invalid or duplicated");
        }
    }
    for (const auto& [identity, triangle] : result) {
        for (const std::uint32_t neighbor : triangle->adjacent_triangles) {
            if (neighbor == identity || !result.contains(neighbor)) {
                throw std::invalid_argument("fill triangle adjacency is invalid");
            }
        }
    }
    return result;
}

std::set<std::uint32_t> connected_triangles(std::uint32_t seed,
                                            std::span<const FillTriangleTopology> triangle_topology,
                                            double maximum_angle_degrees) {
    const TopologyIndex topology = index_topology(triangle_topology);
    if (!topology.contains(seed)) {
        throw std::invalid_argument("picked triangle is absent from fill topology");
    }
    const double minimum_dot = std::cos(maximum_angle_degrees * std::numbers::pi / 180.0);
    std::set<std::uint32_t> selected{seed};
    std::deque<std::uint32_t> pending{seed};
    while (!pending.empty()) {
        const std::uint32_t current_id = pending.front();
        pending.pop_front();
        const FillTriangleTopology& current = *topology.at(current_id);
        for (const std::uint32_t neighbor_id : current.adjacent_triangles) {
            const FillTriangleTopology& neighbor = *topology.at(neighbor_id);
            if (!selected.contains(neighbor_id) &&
                dot(current.geometric_normal, neighbor.geometric_normal) >= minimum_dot) {
                selected.insert(neighbor_id);
                pending.push_back(neighbor_id);
            }
        }
    }
    return selected;
}

std::int64_t uv_tile(double coordinate) {
    if (!std::isfinite(coordinate) ||
        coordinate < static_cast<double>(std::numeric_limits<std::int32_t>::min()) ||
        coordinate > static_cast<double>(std::numeric_limits<std::int32_t>::max())) {
        throw std::invalid_argument("fill UV coordinate cannot identify a tile");
    }
    return static_cast<std::int64_t>(std::floor(coordinate));
}

void select_triangles(const CachedSurfaceMaps& maps, std::span<double> values,
                      const std::set<std::uint32_t>& triangles) {
    for (std::size_t texel = 0; texel < values.size(); ++texel) {
        if (maps.coverage[texel] != 0 && triangles.contains(maps.triangle_identity[texel])) {
            values[texel] = 1.0;
        }
    }
}

void select_island(const CachedSurfaceMaps& maps, std::span<double> values, std::uint32_t island) {
    for (std::size_t texel = 0; texel < values.size(); ++texel) {
        if (maps.coverage[texel] != 0 && maps.uv_island_identity[texel] == island) {
            values[texel] = 1.0;
        }
    }
}

void select_uv_tile(const CachedSurfaceMaps& maps, std::span<double> values, std::size_t picked) {
    const std::int64_t picked_u = uv_tile(maps.surface.texels[picked].uv.x);
    const std::int64_t picked_v = uv_tile(maps.surface.texels[picked].uv.y);
    for (std::size_t texel = 0; texel < values.size(); ++texel) {
        const Vec2d uv = maps.surface.texels[texel].uv;
        if (maps.coverage[texel] != 0 && uv_tile(uv.x) == picked_u && uv_tile(uv.y) == picked_v) {
            values[texel] = 1.0;
        }
    }
}

void select_caller_selection(const CachedSurfaceMaps& maps, std::span<double> values,
                             const FillScopeRequest& request) {
    if (!request.selection || request.selection->values.size() != values.size()) {
        throw std::invalid_argument("selection fill requires one selection value per texel");
    }
    for (std::size_t texel = 0; texel < values.size(); ++texel) {
        const double selected = request.selection->values[texel];
        if (!std::isfinite(selected) || selected < 0.0 || selected > 1.0) {
            throw std::invalid_argument("selection fill values must be normalized");
        }
        values[texel] = maps.coverage[texel] == 0 ? 0.0 : selected;
    }
}

void validate_scope(FillScope scope) {
    if (scope != FillScope::whole_set && scope != FillScope::triangle &&
        scope != FillScope::connected_by_angle && scope != FillScope::uv_island &&
        scope != FillScope::uv_tile && scope != FillScope::selection) {
        throw std::invalid_argument("fill scope is invalid");
    }
}

void multiply_restriction(std::span<double> values, PaintMaskView restriction,
                          std::string_view role) {
    if (restriction.values.size() != values.size()) {
        throw std::invalid_argument(std::string(role) + " must contain one value per texel");
    }
    for (std::size_t texel = 0; texel < values.size(); ++texel) {
        const double value = restriction.values[texel];
        if (!std::isfinite(value) || value < 0.0 || value > 1.0) {
            throw std::invalid_argument(std::string(role) + " values must be normalized");
        }
        values[texel] *= value;
    }
}

}  // namespace

FillScopeResult resolve_fill_scope(const CachedSurfaceMaps& surface,
                                   const FillScopeRequest& request) {
    const std::size_t texel_count = checked_texel_count(surface);
    validate_scope(request.scope);
    FillScopeResult result{.width = surface.surface.width,
                           .height = surface.surface.height,
                           .scope = request.scope,
                           .parameter_report = {},
                           .values = std::vector<double>(texel_count, 0.0),
                           .selected_triangle_ids = {},
                           .selected_texel_count = 0};
    if (request.scope == FillScope::whole_set) {
        for (std::size_t texel = 0; texel < texel_count; ++texel) {
            result.values[texel] = surface.coverage[texel];
        }
    } else if (request.scope == FillScope::selection) {
        select_caller_selection(surface, result.values, request);
    } else {
        const std::size_t picked = picked_texel(request, surface);
        const std::uint32_t seed_triangle = surface.triangle_identity[picked];
        if (request.scope == FillScope::triangle) {
            select_triangles(surface, result.values, std::set<std::uint32_t>{seed_triangle});
            result.selected_triangle_ids = {seed_triangle};
        } else if (request.scope == FillScope::connected_by_angle) {
            const double maximum_angle = validate_tool_parameter(
                connected_angle_parameter, request.maximum_angle_degrees, result.parameter_report);
            const std::set<std::uint32_t> selected =
                connected_triangles(seed_triangle, request.triangle_topology, maximum_angle);
            select_triangles(surface, result.values, selected);
            result.selected_triangle_ids = {selected.begin(), selected.end()};
        } else if (request.scope == FillScope::uv_island) {
            select_island(surface, result.values, surface.uv_island_identity[picked]);
        } else {
            select_uv_tile(surface, result.values, picked);
        }
    }
    result.selected_texel_count = static_cast<std::size_t>(std::count_if(
        result.values.begin(), result.values.end(), [](double value) { return value > 0.0; }));
    return result;
}

FillResult apply_fill(const CachedSurfaceMaps& surface,
                      std::span<const PaintToolChannelRaster> enabled_layer_snapshot,
                      std::span<const PaintToolChannelRaster> material,
                      const FillSettings& settings) {
    FillResult result{.resolved_scope = resolve_fill_scope(surface, settings.scope),
                      .strength = {},
                      .channels = {},
                      .applied_channel_ids = {}};
    result.strength = result.resolved_scope.values;
    const CombinedPaintMask masks = combine_paint_masks(
        result.resolved_scope.width, result.resolved_scope.height, settings.masks);
    multiply_restriction(result.strength, PaintMaskView{masks.values}, "fill masks");
    if (settings.rejection_acceptance) {
        multiply_restriction(result.strength, *settings.rejection_acceptance,
                             "fill rejection acceptance");
    }
    PaintToolShadeResult shaded = shade_paint_tool_channels(
        result.resolved_scope.width, result.resolved_scope.height, enabled_layer_snapshot, material,
        result.strength, settings.blend_mode);
    result.channels = std::move(shaded.channels);
    result.applied_channel_ids = std::move(shaded.applied_channel_ids);
    return result;
}

}  // namespace ctex::paint
