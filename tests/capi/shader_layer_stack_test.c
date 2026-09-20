#include <ctex/capi.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum { LAYER_COUNT = 8 };

typedef struct layer_fixture {
    uint32_t formats[2];
    char identifiers[LAYER_COUNT][24];
    char logical_ids[LAYER_COUNT][24];
    ctex_shader_layer_descriptor layers[LAYER_COUNT];
    ctex_shader_layer_stack_request request;
} layer_fixture;

typedef struct layer_output {
    unsigned char* artifacts;
    char* artifact_report;
    char* pass_plan;
    char* workarounds;
    ctex_shader_layer_stack_info info;
} layer_output;

typedef struct graph_blob {
    unsigned char* data;
    size_t size;
} graph_blob;

typedef struct material_output {
    unsigned char* vertex;
    unsigned char* fragment;
    char* pass_plan;
    char* workarounds;
    ctex_shader_material_info info;
    uint32_t cache_hit;
} material_output;

static int expect(int condition, const char* message) {
    if (!condition) {
        fprintf(stderr, "%s\n", message);
    }
    return condition;
}

static ctex_shader_texture_descriptor texture(const char* logical_id, uint32_t format,
                                              uint32_t initialized) {
    const ctex_shader_texture_descriptor result = {
        .size = CTEX_SHADER_TEXTURE_DESCRIPTOR_CURRENT_SIZE,
        .logical_id = logical_id,
        .generation = 1,
        .role = "layer-stack texture",
        .format = format,
        .width = 256,
        .height = 128,
        .layers = 1,
        .mip_levels = 1,
        .tile_width = 64,
        .tile_height = 64,
        .externally_initialized = initialized,
    };
    return result;
}

static void initialize_fixture(layer_fixture* fixture, uint32_t binding_budget, uint32_t format,
                               uint32_t target, uint32_t floating_point_filtering) {
    memset(fixture, 0, sizeof(*fixture));
    fixture->formats[0] = CTEX_EXECUTOR_TEXTURE_RGBA8_UNORM;
    fixture->formats[1] = CTEX_EXECUTOR_TEXTURE_RGBA16_FLOAT;
    for (size_t index = 0; index < LAYER_COUNT; ++index) {
        (void)snprintf(fixture->identifiers[index], sizeof(fixture->identifiers[index]),
                       "layer-%zu", index);
        (void)snprintf(fixture->logical_ids[index], sizeof(fixture->logical_ids[index]),
                       "source-%zu", index);
        fixture->layers[index].size = CTEX_SHADER_LAYER_DESCRIPTOR_CURRENT_SIZE;
        fixture->layers[index].identifier = fixture->identifiers[index];
        fixture->layers[index].texture = texture(fixture->logical_ids[index], format, 1);
    }
    fixture->request.size = CTEX_SHADER_LAYER_STACK_REQUEST_CURRENT_SIZE;
    fixture->request.stable_identity = "gallery/layer-stack";
    fixture->request.target = target;
    fixture->request.features.size = CTEX_SHADER_DEVICE_FEATURES_DESCRIPTOR_CURRENT_SIZE;
    fixture->request.features.binding_budget = binding_budget;
    fixture->request.features.maximum_texture_dimension = 4096;
    fixture->request.features.supported_texture_formats = fixture->formats;
    fixture->request.features.supported_texture_format_count = 2;
    fixture->request.features.floating_point_filtering = floating_point_filtering;
    fixture->request.features.compute_available = 0;
    fixture->request.layers = fixture->layers;
    fixture->request.layer_count = LAYER_COUNT;
    fixture->request.output = texture("composite", format, 0);
    fixture->request.requested_filter = CTEX_SHADER_FILTER_LINEAR;
}

static void release_layer_output(layer_output* output) {
    free(output->artifacts);
    free(output->artifact_report);
    free(output->pass_plan);
    free(output->workarounds);
    memset(output, 0, sizeof(*output));
}

