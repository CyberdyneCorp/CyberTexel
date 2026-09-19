#include <array>
#include <cmath>
#include <ctex/paint/seam_filter.hpp>
#include <ctex/paint/surface_cache.hpp>
#include <iostream>
#include <stdexcept>
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

bool surface_adjacency_drives_filters_and_derivatives() {
    const std::array<double, 4> values{2.0, 100.0, 6.0, 10.0};
    const SurfaceAdjacencyLink seam{
        .first_texel = 0, .first_frame = {}, .second_texel = 2, .second_frame = {}};
    const std::array blur_samples{
        SurfaceAdjacentSample{
            .texel_index = 0, .tangent_frame = {}, .offset_x = 0, .offset_y = 0, .weight = 1.0},
        sample_across_surface_adjacency(seam, 0, 1, 0, 1.0),
    };
    const std::array derivative_samples{
        SurfaceAdjacentSample{
            .texel_index = 0, .tangent_frame = {}, .offset_x = -2, .offset_y = 0, .weight = -1.0},
        SurfaceAdjacentSample{
            .texel_index = 3, .tangent_frame = {}, .offset_x = 2, .offset_y = 0, .weight = 1.0},
    };
    const double blurred =
        filter_surface_scalar(values, {.operation = SurfaceFilterOperation::blur,
                                       .footprint = {.radius_x = 1, .radius_y = 1},
                                       .output_frame = {},
                                       .samples = blur_samples});
    const double derivative =
        filter_surface_scalar(values, {.operation = SurfaceFilterOperation::derivative,
                                       .footprint = {.radius_x = 2, .radius_y = 0},
                                       .output_frame = {},
                                       .samples = derivative_samples});
    return expect(near(blurred, 4.0),
                  "surface-adjacent blur followed UV storage instead of supplied adjacency") &&
           expect(near(derivative, 8.0),
                  "surface-adjacent derivative did not retain signed weights");
}

bool mirrored_tangent_frame_survives_minification() {
    const StrokeFrame regular{};
    const StrokeFrame mirrored{
        .tangent = {1.0, 0.0, 0.0}, .bitangent = {0.0, -1.0, 0.0}, .normal = {0.0, 0.0, 1.0}};
    const SurfaceAdjacencyLink seam{
        .first_texel = 0, .first_frame = regular, .second_texel = 1, .second_frame = mirrored};
    const std::array<Vec3d, 2> normals{Vec3d{0.0, 0.6, 0.8}, Vec3d{0.0, -0.6, 0.8}};
    const std::array samples{
        SurfaceAdjacentSample{.texel_index = 0,
                              .tangent_frame = regular,
                              .offset_x = 0,
                              .offset_y = 0,
                              .weight = 0.5},
        sample_across_surface_adjacency(seam, 0, 1, 0, 0.5),
    };
    const Vec3d filtered =
        filter_surface_tangent_vector(normals, {.operation = SurfaceFilterOperation::mip_generation,
                                                .footprint = {.radius_x = 1, .radius_y = 1},
                                                .output_frame = regular,
                                                .samples = samples});
    return expect(near(filtered.x, 0.0) && near(filtered.y, 0.6) && near(filtered.z, 0.8),
                  "mirrored tangent frame flipped the normal during minification") &&
           expect(near(std::sqrt(filtered.x * filtered.x + filtered.y * filtered.y +
                                 filtered.z * filtered.z),
                       1.0),
                  "filtered tangent-space normal was not renormalized");
}

bool insufficient_gutter_is_reported_without_overwriting_islands() {
    const std::array<std::uint32_t, 7> islands{no_uv_island, 10, no_uv_island, no_uv_island,
                                               no_uv_island, 20, no_uv_island};
    const IslandPaddingPlan plan =
        plan_island_padding(7, 1, islands, {.radius_x = 1, .radius_y = 0}, 2);
    const SeamDilationRaster source{.width = 7,
                                    .height = 1,
                                    .component_count = 1,
                                    .pixels = {-1.0, 10.0, -1.0, -1.0, -1.0, 20.0, -1.0}};
    const IslandPaddingResult padded = apply_island_padding(source, islands, plan);

    return expect(plan.padding_radius == 3 && plan.unsupported_mip_levels.size() == 1 &&
                      plan.unsupported_mip_levels[0] ==
                          UnsupportedMipLevel{.mip_level = 1,
                                              .required_gutter_radius = 3,
                                              .affected_islands = {10, 20}},
                  "insufficient gutter did not name the affected islands and mip level") &&
           expect(padded.raster.pixels[1] == 10.0 && padded.raster.pixels[5] == 20.0,
                  "padding overwrote valid neighboring island texels") &&
           expect(plan.ownership[3] == no_uv_island && padded.raster.pixels[3] == -1.0,
                  "contested gutter was assigned to one neighboring island") &&
           expect(padded.padded_texel_count == 4,
                  "padding did not fill every unambiguous island-owned gutter texel");
}

