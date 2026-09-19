#include <algorithm>
#include <array>
#include <cmath>
#include <ctex/paint/clone.hpp>
#include <iostream>
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

bool near(float actual, float expected) { return std::abs(actual - expected) <= 1.0e-6F; }

CachedSurfaceMaps surface_maps(std::string texture_set_id = "set:body") {
    std::vector<SurfaceTexel> texels;
    for (std::uint32_t x = 0; x < 4; ++x) {
        texels.push_back({.position = {static_cast<double>(x), 0.0, 0.0},
                          .normal = {0.0, 0.0, 1.0},
                          .geometric_normal = {0.0, 0.0, 1.0},
                          .uv = {(static_cast<double>(x) + 0.5) / 4.0, 0.5},
                          .triangle = x});
    }
    return {.texture_set_id = std::move(texture_set_id),
            .uv_set = "uv0",
            .mesh_revision = 1,
            .surface = {.width = 4, .height = 1, .tile_origin = {}, .texels = std::move(texels)},
            .coverage = {1, 1, 1, 1},
            .triangle_identity = {0, 1, 2, 3},
            .uv_island_identity = {0, 0, 0, 0}};
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

RejectedCoverageRaster coverage(std::array<double, 4> values) {
    const std::vector<double> pixels(values.begin(), values.end());
    return {.coverage = {.width = 4, .height = 1, .values = pixels},
            .stamp_events = {{.stamp_ordinal = 0, .values = pixels}},
            .report = {}};
}

PaintToolChannelRaster channel(std::string id, std::initializer_list<float> red) {
    PaintToolChannelRaster result{.semantic_id = std::move(id), .component_count = 1, .pixels = {}};
    for (const float value : red) {
        result.pixels.push_back({value, value, value, 1.0F});
    }
    return result;
}

CloneSettings settings(CloneMode mode) {
    return {.mode = mode,
            .destination_anchor_uv = {0.375, 0.5},
            .deposition_mode = DepositionMode::non_building,
            .blend_mode = "normal",
            .masks = {}};
}

bool aligned_clone_uses_the_stroke_start_offset_and_masks() {
    const CachedSurfaceMaps maps = surface_maps();
    CloneSourceState source_state;
    source_state.set_source("set:body", {0.125, 0.5});
    const std::array layer{channel("pbr.base_color", {0.0F, 0.0F, 0.0F, 0.0F})};
    const std::array source{channel("pbr.base_color", {0.1F, 0.2F, 0.3F, 0.4F})};
    const std::array<double, 4> selection{1.0, 1.0, 0.5, 1.0};
    CloneSettings clone_settings = settings(CloneMode::aligned);
    clone_settings.masks.screen_selection = PaintMaskView{selection};
    const CloneResult result = apply_clone(maps, stroke(), coverage({1.0, 1.0, 1.0, 0.0}),
                                           source_state, layer, source, clone_settings);

    return expect(
               result.source_sample_indices == std::vector<std::size_t>({no_clone_sample, 0, 1, 2}),
               "aligned clone did not retain one stroke-start UV offset") &&
           expect(result.deposition.strength == std::vector<double>({0.0, 1.0, 0.5, 0.0}),
                  "clone did not inherit rejection and paint masks") &&
           expect(near(result.channels[0].pixels[0].r, 0.0F) &&
                      near(result.channels[0].pixels[1].r, 0.1F) &&
                      near(result.channels[0].pixels[2].r, 0.1F) &&
                      near(result.channels[0].pixels[3].r, 0.0F),
                  "aligned clone did not sample its immutable source snapshot");
}

bool fixed_clone_reuses_the_anchored_source() {
    const CachedSurfaceMaps maps = surface_maps();
    CloneSourceState source_state;
    source_state.set_source("set:body", {0.625, 0.5});
    const std::array layer{channel("pbr.roughness", {0.0F, 0.0F, 0.0F, 0.0F})};
    const std::array source{channel("pbr.roughness", {0.1F, 0.2F, 0.3F, 0.4F})};
    const CloneResult result = apply_clone(maps, stroke(), coverage({1.0, 1.0, 1.0, 1.0}),
                                           source_state, layer, source, settings(CloneMode::fixed));
    return expect(result.source_sample_indices == std::vector<std::size_t>({2, 2, 2, 2}),
                  "fixed clone source moved away from its anchor") &&
           expect(std::all_of(result.channels[0].pixels.begin(), result.channels[0].pixels.end(),
                              [](ColourValue pixel) { return near(pixel.r, 0.3F); }),
                  "fixed clone did not copy the anchored source texel");
}

bool unset_and_cross_set_sources_are_refused() {
    const CachedSurfaceMaps maps = surface_maps("set:destination");
    const std::array layer{channel("pbr.base_color", {0.0F, 0.0F, 0.0F, 0.0F})};
    CloneSourceState source_state;
    bool unset_refused = false;
    try {
        static_cast<void>(apply_clone(maps, stroke(), coverage({1.0, 1.0, 1.0, 1.0}), source_state,
                                      layer, layer));
    } catch (const std::invalid_argument&) {
        unset_refused = true;
    }
    source_state.set_source("set:source", {0.125, 0.5});
    std::string diagnostic;
    try {
        static_cast<void>(apply_clone(maps, stroke(), coverage({1.0, 1.0, 1.0, 1.0}), source_state,
                                      layer, layer));
    } catch (const std::invalid_argument& error) {
        diagnostic = error.what();
    }
    return expect(unset_refused && diagnostic.find("set:source") != std::string::npos &&
                      diagnostic.find("set:destination") != std::string::npos,
                  "clone did not refuse an unset or cross-set source with both set names");
}

}  // namespace

int main() {
    return aligned_clone_uses_the_stroke_start_offset_and_masks() &&
                   fixed_clone_reuses_the_anchored_source() &&
                   unset_and_cross_set_sources_are_refused()
               ? 0
               : 1;
}
