#include <ctex/capi.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum { PREVIEW_CHANNEL_COUNT = 9 };

typedef struct preview_fixture {
    uint32_t formats[4];
    ctex_shader_preview_channel_descriptor channels[PREVIEW_CHANNEL_COUNT];
    ctex_shader_preview_environment_descriptor environment;
    ctex_shader_preview_request request;
} preview_fixture;

typedef struct preview_output {
    unsigned char* vertex;
    unsigned char* fragment;
    char* pass_plan;
    char* workarounds;
    ctex_shader_preview_info info;
} preview_output;

static const char* const semantics[PREVIEW_CHANNEL_COUNT] = {
    "pbr.base_color", "pbr.opacity",   "pbr.roughness", "pbr.metallic",  "pbr.normal",
    "pbr.height",     "pbr.occlusion", "pbr.emission",  "pbr.subsurface"};

static const uint32_t component_counts[PREVIEW_CHANNEL_COUNT] = {3, 1, 1, 1, 3, 1, 1, 3, 1};

static int expect(int condition, const char* message) {
    if (!condition) {
        fprintf(stderr, "%s\n", message);
    }
    return condition;
}

static ctex_shader_texture_descriptor texture(const char* logical_id, uint32_t format,
                                              uint32_t width, uint32_t layers, uint32_t mip_levels,
                                              uint32_t initialized) {
    const ctex_shader_texture_descriptor result = {
        .size = CTEX_SHADER_TEXTURE_DESCRIPTOR_CURRENT_SIZE,
        .logical_id = logical_id,
        .generation = 1,
        .role = "preview fixture",
        .format = format,
        .width = width,
        .height = width,
        .layers = layers,
        .mip_levels = mip_levels,
        .tile_width = 32,
        .tile_height = 32,
        .externally_initialized = initialized,
    };
    return result;
}

static void initialize_fixture(preview_fixture* fixture, uint32_t target,
                               uint32_t with_environment) {
    memset(fixture, 0, sizeof(*fixture));
    fixture->formats[0] = CTEX_EXECUTOR_TEXTURE_R8_UNORM;
    fixture->formats[1] = CTEX_EXECUTOR_TEXTURE_RG16_FLOAT;
    fixture->formats[2] = CTEX_EXECUTOR_TEXTURE_RGBA8_UNORM;
    fixture->formats[3] = CTEX_EXECUTOR_TEXTURE_RGBA16_FLOAT;
    for (size_t index = 0; index < PREVIEW_CHANNEL_COUNT; ++index) {
        fixture->channels[index].size = CTEX_SHADER_PREVIEW_CHANNEL_DESCRIPTOR_CURRENT_SIZE;
        fixture->channels[index].semantic_id = semantics[index];
        fixture->channels[index].component_count = component_counts[index];
        fixture->channels[index].texture =
            texture(semantics[index],
                    component_counts[index] == 1 ? CTEX_EXECUTOR_TEXTURE_R8_UNORM
                                                 : CTEX_EXECUTOR_TEXTURE_RGBA8_UNORM,
                    256, 1, 1, 1);
    }
    fixture->environment.size = CTEX_SHADER_PREVIEW_ENVIRONMENT_DESCRIPTOR_CURRENT_SIZE;
    fixture->environment.radiance =
        texture("lighting/radiance", CTEX_EXECUTOR_TEXTURE_RGBA16_FLOAT, 256, 6, 8, 1);
    fixture->environment.diffuse_irradiance =
        texture("lighting/diffuse", CTEX_EXECUTOR_TEXTURE_RGBA16_FLOAT, 32, 6, 1, 1);
    fixture->environment.specular_brdf_lookup =
        texture("lighting/brdf", CTEX_EXECUTOR_TEXTURE_RG16_FLOAT, 256, 1, 1, 1);
    fixture->request.size = CTEX_SHADER_PREVIEW_REQUEST_CURRENT_SIZE;
    fixture->request.stable_identity = "gallery/material-preview";
    fixture->request.target = target;
    fixture->request.features.size = CTEX_SHADER_DEVICE_FEATURES_DESCRIPTOR_CURRENT_SIZE;
    fixture->request.features.binding_budget = 16;
    fixture->request.features.maximum_texture_dimension = 4096;
    fixture->request.features.supported_texture_formats = fixture->formats;
    fixture->request.features.supported_texture_format_count = 4;
    fixture->request.features.floating_point_filtering = 1;
    fixture->request.features.compute_available = 0;
    fixture->request.channels = fixture->channels;
    fixture->request.channel_count = PREVIEW_CHANNEL_COUNT;
    fixture->request.output =
        texture("preview/output", CTEX_EXECUTOR_TEXTURE_RGBA8_UNORM, 512, 1, 1, 0);
    fixture->request.environment = with_environment ? &fixture->environment : NULL;
    fixture->request.analytic_light_count = 2;
    fixture->request.vertex_count = 36;
}