bool sufficient_gutter_prevents_minification_bleed() {
    const std::array<std::uint32_t, 7> islands{no_uv_island, no_uv_island, 4, 4, 4,
                                               no_uv_island, no_uv_island};
    const IslandPaddingPlan plan = plan_island_padding(7, 1, islands, {}, 2);
    const SeamDilationRaster source{.width = 7,
                                    .height = 1,
                                    .component_count = 1,
                                    .pixels = {-1.0, -1.0, 0.75, 0.75, 0.75, -1.0, -1.0}};
    const IslandPaddingResult padded = apply_island_padding(source, islands, plan);
    const std::array samples{
        SurfaceAdjacentSample{
            .texel_index = 1, .tangent_frame = {}, .offset_x = -1, .offset_y = 0, .weight = 1.0},
        SurfaceAdjacentSample{
            .texel_index = 2, .tangent_frame = {}, .offset_x = 0, .offset_y = 0, .weight = 1.0},
        SurfaceAdjacentSample{
            .texel_index = 4, .tangent_frame = {}, .offset_x = 0, .offset_y = 0, .weight = 1.0},
        SurfaceAdjacentSample{
            .texel_index = 5, .tangent_frame = {}, .offset_x = 1, .offset_y = 0, .weight = 1.0},
    };
    const double minified = filter_surface_scalar(
        padded.raster.pixels, {.operation = SurfaceFilterOperation::mip_generation,
                               .footprint = {.radius_x = 1, .radius_y = 0},
                               .output_frame = {},
                               .samples = samples});
    return expect(plan.unsupported_mip_levels.empty() && padded.padded_texel_count == 2,
                  "sufficient gutter was not supported and padded for its declared mip range") &&
           expect(near(minified, 0.75),
                  "declared supported minification sampled background through the gutter");
}

bool invalid_filter_and_padding_requests_are_refused() {
    bool empty_samples_refused = false;
    try {
        const std::array<double, 1> values{1.0};
        static_cast<void>(
            filter_surface_scalar(values, {.operation = SurfaceFilterOperation::smear,
                                           .footprint = {.radius_x = 1, .radius_y = 1},
                                           .output_frame = {},
                                           .samples = {}}));
    } catch (const std::invalid_argument&) {
        empty_samples_refused = true;
    }
    bool zero_mips_refused = false;
    try {
        const std::array<std::uint32_t, 1> islands{0};
        static_cast<void>(plan_island_padding(1, 1, islands, {}, 0));
    } catch (const std::invalid_argument&) {
        zero_mips_refused = true;
    }
    bool footprint_refused = false;
    try {
        const std::array<double, 1> values{1.0};
        const std::array samples{SurfaceAdjacentSample{
            .texel_index = 0, .tangent_frame = {}, .offset_x = 2, .offset_y = 0, .weight = 1.0}};
        static_cast<void>(
            filter_surface_scalar(values, {.operation = SurfaceFilterOperation::blur,
                                           .footprint = {.radius_x = 1, .radius_y = 1},
                                           .output_frame = {},
                                           .samples = samples}));
    } catch (const std::invalid_argument&) {
        footprint_refused = true;
    }
    bool changed_owner_refused = false;
    try {
        const std::array<std::uint32_t, 1> islands{3};
        IslandPaddingPlan plan = plan_island_padding(1, 1, islands, {}, 1);
        plan.ownership[0] = 4;
        const SeamDilationRaster source{
            .width = 1, .height = 1, .component_count = 1, .pixels = {0.5}};
        static_cast<void>(apply_island_padding(source, islands, plan));
    } catch (const std::invalid_argument&) {
        changed_owner_refused = true;
    }
    return expect(
        empty_samples_refused && zero_mips_refused && footprint_refused && changed_owner_refused,
        "invalid seam-filter request was not refused by name");
}

}  // namespace

int main() {
    return surface_adjacency_drives_filters_and_derivatives() &&
                   mirrored_tangent_frame_survives_minification() &&
                   insufficient_gutter_is_reported_without_overwriting_islands() &&
                   sufficient_gutter_prevents_minification_bleed() &&
                   invalid_filter_and_padding_requests_are_refused()
               ? 0
               : 1;
}
