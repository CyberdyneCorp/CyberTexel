#ifndef CTEX_TESTS_FIXTURES_EXECUTOR_PARITY_CORPUS_HPP
#define CTEX_TESTS_FIXTURES_EXECUTOR_PARITY_CORPUS_HPP

#include <algorithm>
#include <array>
#include <cmath>
#include <ctex/exec/cpu_reference.hpp>
#include <ctex/exec/parity_gate.hpp>
#include <ctex/graph/portable_nodes.hpp>
#include <stdexcept>
#include <string_view>
#include <vector>

namespace ctex::test::executor_parity {

struct StrokeFixture {
    exec::CpuVec2f centre;
    float radius;
    float opacity;
};

struct MaterialFixture {
    graph::ColourValue base;
    graph::ColourValue layer;
    std::string_view blend_mode;
};

struct CorpusCase {
    exec::ParityFixture descriptor;
    std::array<exec::CpuVec3f, 3> positions;
    std::array<exec::CpuVec2f, 3> uv;
    std::array<std::uint32_t, 3> indices;
    exec::CpuRasterCamera camera;
    exec::CpuUvRasterRequest document;
    StrokeFixture stroke;
    MaterialFixture material;
};

inline constexpr exec::CpuMat4f identity_camera{{1.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1.0F, 0.0F, 0.0F,
                                                 0.0F, 0.0F, 1.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1.0F}};

inline constexpr exec::CpuMat4f perspective_fixture_camera{{1.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1.0F,
                                                            0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.5F,
                                                            0.0F, 0.0F, 0.0F, 2.0F}};

inline std::vector<CorpusCase> committed_corpus() {
    const std::array<exec::CpuVec3f, 3> positions{
        {{-1.0F, -1.0F, 0.0F}, {1.0F, -1.0F, 0.0F}, {-1.0F, 1.0F, 0.5F}}};
    const std::array<std::uint32_t, 3> indices{{0, 1, 2}};
    return {
        {.descriptor = {.identifier = "front-brush-u8",
                        .document = "document:v1;size=4x4;channel=base_color:rgba8",
                        .stroke = "stroke:v1;centre=0.35,0.35;radius=0.45;opacity=0.8",
                        .camera = "camera:v1;clip=opengl;matrix=identity;viewport=8x8",
                        .material = "material:v1;blend=normal;base=0.1,0.2,0.3,1",
                        .channels = {{.semantic = "base_color",
                                      .value_class = exec::ParityValueClass::unorm8,
                                      .filtered = false,
                                      .component_count = 4},
                                     {.semantic = "depth",
                                      .value_class = exec::ParityValueClass::floating_point,
                                      .filtered = false,
                                      .component_count = 1},
                                     {.semantic = "coverage",
                                      .value_class = exec::ParityValueClass::unorm8,
                                      .filtered = false,
                                      .component_count = 1}}},
         .positions = positions,
         .uv = {{{0.0F, 0.0F}, {1.0F, 0.0F}, {0.0F, 1.0F}}},
         .indices = indices,
         .camera = {.view_projection = identity_camera, .width = 8, .height = 8},
         .document = {.width = 4, .height = 4},
         .stroke = {.centre = {0.35F, 0.35F}, .radius = 0.45F, .opacity = 0.8F},
         .material = {.base = {0.1F, 0.2F, 0.3F, 1.0F},
                      .layer = {0.9F, 0.4F, 0.2F, 0.75F},
                      .blend_mode = "normal"}},
        {.descriptor = {.identifier = "filtered-layer-u16",
                        .document = "document:v1;size=5x3;channel=base_color:rgba16",
                        .stroke = "stroke:v1;centre=0.6,0.4;radius=0.5;opacity=0.65",
                        .camera = "camera:v1;clip=opengl;matrix=identity;viewport=10x6",
                        .material = "material:v1;blend=multiply;filtered=true",
                        .channels = {{.semantic = "base_color",
                                      .value_class = exec::ParityValueClass::unorm16,
                                      .filtered = true,
                                      .component_count = 4},
                                     {.semantic = "depth",
                                      .value_class = exec::ParityValueClass::floating_point,
                                      .filtered = false,
                                      .component_count = 1},
                                     {.semantic = "coverage",
                                      .value_class = exec::ParityValueClass::unorm8,
                                      .filtered = false,
                                      .component_count = 1}}},
         .positions = positions,
         .uv = {{{1.0F, 0.0F}, {2.0F, 0.0F}, {1.0F, 1.0F}}},
         .indices = indices,
         .camera = {.view_projection = identity_camera, .width = 10, .height = 6},
         .document = {.width = 5, .height = 3, .tile_origin = {1.0F, 0.0F}},
         .stroke = {.centre = {0.6F, 0.4F}, .radius = 0.5F, .opacity = 0.65F},
         .material = {.base = {0.8F, 0.5F, 0.25F, 1.0F},
                      .layer = {0.25F, 0.9F, 0.6F, 0.5F},
                      .blend_mode = "multiply"}},
        {.descriptor = {.identifier = "perspective-float",
                        .document = "document:v1;size=4x4;channel=base_color:rgba32f",
                        .stroke = "stroke:v1;centre=0.45,0.5;radius=0.55;opacity=1",
                        .camera = "camera:v1;clip=opengl;matrix=varying-w;viewport=8x8",
                        .material = "material:v1;blend=screen;filtered=true",
                        .channels = {{.semantic = "base_color",
                                      .value_class = exec::ParityValueClass::floating_point,
                                      .filtered = true,
                                      .component_count = 4},
                                     {.semantic = "depth",
                                      .value_class = exec::ParityValueClass::floating_point,
                                      .filtered = false,
                                      .component_count = 1},
                                     {.semantic = "coverage",
                                      .value_class = exec::ParityValueClass::unorm8,
                                      .filtered = false,
                                      .component_count = 1}}},
         .positions = positions,
         .uv = {{{0.0F, 0.0F}, {1.0F, 0.0F}, {0.0F, 1.0F}}},
         .indices = indices,
         .camera = {.view_projection = perspective_fixture_camera, .width = 8, .height = 8},
         .document = {.width = 4, .height = 4},
         .stroke = {.centre = {0.45F, 0.5F}, .radius = 0.55F, .opacity = 1.0F},
         .material = {.base = {0.2F, 0.4F, 0.7F, 1.0F},
                      .layer = {0.7F, 0.3F, 0.1F, 0.6F},
                      .blend_mode = "screen"}},
    };
}

inline const CorpusCase& find_case(std::span<const CorpusCase> corpus,
                                   std::string_view identifier) {
    const auto found = std::ranges::find(corpus, identifier, [](const CorpusCase& fixture) {
        return std::string_view(fixture.descriptor.identifier);
    });
    if (found == corpus.end()) {
        throw std::invalid_argument("unknown committed parity fixture: " + std::string(identifier));
    }
    return *found;
}

inline std::vector<double> quarter_texel_bilinear(std::span<const double> source,
                                                  std::uint32_t width, std::uint32_t height,
                                                  std::uint8_t components) {
    std::vector<double> result(source.size());
    for (std::uint32_t y = 0; y < height; ++y) {
        const std::uint32_t next_y = std::min(y + 1, height - 1);
        for (std::uint32_t x = 0; x < width; ++x) {
            const std::uint32_t next_x = std::min(x + 1, width - 1);
            for (std::uint8_t component = 0; component < components; ++component) {
                const auto sample = [&](std::uint32_t sample_x, std::uint32_t sample_y) {
                    return source[(static_cast<std::size_t>(sample_y) * width + sample_x) *
                                      components +
                                  component];
                };
                const double top = std::lerp(sample(x, y), sample(next_x, y), 0.25);
                const double bottom = std::lerp(sample(x, next_y), sample(next_x, next_y), 0.25);
                result[(static_cast<std::size_t>(y) * width + x) * components + component] =
                    std::lerp(top, bottom, 0.25);
            }
        }
    }
    return result;
}

inline exec::ParityRenderedFixture render_case(const CorpusCase& fixture) {
    const exec::CpuRasterMeshView mesh{fixture.positions, fixture.uv, fixture.indices};
    const exec::CpuUvRaster raster =
        exec::CpuReferenceExecutor{}.rasterize_uv(mesh, fixture.camera, fixture.document);
    exec::ParityRenderedFixture result{.width = fixture.document.width,
                                       .height = fixture.document.height,
                                       .channels = {{.semantic = "base_color", .values = {}},
                                                    {.semantic = "depth", .values = {}},
                                                    {.semantic = "coverage", .values = {}}}};
    result.channels[0].values.reserve(raster.coverage.size() * 4);
    result.channels[1].values.reserve(raster.coverage.size());
    result.channels[2].values.reserve(raster.coverage.size());
    for (std::uint32_t y = 0; y < fixture.document.height; ++y) {
        for (std::uint32_t x = 0; x < fixture.document.width; ++x) {
            const std::size_t pixel = static_cast<std::size_t>(y) * fixture.document.width + x;
            const float u = (static_cast<float>(x) + 0.5F) / fixture.document.width;
            const float v = 1.0F - (static_cast<float>(y) + 0.5F) / fixture.document.height;
            const float distance =
                std::hypot(u - fixture.stroke.centre.x, v - fixture.stroke.centre.y);
            const double factor =
                raster.coverage[pixel] == 0
                    ? 0.0
                    : fixture.stroke.opacity *
                          std::clamp(1.0F - distance / fixture.stroke.radius, 0.0F, 1.0F);
            const graph::ColourValue colour = graph::blend_colour(
                fixture.material.blend_mode, fixture.material.base, fixture.material.layer, factor);
            result.channels[0].values.insert(result.channels[0].values.end(),
                                             {colour.r, colour.g, colour.b, colour.a});
            result.channels[1].values.push_back(raster.depth[pixel]);
            result.channels[2].values.push_back(raster.coverage[pixel] == 0 ? 0.0 : 1.0);
        }
    }
    if (fixture.descriptor.channels[0].filtered) {
        result.channels[0].values =
            quarter_texel_bilinear(result.channels[0].values, result.width, result.height,
                                   fixture.descriptor.channels[0].component_count);
    }
    return result;
}

inline exec::ParityRenderedFixture render_committed_fixture(const exec::ParityFixture& fixture) {
    const std::vector<CorpusCase> corpus = committed_corpus();
    return render_case(find_case(corpus, fixture.identifier));
}

inline std::vector<exec::ParityFixture> fixture_descriptors() {
    std::vector<exec::ParityFixture> result;
    for (CorpusCase& fixture : committed_corpus()) {
        result.push_back(std::move(fixture.descriptor));
    }
    return result;
}

}  // namespace ctex::test::executor_parity

#endif
