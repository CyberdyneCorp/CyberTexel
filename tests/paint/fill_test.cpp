#include <array>
#include <cmath>
#include <ctex/paint/fill.hpp>
#include <iostream>
#include <limits>
#include <optional>
#include <string_view>
#include <utility>
#include <vector>

namespace {

using ctex::graph::ColourValue;
using namespace ctex::paint;

bool expect(bool condition, std::string_view message) {
    if (!condition) {
        std::cerr << message << '\n';
    }
    return condition;
}

SurfaceTexel texel(double u, double v, std::uint32_t triangle) {
    return {.position = {},
            .normal = {0.0, 0.0, 1.0},
            .geometric_normal = {0.0, 0.0, 1.0},
            .uv = {u, v},
            .triangle = triangle};
}

CachedSurfaceMaps surface_maps() {
    return {.texture_set_id = "material:body:uv0",
            .uv_set = "uv0",
            .mesh_revision = 1,
            .surface = {.width = 4,
                        .height = 2,
                        .tile_origin = {},
                        .texels = {texel(0.1, 0.1, 0), texel(0.2, 0.1, 0), texel(1.1, 0.1, 1),
                                   texel(1.2, 0.1, 1), texel(0.3, 0.2, 2), texel(0.4, 0.2, 2),
                                   texel(1.3, 0.2, 3), texel(1.4, 0.2, 3)}},
            .coverage = {1, 1, 1, 1, 1, 1, 1, 1},
            .triangle_identity = {0, 0, 1, 1, 2, 2, 3, 3},
            .uv_island_identity = {10, 10, 10, 10, 20, 20, 30, 30}};
}

std::vector<FillTriangleTopology> topology() {
    constexpr double cosine_30 = 0.8660254037844386;
    constexpr double sine_30 = 0.5;
    return {
        {.triangle_identity = 0, .geometric_normal = {0.0, 0.0, 1.0}, .adjacent_triangles = {1}},
        {.triangle_identity = 1,
         .geometric_normal = {0.0, sine_30, cosine_30},
         .adjacent_triangles = {0, 2}},
        {.triangle_identity = 2, .geometric_normal = {0.0, 1.0, 0.0}, .adjacent_triangles = {1, 3}},
        {.triangle_identity = 3, .geometric_normal = {1.0, 0.0, 0.0}, .adjacent_triangles = {2}},
    };
}

FillScopeRequest scope_request(FillScope scope,
                               std::optional<std::size_t> picked_texel = std::nullopt) {
    return {.scope = scope,
            .picked_texel = picked_texel,
            .maximum_angle_degrees = default_fill_angle_degrees,
            .triangle_topology = {},
            .selection = std::nullopt};
}

bool all_six_fill_scopes_select_their_exact_regions() {
    const CachedSurfaceMaps maps = surface_maps();
    const auto triangles = topology();
    const std::array<double, 8> selection{0.0, 1.0, 0.0, 0.5, 0.0, 1.0, 0.0, 0.0};
    const FillScopeResult whole = resolve_fill_scope(maps, scope_request(FillScope::whole_set));
    const FillScopeResult face = resolve_fill_scope(maps, scope_request(FillScope::triangle, 0));
    FillScopeRequest connected_request = scope_request(FillScope::connected_by_angle, 0);
    connected_request.triangle_topology = triangles;
    const FillScopeResult connected = resolve_fill_scope(maps, connected_request);
    const FillScopeResult island = resolve_fill_scope(maps, scope_request(FillScope::uv_island, 0));
    const FillScopeResult tile = resolve_fill_scope(maps, scope_request(FillScope::uv_tile, 0));
    FillScopeRequest selection_request = scope_request(FillScope::selection);
    selection_request.selection = PaintMaskView{selection};
    const FillScopeResult selected = resolve_fill_scope(maps, selection_request);

    return expect(whole.values == std::vector<double>(8, 1.0) && whole.selected_texel_count == 8,
                  "whole-set fill did not select every covered texel") &&
           expect(face.values == std::vector<double>({1, 1, 0, 0, 0, 0, 0, 0}) &&
                      face.selected_triangle_ids == std::vector<std::uint32_t>({0}),
                  "triangle fill did not use exact triangle identity") &&
           expect(connected.values == std::vector<double>({1, 1, 1, 1, 0, 0, 0, 0}) &&
                      connected.selected_triangle_ids == std::vector<std::uint32_t>({0, 1}),
                  "connected fill crossed an angle boundary or missed an adjacent face") &&
           expect(island.values == std::vector<double>({1, 1, 1, 1, 0, 0, 0, 0}),
                  "UV-island fill touched another cached island") &&
           expect(tile.values == std::vector<double>({1, 1, 0, 0, 1, 1, 0, 0}),
                  "UV-tile fill did not use integer UV ownership") &&
           expect(selected.values == std::vector<double>(selection.begin(), selection.end()) &&
                      selected.selected_texel_count == 3,
                  "selection fill did not preserve its normalized selection weights");
}

PaintToolChannelRaster channel(std::string id, std::initializer_list<ColourValue> pixels) {
    return {.semantic_id = std::move(id), .component_count = 3, .pixels = pixels};
}

bool fill_applies_material_through_masks_and_rejection() {
    const CachedSurfaceMaps maps = surface_maps();
    const std::array layer{channel("pbr.base_color", {{0, 0, 0, 1},
                                                      {0, 0, 0, 1},
                                                      {0, 0, 0, 1},
                                                      {0, 0, 0, 1},
                                                      {0, 0, 0, 1},
                                                      {0, 0, 0, 1},
                                                      {0, 0, 0, 1},
                                                      {0, 0, 0, 1}})};
    const std::array material{channel("pbr.base_color", {{1, 0, 0, 1},
                                                         {1, 0, 0, 1},
                                                         {1, 0, 0, 1},
                                                         {1, 0, 0, 1},
                                                         {1, 0, 0, 1},
                                                         {1, 0, 0, 1},
                                                         {1, 0, 0, 1},
                                                         {1, 0, 0, 1}})};
    const std::array<double, 8> mask{1, 0.5, 1, 1, 1, 1, 1, 1};
    const std::array<double, 8> rejection{1, 1, 0, 1, 1, 1, 1, 1};
    const FillResult result = apply_fill(maps, layer, material,
                                         {.scope = scope_request(FillScope::triangle, 0),
                                          .blend_mode = "normal",
                                          .masks = {.active_layer_masks = {},
                                                    .colour_id_selection = PaintMaskView{mask},
                                                    .geometry_selection = std::nullopt,
                                                    .screen_selection = std::nullopt,
                                                    .uv_island_selection = std::nullopt},
                                          .rejection_acceptance = PaintMaskView{rejection}});
    return expect(result.strength == std::vector<double>({1, 0.5, 0, 0, 0, 0, 0, 0}),
                  "fill did not intersect scope, masks and rejection") &&
           expect(result.channels[0].pixels[0].r == 1.0F &&
                      result.channels[0].pixels[1].r == 0.5F &&
                      result.channels[0].pixels[2].r == 0.0F,
                  "fill did not shade the resolved region from the active material");
}

// parameter-audit: paint.connected.maximum_angle_degrees
bool connected_angle_is_bounded_reported_and_used() {
    const CachedSurfaceMaps maps = surface_maps();
    const auto triangles = topology();
    FillScopeRequest request = scope_request(FillScope::connected_by_angle, 0);
    request.maximum_angle_degrees = 200.0;
    request.triangle_topology = triangles;
    const FillScopeResult result = resolve_fill_scope(maps, request);
    return expect(result.selected_triangle_ids == std::vector<std::uint32_t>({0, 1, 2, 3}),
                  "the resolved connected angle did not drive fill traversal") &&
           expect(result.parameter_report.clamp_for("paint.connected.maximum_angle_degrees") ==
                      ToolParameterClamp{.name = "paint.connected.maximum_angle_degrees",
                                         .supplied = 200.0,
                                         .resolved = 180.0},
                  "the connected fill angle clamp was not reported");
}

bool invalid_fill_is_refused() {
    CachedSurfaceMaps maps = surface_maps();
    bool missing_pick_refused = false;
    try {
        static_cast<void>(resolve_fill_scope(maps, scope_request(FillScope::triangle)));
    } catch (const std::invalid_argument&) {
        missing_pick_refused = true;
    }
    bool missing_topology_refused = false;
    try {
        static_cast<void>(
            resolve_fill_scope(maps, scope_request(FillScope::connected_by_angle, 0)));
    } catch (const std::invalid_argument&) {
        missing_topology_refused = true;
    }
    bool non_finite_angle_refused = false;
    try {
        FillScopeRequest request = scope_request(FillScope::connected_by_angle, 0);
        request.maximum_angle_degrees = std::numeric_limits<double>::infinity();
        request.triangle_topology = topology();
        static_cast<void>(resolve_fill_scope(maps, request));
    } catch (const std::invalid_argument&) {
        non_finite_angle_refused = true;
    }
    maps.triangle_identity[0] = no_surface_triangle;
    bool inconsistent_maps_refused = false;
    try {
        static_cast<void>(resolve_fill_scope(maps, scope_request(FillScope::whole_set)));
    } catch (const std::invalid_argument&) {
        inconsistent_maps_refused = true;
    }
    return expect(missing_pick_refused && missing_topology_refused && non_finite_angle_refused &&
                      inconsistent_maps_refused,
                  "invalid fill request or surface maps were not refused");
}

}  // namespace

int main() {
    return all_six_fill_scopes_select_their_exact_regions() &&
                   fill_applies_material_through_masks_and_rejection() &&
                   connected_angle_is_bounded_reported_and_used() && invalid_fill_is_refused()
               ? 0
               : 1;
}
