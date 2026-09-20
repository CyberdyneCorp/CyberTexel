#include <ctex/capi.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static int expect(int condition, const char* message) {
    if (!condition) {
        fprintf(stderr, "%s\n", message);
    }
    return condition;
}

static int near(float actual, float expected) {
    float difference = actual - expected;
    if (difference < 0.0F) {
        difference = -difference;
    }
    return difference <= 1.0e-5F;
}

static ctex_cpu_raster_mesh_descriptor triangle_mesh(void) {
    static const ctex_vec3f positions[] = {
        {-1.0F, -1.0F, 0.0F}, {1.0F, -1.0F, 0.0F}, {-1.0F, 1.0F, 0.0F}};
    static const ctex_vec2f uv[] = {{0.0F, 0.0F}, {1.0F, 0.0F}, {0.0F, 1.0F}};
    static const uint32_t indices[] = {0, 1, 2};
    const ctex_cpu_raster_mesh_descriptor result = {
        .size = CTEX_CPU_RASTER_MESH_DESCRIPTOR_CURRENT_SIZE,
        .positions = positions,
        .uv = uv,
        .vertex_count = 3,
        .triangle_indices = indices,
        .triangle_index_count = 3,
    };
    return result;
}

static ctex_cpu_raster_camera_descriptor identity_camera(uint32_t width, uint32_t height) {
    const ctex_cpu_raster_camera_descriptor result = {
        .size = CTEX_CPU_RASTER_CAMERA_DESCRIPTOR_CURRENT_SIZE,
        .view_projection = {1.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1.0F, 0.0F,
                            0.0F, 0.0F, 0.0F, 1.0F},
        .width = width,
        .height = height,
    };
    return result;
}

static ctex_cpu_raster_outputs raster_outputs(float* depth, ctex_vec2f* coordinates,
                                              uint8_t* coverage, uint32_t* triangles,
                                              size_t capacity) {
    const ctex_cpu_raster_outputs result = {
        .size = CTEX_CPU_RASTER_OUTPUTS_CURRENT_SIZE,
        .depth = depth,
        .depth_capacity = capacity,
        .coordinates = coordinates,
        .coordinate_capacity = capacity,
        .coverage = coverage,
        .coverage_capacity = capacity,
        .triangle_identity = triangles,
        .triangle_identity_capacity = capacity,
    };
    return result;
}

static int viewport_raster_is_independent_and_atomic(void) {
    const ctex_cpu_raster_mesh_descriptor mesh = triangle_mesh();
    const ctex_cpu_raster_camera_descriptor camera = identity_camera(4, 4);
    const ctex_cpu_viewport_raster_descriptor descriptor = {
        .size = CTEX_CPU_VIEWPORT_RASTER_DESCRIPTOR_CURRENT_SIZE,
        .mesh = &mesh,
        .camera = &camera,
        .maximum_output_pixels = 16,
    };
    ctex_cpu_raster_info info = {.size = CTEX_CPU_RASTER_INFO_CURRENT_SIZE};
    if (!expect(ctex_cpu_reference_rasterize_viewport(&descriptor, &info, NULL) ==
                        CTEX_RESULT_SUCCESS &&
                    info.width == 4 && info.height == 4 && info.pixel_count == 16,
                "CPU viewport raster sizing did not report its complete output")) {
        return 0;
    }

    float depth[16];
    ctex_vec2f uv[16];
    uint8_t coverage[16];
    uint32_t triangles[16];
    ctex_cpu_raster_outputs outputs = raster_outputs(depth, uv, coverage, triangles, 16);
    if (!expect(ctex_cpu_reference_rasterize_viewport(&descriptor, &info, &outputs) ==
                    CTEX_RESULT_SUCCESS,
                "CPU viewport raster failed")) {
        return 0;
    }
    const size_t covered = 2 * info.width + 1;
    if (!expect(coverage[covered] == 1 && near(depth[covered], 0.5F) &&
                    near(uv[covered].x, 0.375F) && near(uv[covered].y, 0.375F) &&
                    triangles[covered] == 0 && coverage[3] == 0 && near(depth[3], 1.0F) &&
                    triangles[3] == UINT32_MAX,
                "CPU viewport raster did not independently produce depth, UV and coverage")) {
        return 0;
    }

    depth[0] = 7.0F;
    outputs.coverage_capacity = 15;
    if (!expect(ctex_cpu_reference_rasterize_viewport(&descriptor, &info, &outputs) ==
                        CTEX_RESULT_BUFFER_TOO_SMALL &&
                    near(depth[0], 7.0F),
                "short CPU raster output partially published another output")) {
        return 0;
    }
    ctex_cpu_viewport_raster_descriptor over_budget = descriptor;
    over_budget.maximum_output_pixels = 15;
    return expect(
        ctex_cpu_reference_rasterize_viewport(&over_budget, &info, NULL) == CTEX_RESULT_OVER_BUDGET,
        "CPU viewport raster ignored its caller-declared pixel ceiling");
}