static int read_layer_stack(ctex_shader_emission_cache* cache,
                            const ctex_shader_layer_stack_request* request, layer_output* output,
                            uint32_t* sizing_cache_hit) {
    output->info.size = CTEX_SHADER_LAYER_STACK_INFO_CURRENT_SIZE;
    if (ctex_shader_emit_layer_stack(cache, request, &output->info, NULL, 0, NULL, 0, NULL, 0, NULL,
                                     0) != CTEX_RESULT_SUCCESS) {
        return 0;
    }
    *sizing_cache_hit = output->info.cache_hit;
    output->artifacts = (unsigned char*)malloc(output->info.artifact_blob_size);
    output->artifact_report = (char*)malloc(output->info.artifact_report_size);
    output->pass_plan = (char*)malloc(output->info.pass_plan_size);
    output->workarounds = (char*)malloc(output->info.workaround_report_size);
    if (output->artifacts == NULL || output->artifact_report == NULL || output->pass_plan == NULL ||
        output->workarounds == NULL) {
        release_layer_output(output);
        return 0;
    }
    return ctex_shader_emit_layer_stack(cache, request, &output->info, output->artifacts,
                                        output->info.artifact_blob_size, output->artifact_report,
                                        output->info.artifact_report_size, output->pass_plan,
                                        output->info.pass_plan_size, output->workarounds,
                                        output->info.workaround_report_size) == CTEX_RESULT_SUCCESS;
}

static int same_layer_output(const layer_output* left, const layer_output* right) {
    return left->info.artifact_blob_size == right->info.artifact_blob_size &&
           memcmp(left->artifacts, right->artifacts, left->info.artifact_blob_size) == 0 &&
           strcmp(left->artifact_report, right->artifact_report) == 0 &&
           strcmp(left->pass_plan, right->pass_plan) == 0 &&
           strcmp(left->workarounds, right->workarounds) == 0;
}

static int one_pass_and_cache_are_exposed(void) {
    ctex_shader_emission_cache* cache = NULL;
    layer_fixture fixture;
    layer_output first = {0};
    layer_output second = {0};
    uint32_t first_sizing_hit = 1;
    uint32_t second_sizing_hit = 0;
    initialize_fixture(&fixture, 9, CTEX_EXECUTOR_TEXTURE_RGBA8_UNORM,
                       CTEX_MATERIAL_GRAPH_TARGET_WGSL, 1);
    int passed = ctex_shader_emission_cache_create(&cache) == CTEX_RESULT_SUCCESS &&
                 read_layer_stack(cache, &fixture.request, &first, &first_sizing_hit) &&
                 read_layer_stack(cache, &fixture.request, &second, &second_sizing_hit);
    if (passed) {
        passed =
            expect(first_sizing_hit == 0 && first.info.cache_hit == 1 && second_sizing_hit == 1 &&
                       second.info.cache_hit == 1,
                   "unchanged layer stack was not served from the emission cache") &&
            expect(first.info.layer_count == LAYER_COUNT && first.info.pass_count == 1,
                   "eight fitting layers were not emitted as one pass") &&
            expect(strstr(first.artifact_report, "\"pass_identifier\":\"layer-stack-0\"") != NULL &&
                       strstr(first.artifact_report, "\"target\":\"WGSL\"") != NULL &&
                       strstr(first.artifact_report, "\"encoding\":\"text\"") != NULL &&
                       strstr(first.pass_plan, "\"texture_bindings\":[") != NULL &&
                       strstr(first.pass_plan, "\"sampler_bindings\":[") != NULL,
                   "layer-stack artifact inventory or pass plan was incomplete") &&
            expect(same_layer_output(&first, &second),
                   "a layer-stack cache hit changed artifacts or the pass plan");
    }
    release_layer_output(&first);
    release_layer_output(&second);
    ctex_shader_emission_cache_destroy(cache);
    return passed;
}

