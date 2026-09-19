#include <array>
#include <cmath>
#include <ctex/paint/projection.hpp>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
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

bool near(float actual, float expected, float tolerance = 1.0e-6F) {
    return std::abs(actual - expected) <= tolerance;
}

PaintToolChannelRaster channel(std::string semantic_id, std::initializer_list<float> red_values) {
    PaintToolChannelRaster result{
        .semantic_id = std::move(semantic_id), .component_count = 3, .pixels = {}};
    for (const float red : red_values) {
        result.pixels.push_back({red, 0.0F, 0.0F, 1.0F});
    }
    return result;
}

SurfaceTexel texel(Vec3d position, Vec3d normal = {0.0, 0.0, 1.0}, std::uint32_t triangle = 0) {
    return {.position = position,
            .normal = normal,
            .geometric_normal = normal,
            .uv = {},
            .triangle = triangle};
}

CachedSurfaceMaps surface(std::vector<SurfaceTexel> texels) {
    const std::size_t count = texels.size();
    return {.texture_set_id = "set:body",
            .uv_set = "uv0",
            .mesh_revision = 1,
            .surface = {.width = static_cast<std::uint32_t>(count),
                        .height = 1,
                        .tile_origin = {},
                        .texels = std::move(texels)},
            .coverage = std::vector<std::uint8_t>(count, 1),
            .triangle_identity = std::vector<std::uint32_t>(count, 0),
            .uv_island_identity = std::vector<std::uint32_t>(count, 0)};
}

DecalMaterial material(std::vector<double> opacity = {1.0, 0.8, 0.6, 0.4}) {
    return {.width = 2,
            .height = 2,
            .channels = {channel("pbr.base_color", {0.1F, 0.2F, 0.3F, 0.4F})},
            .opacity = std::move(opacity)};
}

ctex::pick::Mat4f identity() {
    return {.values = {1.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1.0F, 0.0F, 0.0F,
                       0.0F, 0.0F, 1.0F}};
}

CachedSurfaceMaps framed_surface() {
    return surface(
        {texel({-0.5, 0.5, 0.0}, {0.0, 0.0, 1.0}, 0), texel({0.5, 0.5, 0.0}, {0.0, 0.0, 1.0}, 1),
         texel({-0.5, -0.5, 0.0}, {0.0, 0.0, 1.0}, 2), texel({2.0, 0.0, 0.0}, {0.0, 0.0, 1.0}, 3),
         texel({1.0, 1.0, 0.0}, {0.0, 0.0, 1.0}, 4)});
}

bool camera_projection_applies_only_to_visible_framed_surface() {
    const CachedSurfaceMaps maps = framed_surface();
    const std::array layer{channel("pbr.base_color", {0.0F, 0.0F, 0.0F, 0.0F, 0.0F})};
    const std::array<double, 5> visibility{1.0, 0.5, 0.0, 1.0, 1.0};
    const std::array<double, 5> selection{0.5, 0.5, 0.5, 0.5, 0.5};
    const std::array<double, 5> rejection{0.5, 0.5, 0.5, 0.5, 0.5};
    const ProjectionResult result =
        apply_projection(maps, layer, material(),
                         {.mapping = CameraProjection{.view_projection = identity(),
                                                      .visible_surface = PaintMaskView{visibility}},
                          .blend_mode = "normal",
                          .masks = {.active_layer_masks = {},
                                    .colour_id_selection = std::nullopt,
                                    .geometry_selection = std::nullopt,
                                    .screen_selection = PaintMaskView{selection},
                                    .uv_island_selection = std::nullopt},
                          .rejection_acceptance = PaintMaskView{rejection}});

    return expect(result.samples[0].source_indices[0] == 0 &&
                      result.samples[1].source_indices[0] == 1 &&
                      result.samples[2].source_indices[0] == 2 && result.samples[3].count == 0,
                  "camera projection did not resolve clip-space image samples") &&
           expect(result.samples[4].source_indices[0] == 1,
                  "camera projection excluded its positive clip boundary") &&
           expect(result.strength == std::vector<double>({0.25, 0.1, 0.0, 0.0, 0.2}),
                  "camera visibility, material opacity, masks or rejection were not composed") &&
           expect(near(result.channels[0].pixels[0].r, 0.025F) &&
                      near(result.channels[0].pixels[1].r, 0.02F) &&
                      near(result.channels[0].pixels[2].r, 0.0F),
                  "camera projection did not shade the visible surface from its snapshot");
}