static int uv_raster_projects_to_the_camera(void) {
    const ctex_cpu_raster_mesh_descriptor mesh = triangle_mesh();
    const ctex_cpu_raster_camera_descriptor camera = identity_camera(8, 8);
    const ctex_cpu_uv_raster_descriptor descriptor = {
        .size = CTEX_CPU_UV_RASTER_DESCRIPTOR_CURRENT_SIZE,
        .mesh = &mesh,
        .camera = &camera,
        .width = 4,
        .height = 4,
        .tile_origin = {0.0F, 0.0F},
        .maximum_output_pixels = 16,
    };
    ctex_cpu_raster_info info = {.size = CTEX_CPU_RASTER_INFO_CURRENT_SIZE};
    float depth[16];
    ctex_vec2f screen[16];
    uint8_t coverage[16];
    uint32_t triangles[16];
    const ctex_cpu_raster_outputs outputs = raster_outputs(depth, screen, coverage, triangles, 16);
    if (!expect(
            ctex_cpu_reference_rasterize_uv(&descriptor, &info, &outputs) == CTEX_RESULT_SUCCESS,
            "CPU UV-space raster failed")) {
        return 0;
    }
    const size_t covered = 2 * info.width + 1;
    return expect(info.pixel_count == 16 && coverage[covered] == 1 && triangles[covered] == 0 &&
                      near(depth[covered], 0.5F) && near(screen[covered].x, 3.0F) &&
                      near(screen[covered].y, 5.0F),
                  "CPU UV raster did not independently project camera depth and screen position");
}

static int invalid_mesh_is_refused(void) {
    ctex_cpu_raster_mesh_descriptor mesh = triangle_mesh();
    static const uint32_t invalid_indices[] = {0, 1, 3};
    mesh.triangle_indices = invalid_indices;
    const ctex_cpu_raster_camera_descriptor camera = identity_camera(4, 4);
    const ctex_cpu_viewport_raster_descriptor descriptor = {
        .size = CTEX_CPU_VIEWPORT_RASTER_DESCRIPTOR_CURRENT_SIZE,
        .mesh = &mesh,
        .camera = &camera,
        .maximum_output_pixels = 16,
    };
    ctex_cpu_raster_info info = {.size = CTEX_CPU_RASTER_INFO_CURRENT_SIZE};
    return expect(ctex_cpu_reference_rasterize_viewport(&descriptor, &info, NULL) ==
                          CTEX_RESULT_INVALID_ARGUMENT &&
                      ctex_get_last_diagnostic_code() == CTEX_DIAGNOSTIC_INVALID_EXECUTOR,
                  "CPU raster accepted an out-of-range source triangle");
}

int main(void) {
    return viewport_raster_is_independent_and_atomic() && uv_raster_projects_to_the_camera() &&
                   invalid_mesh_is_refused()
               ? 0
               : 1;
}
