#include <array>
#include <cmath>
#include <ctex/paint/decal_stencil.hpp>
#include <iostream>
#include <limits>
#include <numbers>
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

CachedSurfaceMaps decal_surface() {
    return {.texture_set_id = "set:body",
            .uv_set = "uv0",
            .mesh_revision = 1,
            .surface = {.width = 2,
                        .height = 1,
                        .tile_origin = {},
                        .texels = {{.position = {},
                                    .normal = {0.0, 0.0, 1.0},
                                    .geometric_normal = {0.0, 0.0, 1.0},
                                    .uv = {0.25, 0.5},
                                    .triangle = 0},
                                   {.position = {0.25, -0.25, 0.0},
                                    .normal = {0.0, 0.0, 1.0},
                                    .geometric_normal = {0.0, 0.0, 1.0},
                                    .uv = {0.75, 0.5},
                                    .triangle = 1}}},
            .coverage = {1, 1},
            .triangle_identity = {0, 1},
            .uv_island_identity = {0, 0}};
}

DecalMaterial decal_material() {
    return {.width = 2,
            .height = 1,
            .channels = {channel("pbr.base_color", {0.2F, 0.8F})},
            .opacity = {0.5, 0.5}};
}

DecalPlacement placement(const CachedSurfaceMaps& surface, double rotation = 0.0) {
    return place_decal_on_surface(
        surface, 0, {.rotation_radians = rotation, .uniform_scale = 1.0, .axis_scale = {1.0, 1.0}});
}

bool retained_decal_edits_without_repicking_and_rasterizes_explicitly() {
    const CachedSurfaceMaps surface = decal_surface();
    const std::array layer{channel("pbr.base_color", {0.0F, 0.0F})};
    const EditableDecalEntry retained = retain_editable_decal("decal:logo", "material:logo:v1",
                                                              placement(surface), decal_material());
    const EditableDecalEntry rotated =
        edit_decal_transform(retained, {.rotation_radians = std::numbers::pi / 2.0,
                                        .uniform_scale = 1.0,
                                        .axis_scale = {1.0, 1.0}});
    const std::array<double, 2> mask{0.5, 0.5};
    const std::array<double, 2> rejection{0.5, 0.5};
    const DecalRasterSettings settings{.blend_mode = "normal",
                                       .masks = {.active_layer_masks = {},
                                                 .colour_id_selection = std::nullopt,
                                                 .geometry_selection = PaintMaskView{mask},
                                                 .screen_selection = std::nullopt,
                                                 .uv_island_selection = std::nullopt},
                                       .rejection_acceptance = PaintMaskView{rejection}};
    const DecalRasterResult original = rasterize_editable_decal(surface, layer, retained, settings);
    const DecalRasterResult edited = rasterize_editable_decal(surface, layer, rotated, settings);

    return expect(retained.revision == 1 && rotated.revision == 2 &&
                      retained.placement.position == rotated.placement.position &&
                      retained.placement.surface_normal == rotated.placement.surface_normal,
                  "editing a retained decal repicked its surface placement") &&
           expect(original.source_sample_indices == std::vector<std::size_t>({1, 1}) &&
                      edited.source_sample_indices == std::vector<std::size_t>({1, 0}),
                  "decal rotation did not change projection through the stored frame") &&
           expect(original.strength == std::vector<double>({0.125, 0.125}) &&
                      edited.strength == std::vector<double>({0.125, 0.125}) &&
                      near(original.channels[0].pixels[1].r, 0.1F) &&
                      near(edited.channels[0].pixels[1].r, 0.025F),
                  "explicit decal rasterization did not compose alpha, masks and rejection");
}

Stamp stamp() {
    return {.position = {},
            .frame = {},
            .radius = 1.0,
            .opacity = 1.0,
            .hardness = 1.0,
            .rotation_radians = 0.0,
            .elongation = 1.0,
            .flow = 1.0,
            .tip_resource_identity = "builtin.circle",
            .source_ordinal = 0,
            .symmetry_instance = 0,
            .ordinal = 0};
}

ResolvedStroke stroke() {
    return {.reconstruction_version = canonical_stroke_reconstruction_version,
            .tip_mode = TipMode::discrete_alpha,
            .symmetry_instance_count = 1,
            .stamps = {stamp()},
            .swept_segments = {}};
}

