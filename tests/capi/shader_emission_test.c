#include <ctex/capi.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct graph_blob {
    unsigned char* data;
    size_t size;
} graph_blob;

typedef struct emission_buffers {
    unsigned char* vertex;
    unsigned char* fragment;
    char* pass_plan;
    char* workarounds;
    ctex_shader_material_info info;
} emission_buffers;

static int expect(int condition, const char* message) {
    if (!condition) {
        fprintf(stderr, "%s\n", message);
    }
    return condition;
}

static void release_graph(graph_blob* graph) {
    free(graph->data);
    graph->data = NULL;
    graph->size = 0;
}

static void release_emission(emission_buffers* emission) {
    free(emission->vertex);
    free(emission->fragment);
    free(emission->pass_plan);
    free(emission->workarounds);
    memset(emission, 0, sizeof(*emission));
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
        release_graph(graph);
        return 0;
    }
    graph->size = info.canonical_size;
    return 1;
}

static int change_base_colour(const graph_blob* source, graph_blob* result) {
    ctex_material_graph_info info = {.size = CTEX_MATERIAL_GRAPH_INFO_CURRENT_SIZE};
    if (ctex_material_graph_inspect(source->data, source->size, &info, NULL, 0, NULL, 0) !=
        CTEX_RESULT_SUCCESS) {
        return 0;
    }
    const ctex_smart_material_value_descriptor colour = {
        .size = CTEX_SMART_MATERIAL_VALUE_DESCRIPTOR_CURRENT_SIZE,
        .type = CTEX_SMART_MATERIAL_VALUE_COLOUR,
        .colour = {0.75F, 0.25F, 0.5F, 1.0F},
    };
    if (ctex_material_graph_set_input_value(source->data, source->size, info.output_node_id,
                                            "pbr.base_color", &colour, &info, NULL,
                                            0) != CTEX_RESULT_SUCCESS) {
        return 0;
    }
    result->data = (unsigned char*)malloc(info.canonical_size);
    if (result->data == NULL ||
        ctex_material_graph_set_input_value(source->data, source->size, info.output_node_id,
                                            "pbr.base_color", &colour, &info, result->data,
                                            info.canonical_size) != CTEX_RESULT_SUCCESS) {
        release_graph(result);
        return 0;
    }
    result->size = info.canonical_size;
    return 1;
}

static ctex_shader_material_request request_for(uint32_t target) {
    static const uint32_t formats[] = {CTEX_EXECUTOR_TEXTURE_RGBA8_UNORM};
    const ctex_shader_device_features_descriptor features = {
        .size = CTEX_SHADER_DEVICE_FEATURES_DESCRIPTOR_CURRENT_SIZE,
        .binding_budget = 16,
        .maximum_texture_dimension = 8192,
        .supported_texture_formats = formats,
        .supported_texture_format_count = 1,
        .floating_point_filtering = 1,
        .compute_available = 0,
    };
    const ctex_shader_texture_descriptor output = {
        .size = CTEX_SHADER_TEXTURE_DESCRIPTOR_CURRENT_SIZE,
        .logical_id = "gallery/material-output",
        .generation = 7,
        .role = "material colour output",
        .format = CTEX_EXECUTOR_TEXTURE_RGBA8_UNORM,
        .width = 64,
        .height = 32,
        .layers = 1,
        .mip_levels = 1,
        .tile_width = 16,
        .tile_height = 16,
        .externally_initialized = 0,
    };
    const ctex_shader_material_request request = {
        .size = CTEX_SHADER_MATERIAL_REQUEST_CURRENT_SIZE,
        .stable_identity = "gallery/material-pass",
        .target = target,
        .features = features,
        .resources = NULL,
        .resource_count = 0,
        .output = output,
        .requested_filter = CTEX_SHADER_FILTER_LINEAR,
        .vertex_count = 3,
    };
    return request;
}

