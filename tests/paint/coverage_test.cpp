#include <array>
#include <cmath>
#include <ctex/paint/coverage.hpp>
#include <iostream>
#include <limits>
#include <string_view>

namespace {

using namespace ctex::paint;

bool expect(bool condition, std::string_view message) {
    if (!condition) {
        std::cerr << message << '\n';
    }
    return condition;
}

bool near(double actual, double expected, double tolerance = 1.0e-9) {
    return std::abs(actual - expected) <= tolerance;
}

Stamp stamp(Vec3d position, std::uint64_t ordinal) {
    return {.position = position,
            .frame = {},
            .radius = 1.0,
            .opacity = 1.0,
            .hardness = 1.0,
            .rotation_radians = 0.0,
            .elongation = 1.0,
            .flow = 1.0,
            .tip_resource_identity = "builtin.circle",
            .source_ordinal = ordinal,
            .symmetry_instance = 0,
            .ordinal = ordinal};
}

TextureSpaceRaster surface(std::initializer_list<Vec3d> positions) {
    TextureSpaceRaster result{.width = static_cast<std::uint32_t>(positions.size()),
                              .height = 1,
                              .tile_origin = {},
                              .texels = {}};
    for (const Vec3d position : positions) {
        result.texels.push_back({.position = position,
                                 .normal = {0.0, 0.0, 1.0},
                                 .geometric_normal = {0.0, 0.0, 1.0},
                                 .uv = {},
                                 .triangle = 0});
    }
    return result;
}

bool uv_rasterization_is_independent_of_screen_visibility() {
    const std::array<Vec3d, 3> positions{Vec3d{100.0, 100.0, 100.0}, Vec3d{104.0, 100.0, 100.0},
                                         Vec3d{100.0, 104.0, 100.0}};
    const std::array<Vec3d, 3> normals{Vec3d{0.0, 0.0, 1.0}, Vec3d{0.0, 0.0, 1.0},
                                       Vec3d{0.0, 0.0, 1.0}};
    const std::array<Vec2d, 3> uv{Vec2d{0.0, 0.0}, Vec2d{1.0, 0.0}, Vec2d{0.0, 1.0}};
    const std::array<std::uint32_t, 3> indices{0, 1, 2};
    const auto raster =
        rasterize_texture_space({positions, normals, uv, indices}, {.width = 4, .height = 4});
    const std::size_t texel = 2 * raster.width + 1;
    return expect(raster.covered(texel) && raster.texels[texel].triangle == 0,
                  "UV triangle was not rasterized independently of a camera") &&
           expect(raster.texels[texel].position.x > 100.0 &&
                      near(raster.texels[texel].uv.x, 0.375) &&
                      near(raster.texels[texel].uv.y, 0.375) &&
                      near(raster.texels[texel].geometric_normal.z, 1.0),
                  "texture-space raster did not interpolate surface geometry");
}

bool continuous_sweeps_fill_gaps_but_discrete_tips_do_not() {
    const TextureSpaceRaster samples =
        surface({Vec3d{0.0, 0.0, 0.0}, Vec3d{5.0, 0.0, 0.0}, Vec3d{10.0, 0.0, 0.0}});
    ResolvedStroke continuous{.reconstruction_version = canonical_stroke_reconstruction_version,
                              .tip_mode = TipMode::continuous_sweep,
                              .symmetry_instance_count = 1,
                              .stamps = {stamp({0.0, 0.0, 0.0}, 0), stamp({10.0, 0.0, 0.0}, 1)},
                              .swept_segments = {{0, 1}}};
    ResolvedStroke discrete = continuous;
    discrete.tip_mode = TipMode::discrete_alpha;
    discrete.swept_segments.clear();
    const auto continuous_coverage = evaluate_stroke_coverage(samples, continuous);
    const auto discrete_coverage = evaluate_stroke_coverage(samples, discrete);
    return expect(continuous_coverage.values == std::vector<double>({1.0, 1.0, 1.0}),
                  "continuous swept capsule left a gap between stamps") &&
           expect(discrete_coverage.values == std::vector<double>({1.0, 0.0, 1.0}),
                  "discrete-alpha tips implicitly filled the gap between stamps");
}

bool falloff_matches_the_specified_smoothstep_complement() {
    return expect(near(brush_falloff(0.25, 0.5), 1.0),
                  "falloff changed inside the hardness core") &&
           expect(near(brush_falloff(0.75, 0.5), 0.5),
                  "falloff did not use the specified smoothstep complement") &&
           expect(near(brush_falloff(1.0, 0.5), 0.0) && near(brush_falloff(0.999, 1.0), 1.0) &&
                      near(brush_falloff(1.001, 1.0), 0.0),
                  "hardness-one coverage did not produce a hard edge");
}

bool discrete_tips_apply_rotation_and_elongation() {
    ResolvedStroke stroke{.reconstruction_version = canonical_stroke_reconstruction_version,
                          .tip_mode = TipMode::discrete_alpha,
                          .symmetry_instance_count = 1,
                          .stamps = {stamp({0.0, 0.0, 0.0}, 0)},
                          .swept_segments = {}};
    stroke.stamps[0].elongation = 2.0;
    auto samples = surface({Vec3d{1.5, 0.0, 0.0}, Vec3d{0.0, 1.5, 0.0}});
    const auto unrotated = evaluate_stroke_coverage(samples, stroke);
    stroke.stamps[0].rotation_radians = std::acos(-1.0) * 0.5;
    const auto rotated = evaluate_stroke_coverage(samples, stroke);
    return expect(unrotated.values == std::vector<double>({1.0, 0.0}),
                  "discrete tip elongation was not applied in its local frame") &&
           expect(rotated.values == std::vector<double>({0.0, 1.0}),
                  "discrete tip rotation did not rotate its elongated axis");
}

bool coordinate_modes_expose_uv_triplanar_and_caller_planar_frames() {
    const SurfaceTexel texel{.position = {2.0, 3.0, 5.0},
                             .normal = {1.0, 2.0, 2.0},
                             .geometric_normal = {0.0, 0.0, 1.0},
                             .uv = {0.25, 0.75},
                             .triangle = 4};
    const auto uv = material_coordinates(texel, {.mode = MaterialCoordinateMode::uv, .planar = {}});
    const auto triplanar =
        material_coordinates(texel, {.mode = MaterialCoordinateMode::triplanar, .planar = {}});
    const auto planar = material_coordinates(texel, {.mode = MaterialCoordinateMode::planar,
                                                     .planar = {.origin = {1.0, 1.0, 1.0},
                                                                .u_axis = {0.0, 1.0, 0.0},
                                                                .v_axis = {0.0, 0.0, 1.0}}});
    return expect(uv.count == 1 && uv.projections[0].coordinate == texel.uv &&
                      near(uv.projections[0].weight, 1.0),
                  "UV material coordinates changed the surface UV") &&
           expect(triplanar.count == 3 && triplanar.projections[0].coordinate == Vec2d{3.0, 5.0} &&
                      triplanar.projections[1].coordinate == Vec2d{2.0, 5.0} &&
                      triplanar.projections[2].coordinate == Vec2d{2.0, 3.0} &&
                      near(triplanar.projections[0].weight, 1.0 / 9.0) &&
                      near(triplanar.projections[1].weight, 4.0 / 9.0) &&
                      near(triplanar.projections[2].weight, 4.0 / 9.0),
                  "triplanar projections did not use squared normal weights") &&
           expect(planar.count == 1 && planar.projections[0].coordinate == Vec2d{2.0, 4.0},
                  "planar projection did not use the caller-supplied frame");
}

bool invalid_geometry_and_projection_inputs_are_refused() {
    const std::array<Vec3d, 3> positions{};
    const std::array<Vec3d, 3> normals{};
    const std::array<Vec2d, 3> uv{};
    const std::array<std::uint32_t, 3> indices{0, 1, 2};
    bool mesh_refused = false;
    try {
        static_cast<void>(
            rasterize_texture_space({positions, normals, uv, indices}, {.width = 4, .height = 4}));
    } catch (const std::invalid_argument&) {
        mesh_refused = true;
    }
    bool frame_refused = false;
    try {
        static_cast<void>(material_coordinates(
            {.position = {},
             .normal = {0.0, 0.0, 1.0},
             .geometric_normal = {0.0, 0.0, 1.0},
             .uv = {},
             .triangle = 0},
            {.mode = MaterialCoordinateMode::planar,
             .planar = {.origin = {}, .u_axis = {1.0, 0.0, 0.0}, .v_axis = {1.0, 0.0, 0.0}}}));
    } catch (const std::invalid_argument&) {
        frame_refused = true;
    }
    bool falloff_refused = false;
    try {
        static_cast<void>(brush_falloff(std::numeric_limits<double>::quiet_NaN(), 0.5));
    } catch (const std::invalid_argument&) {
        falloff_refused = true;
    }
    return expect(mesh_refused && frame_refused && falloff_refused,
                  "invalid paint coverage input was not refused");
}

}  // namespace

int main() {
    return uv_rasterization_is_independent_of_screen_visibility() &&
                   continuous_sweeps_fill_gaps_but_discrete_tips_do_not() &&
                   falloff_matches_the_specified_smoothstep_complement() &&
                   discrete_tips_apply_rotation_and_elongation() &&
                   coordinate_modes_expose_uv_triplanar_and_caller_planar_frames() &&
                   invalid_geometry_and_projection_inputs_are_refused()
               ? 0
               : 1;
}