// parameter-audit: projection.planar.extent.x
// parameter-audit: projection.planar.extent.y
bool planar_projection_uses_its_centered_frame_and_extent() {
    const CachedSurfaceMaps maps = framed_surface();
    const std::array layer{channel("pbr.base_color", {0.0F, 0.0F, 0.0F, 0.0F, 0.0F})};
    const ProjectionResult result = apply_projection(
        maps, layer, material(),
        {.mapping = PlanarProjection{
             .frame = {.origin = {}, .u_axis = {1.0, 0.0, 0.0}, .v_axis = {0.0, 1.0, 0.0}},
             .extent = {2.0, 2.0}}});
    const CachedSurfaceMaps axis_maps = surface({texel({0.75, 0.0, 0.0}), texel({0.0, 0.75, 0.0})});
    const std::array axis_layer{channel("pbr.base_color", {0.0F, 0.0F})};
    const ProjectionResult narrow_x = apply_projection(
        axis_maps, axis_layer, material(),
        {.mapping = PlanarProjection{
             .frame = {.origin = {}, .u_axis = {1.0, 0.0, 0.0}, .v_axis = {0.0, 1.0, 0.0}},
             .extent = {1.0, 2.0}}});
    const ProjectionResult narrow_y = apply_projection(
        axis_maps, axis_layer, material(),
        {.mapping = PlanarProjection{
             .frame = {.origin = {}, .u_axis = {1.0, 0.0, 0.0}, .v_axis = {0.0, 1.0, 0.0}},
             .extent = {2.0, 1.0}}});

    return expect(result.samples[0].source_indices[0] == 0 &&
                      result.samples[1].source_indices[0] == 1 &&
                      result.samples[2].source_indices[0] == 2 && result.samples[3].count == 0,
                  "planar projection did not map its centered finite frame") &&
           expect(result.samples[4].source_indices[0] == 1,
                  "planar projection excluded its positive frame boundary") &&
           expect(narrow_x.samples[0].count == 0 && narrow_x.samples[1].count == 1 &&
                      narrow_y.samples[0].count == 1 && narrow_y.samples[1].count == 0,
                  "planar extent axes did not independently change projected coverage") &&
           expect(result.strength == std::vector<double>({1.0, 0.8, 0.6, 0.0, 0.8}),
                  "planar projection did not preserve sampled image opacity");
}

// parameter-audit: projection.triplanar.scale
// parameter-audit: projection.triplanar.offset.x
// parameter-audit: projection.triplanar.offset.y
bool triplanar_projection_blends_normal_weighted_repeating_planes() {
    const double root_half = std::sqrt(0.5);
    const CachedSurfaceMaps maps = surface({texel({0.25, 0.75, 0.25}, {root_half, 0.5, 0.5})});
    const std::array layer{channel("pbr.base_color", {0.0F})};
    const ProjectionResult result =
        apply_projection(maps, layer, material(),
                         {.mapping = TriplanarProjection{.scale = 1.0, .offset = {0.0, 0.0}}});
    const ProjectionResult scaled =
        apply_projection(maps, layer, material({1.0, 1.0, 1.0, 1.0}),
                         {.mapping = TriplanarProjection{.scale = 2.0, .offset = {0.0, 0.0}}});
    const ProjectionResult offset =
        apply_projection(maps, layer, material({1.0, 1.0, 1.0, 1.0}),
                         {.mapping = TriplanarProjection{.scale = 1.0, .offset = {0.5, 0.5}}});
    const ProjectionResult offset_x =
        apply_projection(maps, layer, material({1.0, 1.0, 1.0, 1.0}),
                         {.mapping = TriplanarProjection{.scale = 1.0, .offset = {0.5, 0.0}}});
    const ProjectionResult offset_y =
        apply_projection(maps, layer, material({1.0, 1.0, 1.0, 1.0}),
                         {.mapping = TriplanarProjection{.scale = 1.0, .offset = {0.0, 0.5}}});
    const ProjectionSample& sample = result.samples[0];

    return expect(sample.count == 3 &&
                      sample.source_indices == std::array<std::size_t, 3>{3, 2, 0} &&
                      near(static_cast<float>(sample.weights[0]), 0.5F) &&
                      near(static_cast<float>(sample.weights[1]), 0.25F) &&
                      near(static_cast<float>(sample.weights[2]), 0.25F),
                  "triplanar projection did not resolve its three squared-normal planes") &&
           expect(near(static_cast<float>(result.strength[0]), 0.6F) &&
                      near(result.sampled_material[0].pixels[0].r, 0.25F) &&
                      near(result.channels[0].pixels[0].r, 0.15F),
                  "triplanar projection did not opacity-weight its material samples") &&
           expect(scaled.samples[0].source_indices == std::array<std::size_t, 3>{1, 1, 1} &&
                      offset.samples[0].source_indices == std::array<std::size_t, 3>{0, 1, 3} &&
                      offset_x.samples[0].source_indices != sample.source_indices &&
                      offset_y.samples[0].source_indices != sample.source_indices &&
                      offset_x.samples[0].source_indices != offset_y.samples[0].source_indices &&
                      near(scaled.sampled_material[0].pixels[0].r, 0.2F) &&
                      near(offset.sampled_material[0].pixels[0].r, 0.2F),
                  "triplanar scale or offset did not affect repeating coordinates");
}