static int allocate_emission(const graph_blob* graph, const ctex_shader_material_request* request,
                             emission_buffers* emission) {
    emission->info.size = CTEX_SHADER_MATERIAL_INFO_CURRENT_SIZE;
    if (ctex_shader_emit_material(NULL, graph->data, graph->size, request, &emission->info, NULL, 0,
                                  NULL, 0, NULL, 0, NULL, 0) != CTEX_RESULT_SUCCESS) {
        return 0;
    }
    emission->vertex = (unsigned char*)malloc(emission->info.vertex_artifact_size);
    emission->fragment = emission->info.fragment_artifact_size == 0
                             ? NULL
                             : (unsigned char*)malloc(emission->info.fragment_artifact_size);
    emission->pass_plan = (char*)malloc(emission->info.pass_plan_size);
    emission->workarounds = (char*)malloc(emission->info.workaround_report_size);
    if (emission->vertex == NULL ||
        (emission->info.fragment_artifact_size != 0 && emission->fragment == NULL) ||
        emission->pass_plan == NULL || emission->workarounds == NULL) {
        release_emission(emission);
        return 0;
    }
    return ctex_shader_emit_material(NULL, graph->data, graph->size, request, &emission->info,
                                     emission->vertex, emission->info.vertex_artifact_size,
                                     emission->fragment, emission->info.fragment_artifact_size,
                                     emission->pass_plan, emission->info.pass_plan_size,
                                     emission->workarounds,
                                     emission->info.workaround_report_size) == CTEX_RESULT_SUCCESS;
}

static int complete_artifacts_for_every_target(const graph_blob* graph) {
    static const uint32_t targets[] = {
        CTEX_MATERIAL_GRAPH_TARGET_WGSL, CTEX_MATERIAL_GRAPH_TARGET_MSL,
        CTEX_MATERIAL_GRAPH_TARGET_SPIRV, CTEX_MATERIAL_GRAPH_TARGET_HLSL};
    int passed = 1;
    for (size_t index = 0; index < sizeof(targets) / sizeof(targets[0]); ++index) {
        const ctex_shader_material_request request = request_for(targets[index]);
        emission_buffers emission = {0};
        if (!allocate_emission(graph, &request, &emission)) {
            return expect(0, "shader emission failed for a declared target");
        }
        passed = expect(emission.info.target == targets[index] && emission.info.pass_count == 1 &&
                            emission.info.logical_resource_count == 1 &&
                            emission.info.vertex_artifact_size > 0,
                        "shader artifact metadata was incomplete") &&
                 passed;
        passed = expect(strstr(emission.pass_plan,
                               "\"stable_identity\":\"gallery/material-pass\"") != NULL &&
                            strstr(emission.pass_plan,
                                   "\"logical_id\":\"gallery/material-output\"") != NULL &&
                            strstr(emission.pass_plan, "\"generation\":7") != NULL &&
                            strstr(emission.pass_plan, "\"accesses\":[") != NULL &&
                            strstr(emission.pass_plan, "\"lifetimes\":[") != NULL &&
                            strstr(emission.pass_plan, "\"format\":\"rgba8_unorm\"") != NULL &&
                            strstr(emission.pass_plan, "\"mode\":\"write\"") != NULL &&
                            strstr(emission.pass_plan, "\"load\":\"clear\"") != NULL &&
                            strstr(emission.pass_plan, "\"kind\":\"render\"") != NULL &&
                            strstr(emission.pass_plan, "\"topology\":\"triangle_list\"") != NULL,
                        "host pass plan omitted identity, resource, access, or lifetime data") &&
                 passed;
        if (targets[index] == CTEX_MATERIAL_GRAPH_TARGET_SPIRV) {
            uint32_t magic = 0;
            memcpy(&magic, emission.vertex, sizeof(magic));
            passed = expect(emission.info.fragment_artifact_size >= sizeof(uint32_t) &&
                                magic == UINT32_C(0x07230203),
                            "SPIR-V emission did not return binary stage modules") &&
                     passed;
        } else {
            passed = expect(emission.vertex[emission.info.vertex_artifact_size - 1] == '\0',
                            "text shader artifact was not NUL terminated") &&
                     passed;
        }
        release_emission(&emission);
    }
    return passed;
}