static int split_workaround_and_spirv_are_exposed(void) {
    layer_fixture fixture;
    layer_output split = {0};
    layer_output float_workaround = {0};
    layer_output spirv = {0};
    uint32_t ignored = 0;
    initialize_fixture(&fixture, 4, CTEX_EXECUTOR_TEXTURE_RGBA8_UNORM,
                       CTEX_MATERIAL_GRAPH_TARGET_WGSL, 1);
    int passed = read_layer_stack(NULL, &fixture.request, &split, &ignored);
    if (passed) {
        passed =
            expect(split.info.pass_count == 4 &&
                       strstr(split.pass_plan, "\"dependencies\":[\"layer-stack-0\"]") != NULL &&
                       strstr(split.pass_plan, "carried composite") != NULL,
                   "binding-budget split did not expose carried intermediate passes");
    }
    initialize_fixture(&fixture, 9, CTEX_EXECUTOR_TEXTURE_RGBA16_FLOAT,
                       CTEX_MATERIAL_GRAPH_TARGET_WGSL, 0);
    passed = read_layer_stack(NULL, &fixture.request, &float_workaround, &ignored) && passed;
    if (passed) {
        passed =
            expect(float_workaround.info.workaround_count == 1 &&
                       strstr(float_workaround.workarounds, "nearest_float_sampling") != NULL &&
                       strstr(float_workaround.pass_plan, "\"min_filter\":\"nearest\"") != NULL,
                   "float-filtering workaround was not published");
    }
    initialize_fixture(&fixture, 9, CTEX_EXECUTOR_TEXTURE_RGBA8_UNORM,
                       CTEX_MATERIAL_GRAPH_TARGET_SPIRV, 1);
    passed = read_layer_stack(NULL, &fixture.request, &spirv, &ignored) && passed;
    if (passed) {
        uint32_t magic = 0;
        memcpy(&magic, spirv.artifacts, sizeof(magic));
        passed = expect(magic == UINT32_C(0x07230203) &&
                            strstr(spirv.artifact_report, "\"encoding\":\"spirv\"") != NULL &&
                            strstr(spirv.artifact_report, "\"target\":\"SPIR-V\"") != NULL,
                        "packed SPIR-V layer artifact was not binary or inventoried");
    }
    release_layer_output(&split);
    release_layer_output(&float_workaround);
    release_layer_output(&spirv);
    return passed;
}

static int cache_keys_statistics_and_clear_are_exposed(void) {
    ctex_shader_emission_cache* cache = NULL;
    layer_fixture fixture;
    ctex_shader_layer_stack_info info = {.size = CTEX_SHADER_LAYER_STACK_INFO_CURRENT_SIZE};
    ctex_shader_emission_cache_info cache_info = {.size =
                                                      CTEX_SHADER_EMISSION_CACHE_INFO_CURRENT_SIZE};
    initialize_fixture(&fixture, 9, CTEX_EXECUTOR_TEXTURE_RGBA8_UNORM,
                       CTEX_MATERIAL_GRAPH_TARGET_WGSL, 1);
    int passed = ctex_shader_emission_cache_create(&cache) == CTEX_RESULT_SUCCESS &&
                 ctex_shader_emit_layer_stack(cache, &fixture.request, &info, NULL, 0, NULL, 0,
                                              NULL, 0, NULL, 0) == CTEX_RESULT_SUCCESS;
    info.size = CTEX_SHADER_LAYER_STACK_INFO_CURRENT_SIZE;
    passed = ctex_shader_emit_layer_stack(cache, &fixture.request, &info, NULL, 0, NULL, 0, NULL, 0,
                                          NULL, 0) == CTEX_RESULT_SUCCESS &&
             passed;
    fixture.request.target = CTEX_MATERIAL_GRAPH_TARGET_HLSL;
    info.size = CTEX_SHADER_LAYER_STACK_INFO_CURRENT_SIZE;
    passed = ctex_shader_emit_layer_stack(cache, &fixture.request, &info, NULL, 0, NULL, 0, NULL, 0,
                                          NULL, 0) == CTEX_RESULT_SUCCESS &&
             passed;
    passed =
        ctex_shader_emission_cache_get_info(cache, &cache_info) == CTEX_RESULT_SUCCESS && passed;
    if (passed) {
        passed = expect(
            cache_info.entry_count == 2 && cache_info.hit_count == 1 && cache_info.miss_count == 2,
            "target-specific layer cache accounting was incorrect");
    }
    passed = ctex_shader_emission_cache_clear(cache) == CTEX_RESULT_SUCCESS && passed;
    cache_info.size = CTEX_SHADER_EMISSION_CACHE_INFO_CURRENT_SIZE;
    passed =
        ctex_shader_emission_cache_get_info(cache, &cache_info) == CTEX_RESULT_SUCCESS && passed;
    if (passed) {
        passed = expect(
            cache_info.entry_count == 0 && cache_info.hit_count == 0 && cache_info.miss_count == 0,
            "clearing the emission cache did not reset its state");
    }
    ctex_shader_emission_cache_destroy(cache);
    return passed;
}