bool invalid_projection_inputs_are_refused() {
    const CachedSurfaceMaps maps = surface({texel({})});
    const std::array layer{channel("pbr.base_color", {0.0F})};
    const std::array<double, 1> visibility{1.0};
    bool camera_refused = false;
    try {
        static_cast<void>(apply_projection(
            maps, layer, material(),
            {.mapping = CameraProjection{.view_projection = {},
                                         .visible_surface = PaintMaskView{visibility}}}));
    } catch (const std::invalid_argument&) {
        camera_refused = true;
    }
    bool planar_refused = false;
    try {
        static_cast<void>(apply_projection(
            maps, layer, material(),
            {.mapping = PlanarProjection{
                 .frame = {}, .extent = {std::numeric_limits<double>::infinity(), 1.0}}}));
    } catch (const std::invalid_argument&) {
        planar_refused = true;
    }
    bool triplanar_refused = false;
    try {
        static_cast<void>(apply_projection(
            maps, layer, material(),
            {.mapping = TriplanarProjection{.scale = std::numeric_limits<double>::quiet_NaN(),
                                            .offset = {}}}));
    } catch (const std::invalid_argument&) {
        triplanar_refused = true;
    }
    return expect(camera_refused && planar_refused && triplanar_refused,
                  "invalid camera, planar or triplanar projection was not refused");
}

bool projection_parameters_are_bounded_reported_and_used() {
    const CachedSurfaceMaps maps = surface({texel({0.25, 0.25, 0.0})});
    const std::array layer{channel("pbr.base_color", {0.0F})};
    const ProjectionResult planar = apply_projection(
        maps, layer, material(),
        {.mapping = PlanarProjection{
             .frame = {.origin = {}, .u_axis = {1.0, 0.0, 0.0}, .v_axis = {0.0, 1.0, 0.0}},
             .extent = {0.0, maximum_tool_transform_extent * 2.0}}});
    const ProjectionResult triplanar =
        apply_projection(maps, layer, material(),
                         {.mapping = TriplanarProjection{.scale = 0.0, .offset = {1.5, -1.5}}});

    return expect(planar.parameter_report.clamps.size() == 2 &&
                      planar.parameter_report.clamp_for("projection.planar.extent.x") ==
                          ToolParameterClamp{"projection.planar.extent.x", 0.0,
                                             stroke_position_tolerance} &&
                      planar.parameter_report.clamp_for("projection.planar.extent.y") ==
                          ToolParameterClamp{"projection.planar.extent.y",
                                             maximum_tool_transform_extent * 2.0,
                                             maximum_tool_transform_extent} &&
                      planar.samples[0].count == 0,
                  "planar projection bounds or clamp report are incomplete") &&
           expect(triplanar.parameter_report.clamps.size() == 3 &&
                      triplanar.parameter_report.clamp_for("projection.triplanar.scale") ==
                          ToolParameterClamp{"projection.triplanar.scale", 0.0,
                                             stroke_position_tolerance} &&
                      triplanar.parameter_report.clamp_for("projection.triplanar.offset.x") ==
                          ToolParameterClamp{"projection.triplanar.offset.x", 1.5, 1.0} &&
                      triplanar.parameter_report.clamp_for("projection.triplanar.offset.y") ==
                          ToolParameterClamp{"projection.triplanar.offset.y", -1.5, -1.0} &&
                      triplanar.samples[0].source_indices == std::array<std::size_t, 3>{2, 2, 2},
                  "triplanar projection bounds or clamp report are incomplete");
}

}  // namespace

int main() {
    return camera_projection_applies_only_to_visible_framed_surface() &&
                   planar_projection_uses_its_centered_frame_and_extent() &&
                   triplanar_projection_blends_normal_weighted_repeating_planes() &&
                   invalid_projection_inputs_are_refused() &&
                   projection_parameters_are_bounded_reported_and_used()
               ? 0
               : 1;
}