static int repeated_output_is_identical(const graph_blob* graph) {
    const ctex_shader_material_request request = request_for(CTEX_MATERIAL_GRAPH_TARGET_WGSL);
    emission_buffers first = {0};
    emission_buffers second = {0};
    int passed =
        allocate_emission(graph, &request, &first) && allocate_emission(graph, &request, &second);
    if (passed) {
        passed = expect(
            first.info.vertex_artifact_size == second.info.vertex_artifact_size &&
                first.info.fragment_artifact_size == second.info.fragment_artifact_size &&
                memcmp(first.vertex, second.vertex, first.info.vertex_artifact_size) == 0 &&
                memcmp(first.fragment, second.fragment, first.info.fragment_artifact_size) == 0 &&
                strcmp(first.pass_plan, second.pass_plan) == 0,
            "identical graph emission was not byte-identical");
    }
    release_emission(&first);
    release_emission(&second);
    return passed;
}

static int value_edits_preserve_the_pass_plan(const graph_blob* graph) {
    const ctex_shader_material_request request = request_for(CTEX_MATERIAL_GRAPH_TARGET_WGSL);
    graph_blob changed = {0};
    emission_buffers first = {0};
    emission_buffers second = {0};
    int passed = change_base_colour(graph, &changed) &&
                 allocate_emission(graph, &request, &first) &&
                 allocate_emission(&changed, &request, &second);
    if (passed) {
        passed = expect(strcmp(first.pass_plan, second.pass_plan) == 0 &&
                            strcmp((const char*)first.fragment, (const char*)second.fragment) != 0,
                        "a constant edit changed the pass plan or failed to change source");
    }
    release_emission(&first);
    release_emission(&second);
    release_graph(&changed);
    return passed;
}

static int undersized_output_is_atomic(const graph_blob* graph) {
    const ctex_shader_material_request request = request_for(CTEX_MATERIAL_GRAPH_TARGET_WGSL);
    emission_buffers emission = {0};
    if (!allocate_emission(graph, &request, &emission)) {
        return 0;
    }
    memset(emission.vertex, 0xa5, emission.info.vertex_artifact_size);
    memset(emission.pass_plan, 0xa5, emission.info.pass_plan_size);
    const ctex_result result = ctex_shader_emit_material(
        NULL, graph->data, graph->size, &request, &emission.info, emission.vertex,
        emission.info.vertex_artifact_size - 1, emission.fragment,
        emission.info.fragment_artifact_size, emission.pass_plan, emission.info.pass_plan_size,
        emission.workarounds, emission.info.workaround_report_size);
    const int passed =
        expect(result == CTEX_RESULT_BUFFER_TOO_SMALL && emission.vertex[0] == 0xa5 &&
                   (unsigned char)emission.pass_plan[0] == 0xa5,
               "undersized shader output partially published results");
    release_emission(&emission);
    return passed;
}

static int unsupported_target_names_the_inventory(const graph_blob* graph) {
    const ctex_shader_material_request request = request_for(99);
    ctex_shader_material_info info = {.size = CTEX_SHADER_MATERIAL_INFO_CURRENT_SIZE};
    const ctex_result result = ctex_shader_emit_material(NULL, graph->data, graph->size, &request,
                                                         &info, NULL, 0, NULL, 0, NULL, 0, NULL, 0);
    const char* diagnostic = ctex_get_last_diagnostic();
    return expect(result == CTEX_RESULT_INVALID_ARGUMENT && diagnostic != NULL &&
                      strstr(diagnostic, "unknown(99)") != NULL &&
                      strstr(diagnostic, "WGSL, MSL, SPIR-V, HLSL") != NULL,
                  "unsupported target diagnostic omitted the request or available targets");
}

int main(void) {
    graph_blob graph = {0};
    if (!expect(create_graph(&graph), "could not create the material graph fixture")) {
        return EXIT_FAILURE;
    }
    const int passed =
        complete_artifacts_for_every_target(&graph) && repeated_output_is_identical(&graph) &&
        value_edits_preserve_the_pass_plan(&graph) && undersized_output_is_atomic(&graph) &&
        unsupported_target_names_the_inventory(&graph);
    release_graph(&graph);
    return passed ? EXIT_SUCCESS : EXIT_FAILURE;
}