static ctex_result emit(ctex_shader_emission_cache* cache,
                        const ctex_shader_preview_request* request, const char* semantic_id,
                        ctex_shader_preview_info* info, void* vertex, size_t vertex_size,
                        void* fragment, size_t fragment_size, char* pass_plan,
                        size_t pass_plan_size, char* workarounds, size_t workarounds_size) {
    if (semantic_id == NULL) {
        return ctex_shader_emit_lit_preview(cache, request, info, vertex, vertex_size, fragment,
                                            fragment_size, pass_plan, pass_plan_size, workarounds,
                                            workarounds_size);
    }
    return ctex_shader_emit_channel_inspection(cache, request, semantic_id, info, vertex,
                                               vertex_size, fragment, fragment_size, pass_plan,
                                               pass_plan_size, workarounds, workarounds_size);
}

static void release_output(preview_output* output) {
    free(output->vertex);
    free(output->fragment);
    free(output->pass_plan);
    free(output->workarounds);
    memset(output, 0, sizeof(*output));
}

static int read_output(ctex_shader_emission_cache* cache,
                       const ctex_shader_preview_request* request, const char* semantic_id,
                       preview_output* output, uint32_t* sizing_cache_hit) {
    output->info.size = CTEX_SHADER_PREVIEW_INFO_CURRENT_SIZE;
    if (emit(cache, request, semantic_id, &output->info, NULL, 0, NULL, 0, NULL, 0, NULL, 0) !=
        CTEX_RESULT_SUCCESS) {
        return 0;
    }
    *sizing_cache_hit = output->info.cache_hit;
    output->vertex = (unsigned char*)malloc(output->info.vertex_artifact_size);
    output->fragment = output->info.fragment_artifact_size == 0
                           ? NULL
                           : (unsigned char*)malloc(output->info.fragment_artifact_size);
    output->pass_plan = (char*)malloc(output->info.pass_plan_size);
    output->workarounds = (char*)malloc(output->info.workaround_report_size);
    if (output->vertex == NULL ||
        (output->info.fragment_artifact_size != 0 && output->fragment == NULL) ||
        output->pass_plan == NULL || output->workarounds == NULL) {
        release_output(output);
        return 0;
    }
    return emit(cache, request, semantic_id, &output->info, output->vertex,
                output->info.vertex_artifact_size, output->fragment,
                output->info.fragment_artifact_size, output->pass_plan, output->info.pass_plan_size,
                output->workarounds, output->info.workaround_report_size) == CTEX_RESULT_SUCCESS;
}

static int same_output(const preview_output* left, const preview_output* right) {
    return left->info.vertex_artifact_size == right->info.vertex_artifact_size &&
           left->info.fragment_artifact_size == right->info.fragment_artifact_size &&
           memcmp(left->vertex, right->vertex, left->info.vertex_artifact_size) == 0 &&
           memcmp(left->fragment, right->fragment, left->info.fragment_artifact_size) == 0 &&
           strcmp(left->pass_plan, right->pass_plan) == 0 &&
           strcmp(left->workarounds, right->workarounds) == 0;
}