RejectedCoverageRaster coverage(std::initializer_list<double> values) {
    const std::vector<double> pixels(values);
    return {.coverage = {.width = static_cast<std::uint32_t>(pixels.size()),
                         .height = 1,
                         .values = pixels},
            .stamp_events = {{.stamp_ordinal = 0, .values = pixels}},
            .report = {}};
}

bool stencil_is_screen_anchored_transformable_and_invertible() {
    const ToolOpacityImage image{.width = 2, .height = 1, .opacity = {0.0, 1.0}};
    const std::array<Vec2d, 3> positions{Vec2d{-0.25, 0.0}, Vec2d{0.25, 0.0}, Vec2d{0.75, 0.0}};
    const StencilTransform base{.position = {}, .rotation_radians = 0.0, .scale = {1.0, 1.0}};
    const StencilMaskResult ordinary = resolve_stencil_mask(3, 1, positions, image, base);
    const StencilMaskResult inverted = resolve_stencil_mask(3, 1, positions, image, base, true);
    const StencilMaskResult rotated = resolve_stencil_mask(
        3, 1, positions, image,
        {.position = {}, .rotation_radians = std::numbers::pi, .scale = {1.0, 1.0}});
    const StencilMaskResult scaled = resolve_stencil_mask(
        3, 1, positions, image, {.position = {}, .rotation_radians = 0.0, .scale = {2.0, 1.0}});
    const StencilMaskResult translated = resolve_stencil_mask(
        3, 1, positions, image,
        {.position = {0.5, 0.0}, .rotation_radians = 0.0, .scale = {1.0, 1.0}});
    return expect(ordinary.values == std::vector<double>({0, 1, 0}) &&
                      inverted.values == std::vector<double>({1, 0, 1}),
                  "stencil opacity or inversion did not stay in screen space") &&
           expect(rotated.values == std::vector<double>({1, 0, 0}) &&
                      scaled.values == std::vector<double>({0, 1, 1}) &&
                      translated.values == std::vector<double>({0, 0, 1}),
                  "stencil position, rotation or scale did not affect its screen mask");
}

bool stencil_constrains_canonical_paint_strength() {
    const std::array<Vec2d, 2> positions{Vec2d{-0.25, 0.0}, Vec2d{0.25, 0.0}};
    const std::array layer{channel("pbr.base_color", {0.0F, 0.0F})};
    const std::array material{channel("pbr.base_color", {1.0F, 1.0F})};
    const std::array<double, 2> selection{1.0, 0.5};
    const StencilResult result =
        apply_stencil(stroke(), coverage({1, 1}), positions, layer, material,
                      {.transform = {.position = {}, .rotation_radians = 0.0, .scale = {1.0, 1.0}},
                       .image = {.width = 2, .height = 1, .opacity = {0.0, 1.0}},
                       .inverted = false,
                       .deposition_mode = DepositionMode::non_building,
                       .blend_mode = "normal",
                       .masks = {.active_layer_masks = {},
                                 .colour_id_selection = std::nullopt,
                                 .geometry_selection = std::nullopt,
                                 .screen_selection = PaintMaskView{selection},
                                 .uv_island_selection = std::nullopt}});
    return expect(result.deposition.strength == std::vector<double>({0.0, 0.5}) &&
                      near(result.channels[0].pixels[0].r, 0.0F) &&
                      near(result.channels[0].pixels[1].r, 0.5F),
                  "stencil did not constrain the canonical masked paint path");
}

