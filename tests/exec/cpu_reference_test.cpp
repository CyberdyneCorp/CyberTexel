#include <algorithm>
#include <array>
#include <cmath>
#include <ctex/exec/cpu_reference.hpp>
#include <iostream>
#include <limits>
#include <string_view>

namespace {

using ctex::exec::CpuMat4f;
using ctex::exec::CpuRasterCamera;
using ctex::exec::CpuRasterMeshView;
using ctex::exec::CpuReferenceExecutor;
using ctex::exec::CpuUvRasterRequest;
using ctex::exec::CpuVec2f;
using ctex::exec::CpuVec3f;

constexpr CpuMat4f identity{{1.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1.0F, 0.0F,
                             0.0F, 0.0F, 0.0F, 1.0F}};

bool expect(bool condition, std::string_view message) {
    if (!condition) {
        std::cerr << message << '\n';
    }
    return condition;
}

bool near(float actual, float expected, float tolerance = 1.0e-5F) {
    return std::abs(actual - expected) <= tolerance;
}

struct TriangleFixture {
    std::array<CpuVec3f, 3> positions{
        {{-1.0F, -1.0F, 0.0F}, {1.0F, -1.0F, 0.0F}, {-1.0F, 1.0F, 0.0F}}};
    std::array<CpuVec2f, 3> uv{{{0.0F, 0.0F}, {1.0F, 0.0F}, {0.0F, 1.0F}}};
    std::array<std::uint32_t, 3> indices{{0, 1, 2}};