static int undersized_layer_output_is_atomic(void) {
    layer_fixture fixture;
    layer_output output = {0};
    uint32_t ignored = 0;
    initialize_fixture(&fixture, 9, CTEX_EXECUTOR_TEXTURE_RGBA8_UNORM,
                       CTEX_MATERIAL_GRAPH_TARGET_WGSL, 1);
    if (!read_layer_stack(NULL, &fixture.request, &output, &ignored)) {
        return 0;
    }
    memset(output.artifacts, 0xa5, output.info.artifact_blob_size);
    memset(output.artifact_report, 0xa5, output.info.artifact_report_size);
    const ctex_result result = ctex_shader_emit_layer_stack(
        NULL, &fixture.request, &output.info, output.artifacts, output.info.artifact_blob_size - 1,
        output.artifact_report, output.info.artifact_report_size, output.pass_plan,
        output.info.pass_plan_size, output.workarounds, output.info.workaround_report_size);
    const int passed =
        expect(result == CTEX_RESULT_BUFFER_TOO_SMALL && output.artifacts[0] == 0xa5 &&
                   (unsigned char)output.artifact_report[0] == 0xa5,
               "undersized layer-stack output partially published data");
    release_layer_output(&output);
    return passed;
}

static int create_graph(graph_blob* graph) {
    ctex_material_graph_info info = {.size = CTEX_MATERIAL_GRAPH_INFO_CURRENT_SIZE};
    if (ctex_material_graph_create_default(&info, NULL, 0, NULL, 0) != CTEX_RESULT_SUCCESS) {
        return 0;
    }
    graph->data = (unsigned char*)malloc(info.canonical_size);
    if (graph->data == NULL ||
        ctex_material_graph_create_default(&info, graph->data, info.canonical_size, NULL, 0) !=
            CTEX_RESULT_SUCCESS) {
        free(graph->data);
        graph->data = NULL;
        return 0;
    }
    graph->size = info.canonical_size;
    return 1;
}

static ctex_shader_material_request material_request(void) {
    static const uint32_t formats[] = {CTEX_EXECUTOR_TEXTURE_RGBA8_UNORM};
    const ctex_shader_material_request result = {
        .size = CTEX_SHADER_MATERIAL_REQUEST_CURRENT_SIZE,
        .stable_identity = "gallery/cached-material",
        .target = CTEX_MATERIAL_GRAPH_TARGET_WGSL,
        .features = {.size = CTEX_SHADER_DEVICE_FEATURES_DESCRIPTOR_CURRENT_SIZE,
                     .binding_budget = 16,
                     .maximum_texture_dimension = 4096,
                     .supported_texture_formats = formats,
                     .supported_texture_format_count = 1,
                     .floating_point_filtering = 1,
                     .compute_available = 0},
        .resources = NULL,
        .resource_count = 0,
        .output = {.size = CTEX_SHADER_TEXTURE_DESCRIPTOR_CURRENT_SIZE,
                   .logical_id = "cached-material-output",
                   .generation = 1,
                   .role = "material output",
                   .format = CTEX_EXECUTOR_TEXTURE_RGBA8_UNORM,
                   .width = 64,
                   .height = 64,
                   .layers = 1,
                   .mip_levels = 1,
                   .tile_width = 16,
                   .tile_height = 16,
                   .externally_initialized = 0},
        .requested_filter = CTEX_SHADER_FILTER_LINEAR,
        .vertex_count = 3,
    };
    return result;
}