static int lit_preview_declares_lighting_and_cache(void) {
    preview_fixture fixture;
    preview_output first = {0};
    preview_output second = {0};
    ctex_shader_emission_cache* cache = NULL;
    uint32_t first_sizing_hit = 1;
    uint32_t second_sizing_hit = 0;
    initialize_fixture(&fixture, CTEX_MATERIAL_GRAPH_TARGET_WGSL, 1);
    int passed = ctex_shader_emission_cache_create(&cache) == CTEX_RESULT_SUCCESS &&
                 read_output(cache, &fixture.request, NULL, &first, &first_sizing_hit) &&
                 read_output(cache, &fixture.request, NULL, &second, &second_sizing_hit);
    if (passed) {
        passed =
            expect(first_sizing_hit == 0 && first.info.cache_hit == 1 && second_sizing_hit == 1 &&
                       second.info.cache_hit == 1,
                   "unchanged lit preview was not served from cache") &&
            expect(first.info.kind == CTEX_SHADER_PREVIEW_LIT &&
                       first.info.fallback_lighting == 0 && first.info.pass_count == 1 &&
                       first.info.binding_count == 13,
                   "lit preview metadata was incomplete") &&
            expect(strstr(first.pass_plan, "prefiltered environment radiance") != NULL &&
                       strstr(first.pass_plan, "\"view_dimension\":\"cube\"") != NULL &&
                       strstr(first.pass_plan, "GGX roughness") != NULL &&
                       strstr(first.pass_plan, "diffuse environment irradiance") != NULL &&
                       strstr(first.pass_plan, "not divided by pi") != NULL &&
                       strstr(first.pass_plan, "split-sum specular BRDF lookup") != NULL &&
                       strstr(first.pass_plan, "Fresnel scale") != NULL,
                   "environment textures omitted view, encoding, or mip contracts") &&
            expect(
                strstr(first.pass_plan, "\"uniform_blocks\":[{") != NULL &&
                    strstr(first.pass_plan, "\"name\":\"camera_position\",\"offset\":0") != NULL &&
                    strstr(first.pass_plan,
                           "\"name\":\"environment_rotation_intensity\",\"offset\":16") != NULL &&
                    strstr(first.pass_plan, "\"name\":\"light_1_color\",\"offset\":80") != NULL,
                "preview uniform layout omitted camera, environment, or light fields") &&
            expect(same_output(&first, &second),
                   "preview cache hit changed artifacts or the pass plan");
    }
    release_output(&first);
    release_output(&second);
    ctex_shader_emission_cache_destroy(cache);
    return passed;
}

static int fallback_lighting_compiles_for_every_target(void) {
    static const uint32_t targets[] = {
        CTEX_MATERIAL_GRAPH_TARGET_WGSL, CTEX_MATERIAL_GRAPH_TARGET_MSL,
        CTEX_MATERIAL_GRAPH_TARGET_SPIRV, CTEX_MATERIAL_GRAPH_TARGET_HLSL};
    int passed = 1;
    for (size_t index = 0; index < sizeof(targets) / sizeof(targets[0]); ++index) {
        preview_fixture fixture;
        preview_output output = {0};
        uint32_t ignored = 0;
        initialize_fixture(&fixture, targets[index], 0);
        if (!read_output(NULL, &fixture.request, NULL, &output, &ignored)) {
            return expect(0, "fallback preview failed for a declared target");
        }
        passed =
            expect(output.info.target == targets[index] && output.info.fallback_lighting == 1 &&
                       strstr(output.pass_plan, "prefiltered environment radiance") == NULL &&
                       strstr(output.pass_plan, "diffuse environment irradiance") == NULL,
                   "fallback preview retained environment resource bindings") &&
            passed;
        if (targets[index] == CTEX_MATERIAL_GRAPH_TARGET_SPIRV) {
            uint32_t magic = 0;
            memcpy(&magic, output.vertex, sizeof(magic));
            passed = expect(magic == UINT32_C(0x07230203),
                            "fallback SPIR-V preview was not a binary module") &&
                     passed;
        }
        release_output(&output);
    }
    return passed;
}