bool invalid_decal_and_stencil_inputs_are_refused() {
    const CachedSurfaceMaps surface = decal_surface();
    bool identity_refused = false;
    try {
        static_cast<void>(
            retain_editable_decal("", "material", placement(surface), decal_material()));
    } catch (const std::invalid_argument&) {
        identity_refused = true;
    }
    bool non_finite_transform_refused = false;
    try {
        static_cast<void>(resolve_decal_frame(
            {.position = {},
             .surface_normal = {0.0, 0.0, 1.0},
             .transform = {.rotation_radians = 0.0,
                           .uniform_scale = std::numeric_limits<double>::infinity(),
                           .axis_scale = {1.0, 1.0}},
             .parameter_report = {}}));
    } catch (const std::invalid_argument&) {
        non_finite_transform_refused = true;
    }
    bool opacity_refused = false;
    try {
        const std::array<Vec2d, 1> position{Vec2d{}};
        static_cast<void>(
            resolve_stencil_mask(1, 1, position, {.width = 1, .height = 1, .opacity = {1.1}}, {}));
    } catch (const std::invalid_argument&) {
        opacity_refused = true;
    }
    return expect(identity_refused && non_finite_transform_refused && opacity_refused,
                  "invalid decal or stencil input was not refused");
}

bool decal_and_stencil_parameters_are_bounded_and_reported() {
    const CachedSurfaceMaps surface = decal_surface();
    const DecalPlacement placed =
        place_decal_on_surface(surface, 0,
                               {.rotation_radians = 10.0,
                                .uniform_scale = 0.0,
                                .axis_scale = {maximum_tool_transform_extent * 2.0, -1.0}});
    const DecalFrame frame = resolve_decal_frame(placed);
    const std::array layer{channel("pbr.base_color", {0.0F, 0.0F})};
    const DecalRasterResult raster = rasterize_decal(surface, layer, placed, decal_material());
    const EditableDecalEntry edited = edit_decal_transform(
        retain_editable_decal("decal", "material", placement(surface), decal_material()),
        {.rotation_radians = -10.0, .uniform_scale = 0.0, .axis_scale = {1.0, 1.0}});

    const std::array<Vec2d, 1> positions{Vec2d{}};
    const StencilMaskResult stencil = resolve_stencil_mask(
        1, 1, positions, {.width = 1, .height = 1, .opacity = {1.0}},
        {.position = {maximum_tool_transform_extent * 2.0, -maximum_tool_transform_extent * 2.0},
         .rotation_radians = 10.0,
         .scale = {0.0, maximum_tool_transform_extent * 2.0}});

    return expect(placed.transform == frame.resolved_transform &&
                      frame.parameter_report.clamps.size() == 4 &&
                      frame.parameter_report.clamp_for("decal.rotation_radians") ==
                          ToolParameterClamp{"decal.rotation_radians", 10.0,
                                             maximum_stroke_rotation_radians} &&
                      frame.parameter_report.clamp_for("decal.uniform_scale") &&
                      frame.parameter_report.clamp_for("decal.axis_scale.x") &&
                      frame.parameter_report.clamp_for("decal.axis_scale.y") &&
                      raster.parameter_report == frame.parameter_report,
                  "decal transform bounds or clamp report are incomplete") &&
           expect(edited.placement.transform.rotation_radians == -maximum_stroke_rotation_radians &&
                      edited.placement.transform.uniform_scale == stroke_position_tolerance &&
                      edited.placement.parameter_report.clamps.size() == 2,
                  "editable decal bypassed shared transform resolution") &&
           expect(
               stencil.resolved_transform.position ==
                       Vec2d{maximum_tool_transform_extent, -maximum_tool_transform_extent} &&
                   stencil.resolved_transform.rotation_radians == maximum_stroke_rotation_radians &&
                   stencil.resolved_transform.scale ==
                       Vec2d{stroke_position_tolerance, maximum_tool_transform_extent} &&
                   stencil.parameter_report.clamps.size() == 5 &&
                   stencil.parameter_report.clamp_for("stencil.position.x") &&
                   stencil.parameter_report.clamp_for("stencil.position.y") &&
                   stencil.parameter_report.clamp_for("stencil.rotation_radians") &&
                   stencil.parameter_report.clamp_for("stencil.scale.x") &&
                   stencil.parameter_report.clamp_for("stencil.scale.y"),
               "stencil transform bounds or clamp report are incomplete");
}

}  // namespace

int main() {
    return retained_decal_edits_without_repicking_and_rasterizes_explicitly() &&
                   stencil_is_screen_anchored_transformable_and_invertible() &&
                   stencil_constrains_canonical_paint_strength() &&
                   invalid_decal_and_stencil_inputs_are_refused() &&
                   decal_and_stencil_parameters_are_bounded_and_reported()
               ? 0
               : 1;
}