static void release_material_output(material_output* output) {
    free(output->vertex);
    free(output->fragment);
    free(output->pass_plan);
    free(output->workarounds);
    memset(output, 0, sizeof(*output));
}

static int read_cached_material(ctex_shader_emission_cache* cache, const graph_blob* graph,
                                const ctex_shader_material_request* request,
                                material_output* output) {
    output->info.size = CTEX_SHADER_MATERIAL_INFO_CURRENT_SIZE;
    if (ctex_shader_emit_material_cached(cache, NULL, graph->data, graph->size, request,
                                         &output->info, NULL, 0, NULL, 0, NULL, 0, NULL, 0,
                                         &output->cache_hit) != CTEX_RESULT_SUCCESS) {
        return 0;
    }
    output->vertex = (unsigned char*)malloc(output->info.vertex_artifact_size);
    output->fragment = (unsigned char*)malloc(output->info.fragment_artifact_size);
    output->pass_plan = (char*)malloc(output->info.pass_plan_size);
    output->workarounds = (char*)malloc(output->info.workaround_report_size);
    if (output->vertex == NULL || output->fragment == NULL || output->pass_plan == NULL ||
        output->workarounds == NULL) {
        release_material_output(output);
        return 0;
    }
    return ctex_shader_emit_material_cached(
               cache, NULL, graph->data, graph->size, request, &output->info, output->vertex,
               output->info.vertex_artifact_size, output->fragment,
               output->info.fragment_artifact_size, output->pass_plan, output->info.pass_plan_size,
               output->workarounds, output->info.workaround_report_size,
               &output->cache_hit) == CTEX_RESULT_SUCCESS;
}

static int material_cache_is_exposed(void) {
    ctex_shader_emission_cache* cache = NULL;
    graph_blob graph = {0};
    material_output first = {0};
    material_output second = {0};
    const ctex_shader_material_request request = material_request();
    int passed = ctex_shader_emission_cache_create(&cache) == CTEX_RESULT_SUCCESS &&
                 create_graph(&graph) && read_cached_material(cache, &graph, &request, &first) &&
                 read_cached_material(cache, &graph, &request, &second);
    if (passed) {
        passed =
            expect(first.cache_hit == 1 && second.cache_hit == 1,
                   "material cache did not report reusable emissions") &&
            expect(first.info.vertex_artifact_size == second.info.vertex_artifact_size &&
                       first.info.fragment_artifact_size == second.info.fragment_artifact_size &&
                       memcmp(first.vertex, second.vertex, first.info.vertex_artifact_size) == 0 &&
                       memcmp(first.fragment, second.fragment, first.info.fragment_artifact_size) ==
                           0 &&
                       strcmp(first.pass_plan, second.pass_plan) == 0,
                   "material cache hit changed shader artifacts or the pass plan");
    }
    release_material_output(&first);
    release_material_output(&second);
    free(graph.data);
    ctex_shader_emission_cache_destroy(cache);
    return passed;
}

int main(void) {
    return one_pass_and_cache_are_exposed() && split_workaround_and_spirv_are_exposed() &&
                   cache_keys_statistics_and_clear_are_exposed() &&
                   undersized_layer_output_is_atomic() && material_cache_is_exposed()
               ? EXIT_SUCCESS
               : EXIT_FAILURE;
}