    [[nodiscard]] CpuRasterMeshView view() const { return {positions, uv, indices}; }
};

class CountingOperation final : public ctex::exec::CpuOperation {
public:
    [[nodiscard]] std::string_view identifier() const noexcept override { return "count"; }
    void execute(const CpuReferenceExecutor& executor) override {
        saw_cpu = executor.descriptor().route == ctex::exec::ExecutorRoute::cpu_reference;
        ++runs;
    }
    int runs{};
    bool saw_cpu{};
};

bool executor_is_always_available_and_runs_cpu_semantics() {
    CpuReferenceExecutor executor;
    CountingOperation operation;
    const auto record = executor.execute(operation);
    return expect(executor.descriptor().identifier == "cpu" &&
                      executor.descriptor().route == ctex::exec::ExecutorRoute::cpu_reference &&
                      executor.descriptor().availability ==
                          ctex::exec::ExecutorAvailability::available &&
                      executor.descriptor().device_name == "System CPU",
                  "CPU reference descriptor did not identify an always-available CPU route") &&
           expect(operation.runs == 1 && operation.saw_cpu && record.completed &&
                      record.operation == "count",
                  "CPU operation semantics did not execute exactly once");
}

bool viewport_raster_owns_depth_uv_and_coverage() {
    const TriangleFixture fixture;
    const auto raster = CpuReferenceExecutor{}.rasterize_viewport(
        fixture.view(), CpuRasterCamera{.view_projection = identity, .width = 4, .height = 4});
    const std::size_t pixel = 2 * raster.width + 1;
    return expect(raster.depth.size() == 16 && raster.uv.size() == 16 &&
                      raster.coverage.size() == 16 && raster.triangle.size() == 16,
                  "viewport raster did not own one value per pixel") &&
           expect(raster.coverage[pixel] == 1 && near(raster.depth[pixel], 0.5F) &&
                      near(raster.uv[pixel].x, 0.375F) && near(raster.uv[pixel].y, 0.375F) &&
                      raster.triangle[pixel] == 0,
                  "viewport raster did not interpolate depth, UV, coverage, and triangle") &&
           expect(raster.coverage[3] == 0 && raster.depth[3] == 1.0F &&
                      raster.triangle[3] == ctex::exec::no_raster_triangle,
                  "uncovered viewport pixels did not retain explicit clear values");
}

bool nearest_surface_wins_independent_of_submission_order() {
    std::array<CpuVec3f, 6> positions{{{-1.0F, -1.0F, 0.5F},
                                       {1.0F, -1.0F, 0.5F},
                                       {-1.0F, 1.0F, 0.5F},
                                       {-1.0F, -1.0F, -0.5F},
                                       {1.0F, -1.0F, -0.5F},
                                       {-1.0F, 1.0F, -0.5F}}};
    std::array<CpuVec2f, 6> uv{
        {{0.0F, 0.0F}, {1.0F, 0.0F}, {0.0F, 1.0F}, {0.2F, 0.2F}, {0.8F, 0.2F}, {0.2F, 0.8F}}};
    std::array<std::uint32_t, 6> indices{{0, 1, 2, 3, 4, 5}};
    const auto raster = CpuReferenceExecutor{}.rasterize_viewport(
        {positions, uv, indices},
        CpuRasterCamera{.view_projection = identity, .width = 4, .height = 4});
    const std::size_t pixel = 2 * raster.width + 1;
    return expect(near(raster.depth[pixel], 0.25F) && raster.triangle[pixel] == 1 &&
                      near(raster.uv[pixel].x, 0.425F),
                  "viewport depth test did not retain the nearest surface");
}

bool clip_planes_are_applied_before_rasterization() {
    std::array<CpuVec3f, 6> outside{{{-2.0F, -1.0F, 0.0F},
                                     {2.0F, -1.0F, 0.0F},
                                     {-1.0F, -2.0F, 0.0F},
                                     {-1.0F, 2.0F, 0.0F},
                                     {-1.0F, -1.0F, -2.0F},
                                     {-1.0F, -1.0F, 2.0F}}};
    for (CpuVec3f position : outside) {
        TriangleFixture fixture;
        fixture.positions[0] = position;
        const auto raster = CpuReferenceExecutor{}.rasterize_viewport(
            fixture.view(), CpuRasterCamera{.view_projection = identity, .width = 8, .height = 8});
        const auto covered = std::count(raster.coverage.begin(), raster.coverage.end(), 1);
        if (!expect(covered > 0 && covered < 64,
                    "triangle crossing a clip plane was discarded or escaped the viewport")) {
            return false;
        }
    }
    return true;
}

bool viewport_uv_is_perspective_correct() {
    const TriangleFixture fixture;
    constexpr CpuMat4f varying_w{{1.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F,
                                  1.0F, 0.0F, 0.0F, 0.0F, 2.0F}};
    auto positions = fixture.positions;
    positions[2].z = 2.0F;
    const auto raster = CpuReferenceExecutor{}.rasterize_viewport(
        {positions, fixture.uv, fixture.indices},
        CpuRasterCamera{.view_projection = varying_w, .width = 8, .height = 8});
    const std::size_t pixel = 4 * raster.width + 3;
    return expect(raster.coverage[pixel] == 1 && near(raster.uv[pixel].x, 1.0F / 3.0F) &&
                      near(raster.uv[pixel].y, 1.0F / 3.0F),
                  "viewport UV interpolation was affine instead of perspective-correct");
}

bool uv_space_raster_projects_texels_to_the_camera() {
    const TriangleFixture fixture;
    const auto raster = CpuReferenceExecutor{}.rasterize_uv(
        fixture.view(), CpuRasterCamera{.view_projection = identity, .width = 8, .height = 8},
        CpuUvRasterRequest{.width = 4, .height = 4});
    const std::size_t pixel = 2 * raster.width + 1;
    return expect(raster.coverage[pixel] == 1 && raster.triangle[pixel] == 0,
                  "UV-space raster did not cover the triangle's texture texels") &&
           expect(near(raster.depth[pixel], 0.5F) && near(raster.screen_position[pixel].x, 3.0F) &&
                      near(raster.screen_position[pixel].y, 5.0F),
                  "UV-space raster did not independently project depth and screen position");
}

bool invalid_inputs_are_refused_before_rasterization() {
    TriangleFixture fixture;
    fixture.positions[0].x = std::numeric_limits<float>::quiet_NaN();
    bool non_finite_refused = false;
    try {
        static_cast<void>(CpuReferenceExecutor{}.rasterize_viewport(
            fixture.view(), CpuRasterCamera{.view_projection = identity, .width = 4, .height = 4}));
    } catch (const std::invalid_argument& error) {
        non_finite_refused =
            std::string_view(error.what()).find("finite") != std::string_view::npos;
    }
    const TriangleFixture valid;
    bool dimensions_refused = false;
    try {
        static_cast<void>(CpuReferenceExecutor{}.rasterize_uv(
            valid.view(), CpuRasterCamera{.view_projection = identity, .width = 4, .height = 4},
            CpuUvRasterRequest{.width = 0, .height = 4}));
    } catch (const std::invalid_argument& error) {
        dimensions_refused =
            std::string_view(error.what()).find("dimensions") != std::string_view::npos;
    }
    return expect(non_finite_refused && dimensions_refused,
                  "invalid CPU raster input was not refused with a named diagnostic");
}

}  // namespace

int main() {
    return executor_is_always_available_and_runs_cpu_semantics() &&
                   viewport_raster_owns_depth_uv_and_coverage() &&
                   nearest_surface_wins_independent_of_submission_order() &&
                   clip_planes_are_applied_before_rasterization() &&
                   viewport_uv_is_perspective_correct() &&
                   uv_space_raster_projects_texels_to_the_camera() &&
                   invalid_inputs_are_refused_before_rasterization()
               ? 0
               : 1;
}