static int every_channel_is_inspectable_without_lighting(void) {
    preview_fixture fixture;
    initialize_fixture(&fixture, CTEX_MATERIAL_GRAPH_TARGET_WGSL, 0);
    for (size_t index = 0; index < PREVIEW_CHANNEL_COUNT; ++index) {
        ctex_shader_preview_info info = {.size = CTEX_SHADER_PREVIEW_INFO_CURRENT_SIZE};
        if (!expect(emit(NULL, &fixture.request, semantics[index], &info, NULL, 0, NULL, 0, NULL, 0,
                         NULL, 0) == CTEX_RESULT_SUCCESS &&
                        info.kind == CTEX_SHADER_PREVIEW_CHANNEL_INSPECTION &&
                        info.binding_count == 2,
                    "a declared channel did not produce an inspection shader")) {
            return 0;
        }
    }
    preview_output roughness = {0};
    uint32_t ignored = 0;
    if (!read_output(NULL, &fixture.request, "pbr.roughness", &roughness, &ignored)) {
        return 0;
    }
    const int passed =
        expect(strstr(roughness.pass_plan, "inspection channel pbr.roughness") != NULL &&
                   strstr(roughness.pass_plan, "\"uniform_blocks\":[]") != NULL &&
                   strstr(roughness.pass_plan, "environment radiance") == NULL,
               "channel inspection retained a lighting dependency");
    release_output(&roughness);
    return passed;
}

static int invalid_environment_and_short_output_are_refused(void) {
    preview_fixture fixture;
    initialize_fixture(&fixture, CTEX_MATERIAL_GRAPH_TARGET_WGSL, 1);
    fixture.environment.radiance.format = CTEX_EXECUTOR_TEXTURE_RGBA8_UNORM;
    ctex_shader_preview_info info = {.size = CTEX_SHADER_PREVIEW_INFO_CURRENT_SIZE};
    int passed = expect(emit(NULL, &fixture.request, NULL, &info, NULL, 0, NULL, 0, NULL, 0, NULL,
                             0) == CTEX_RESULT_INVALID_ARGUMENT &&
                            strstr(ctex_get_last_diagnostic(), "RGBA16-float cube") != NULL,
                        "invalid preview environment format was accepted");

    preview_output output = {0};
    uint32_t ignored = 0;
    initialize_fixture(&fixture, CTEX_MATERIAL_GRAPH_TARGET_WGSL, 1);
    if (!read_output(NULL, &fixture.request, NULL, &output, &ignored)) {
        return 0;
    }
    memset(output.vertex, 0xa5, output.info.vertex_artifact_size);
    memset(output.pass_plan, 0xa5, output.info.pass_plan_size);
    const ctex_result result =
        emit(NULL, &fixture.request, NULL, &output.info, output.vertex,
             output.info.vertex_artifact_size - 1, output.fragment,
             output.info.fragment_artifact_size, output.pass_plan, output.info.pass_plan_size,
             output.workarounds, output.info.workaround_report_size);
    passed = expect(result == CTEX_RESULT_BUFFER_TOO_SMALL && output.vertex[0] == 0xa5 &&
                        (unsigned char)output.pass_plan[0] == 0xa5,
                    "undersized preview output partially published data") &&
             passed;
    release_output(&output);
    return passed;
}

int main(void) {
    return lit_preview_declares_lighting_and_cache() &&
                   fallback_lighting_compiles_for_every_target() &&
                   every_channel_is_inspectable_without_lighting() &&
                   invalid_environment_and_short_output_are_refused()
               ? EXIT_SUCCESS
               : EXIT_FAILURE;
}
