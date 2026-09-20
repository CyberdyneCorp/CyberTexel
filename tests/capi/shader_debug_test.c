#include <ctex/capi.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct graph_blob {
    unsigned char* data;
    size_t size;
} graph_blob;

typedef struct inspectable_output {
    unsigned char* vertex;
    unsigned char* fragment;
    char* pass_plan;
    char* workarounds;
    char* debug_metadata;
    ctex_shader_material_info info;
    ctex_shader_material_debug_info debug_info;
    uint32_t cache_hit;
} inspectable_output;

static int expect(int condition, const char* message) {
    if (!condition) {
        fprintf(stderr, "%s\n", message);
    }
    return condition;
}

static size_t occurrences(const char* text, const char* sought) {
    size_t count = 0;
    const size_t sought_size = strlen(sought);
    const char* cursor = text;
    while ((cursor = strstr(cursor, sought)) != NULL) {
        ++count;
        cursor += sought_size;
    }
    return count;
}

static void release_graph(graph_blob* graph) {
    free(graph->data);
    graph->data = NULL;
    graph->size = 0;
}

static int create_default_graph(graph_blob* graph, ctex_material_graph_info* out_info) {
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
    *out_info = info;
    return 1;
}

static int add_node(const graph_blob* source, const char* type_id, graph_blob* result,
                    uint64_t* out_node_id) {
    ctex_material_graph_info info = {.size = CTEX_MATERIAL_GRAPH_INFO_CURRENT_SIZE};
    uint64_t node_id = 0;
    if (ctex_material_graph_add_builtin_node(source->data, source->size, type_id, (ctex_vec2f){0},
                                             &info, &node_id, NULL, 0) != CTEX_RESULT_SUCCESS) {
        return 0;
    }
    result->data = (unsigned char*)malloc(info.canonical_size);
    if (result->data == NULL ||
        ctex_material_graph_add_builtin_node(source->data, source->size, type_id, (ctex_vec2f){0},
                                             &info, &node_id, result->data,
                                             info.canonical_size) != CTEX_RESULT_SUCCESS) {
        release_graph(result);
        return 0;
    }
    result->size = info.canonical_size;
    *out_node_id = node_id;
    return 1;
}

static int add_link(const graph_blob* source, uint64_t source_node, const char* source_socket,
                    uint64_t target_node, const char* target_socket, graph_blob* result) {
    const ctex_material_graph_link_descriptor link = {
        .size = CTEX_MATERIAL_GRAPH_LINK_DESCRIPTOR_CURRENT_SIZE,
        .source_node = source_node,
        .source_socket = source_socket,
        .target_node = target_node,
        .target_socket = target_socket,
    };
    ctex_material_graph_link_info info = {.size = CTEX_MATERIAL_GRAPH_LINK_INFO_CURRENT_SIZE};
    if (ctex_material_graph_add_link(source->data, source->size, &link, &info, NULL, 0, NULL, 0) !=
        CTEX_RESULT_SUCCESS) {
        return 0;
    }
    result->data = (unsigned char*)malloc(info.graph.canonical_size);
    if (result->data == NULL ||
        ctex_material_graph_add_link(source->data, source->size, &link, &info, result->data,
                                     info.graph.canonical_size, NULL, 0) != CTEX_RESULT_SUCCESS) {
        release_graph(result);
        return 0;
    }
    result->size = info.graph.canonical_size;
    return 1;
}

static int create_fanout_graph(graph_blob* result, uint64_t* out_node_id) {
    graph_blob base = {0};
    graph_blob node_graph = {0};
    graph_blob roughness = {0};
    graph_blob metallic = {0};
    ctex_material_graph_info info;
    int passed =
        create_default_graph(&base, &info) &&
        add_node(&base, "ctex.input.constant-value", &node_graph, out_node_id) &&
        add_link(&node_graph, *out_node_id, "value", info.output_node_id, "pbr.roughness",
                 &roughness) &&
        add_link(&roughness, *out_node_id, "value", info.output_node_id, "pbr.metallic",
                 &metallic) &&
        add_link(&metallic, *out_node_id, "value", info.output_node_id, "pbr.opacity", result);
    release_graph(&metallic);
    release_graph(&roughness);
    release_graph(&node_graph);
    release_graph(&base);
    return passed;
}

static ctex_shader_material_request request_for(uint32_t target) {
    static const uint32_t formats[] = {CTEX_EXECUTOR_TEXTURE_RGBA8_UNORM};
    const ctex_shader_material_request result = {
        .size = CTEX_SHADER_MATERIAL_REQUEST_CURRENT_SIZE,
        .stable_identity = "gallery/inspectable-material",
        .target = target,
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
                   .logical_id = "inspectable-output",
                   .generation = 1,
                   .role = "inspectable material output",
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

static void release_output(inspectable_output* output) {
    free(output->vertex);
    free(output->fragment);
    free(output->pass_plan);
    free(output->workarounds);
    free(output->debug_metadata);
    memset(output, 0, sizeof(*output));
}

static int read_output(const ctex_shader_material_source_descriptor* source,
                       const ctex_shader_material_request* request, inspectable_output* output) {
    output->info.size = CTEX_SHADER_MATERIAL_INFO_CURRENT_SIZE;
    output->debug_info.size = CTEX_SHADER_MATERIAL_DEBUG_INFO_CURRENT_SIZE;
    if (ctex_shader_emit_material_inspectable(NULL, NULL, source, request, &output->info, NULL, 0,
                                              NULL, 0, NULL, 0, NULL, 0, &output->debug_info, NULL,
                                              0, &output->cache_hit) != CTEX_RESULT_SUCCESS) {
        return 0;
    }
    output->vertex = (unsigned char*)malloc(output->info.vertex_artifact_size);
    output->fragment = output->info.fragment_artifact_size == 0
                           ? NULL
                           : (unsigned char*)malloc(output->info.fragment_artifact_size);
    output->pass_plan = (char*)malloc(output->info.pass_plan_size);
    output->workarounds = (char*)malloc(output->info.workaround_report_size);
    output->debug_metadata = (char*)malloc(output->debug_info.metadata_size);
    if (output->vertex == NULL ||
        (output->info.fragment_artifact_size != 0 && output->fragment == NULL) ||
        output->pass_plan == NULL || output->workarounds == NULL ||
        output->debug_metadata == NULL) {
        release_output(output);
        return 0;
    }
    return ctex_shader_emit_material_inspectable(
               NULL, NULL, source, request, &output->info, output->vertex,
               output->info.vertex_artifact_size, output->fragment,
               output->info.fragment_artifact_size, output->pass_plan, output->info.pass_plan_size,
               output->workarounds, output->info.workaround_report_size, &output->debug_info,
               output->debug_metadata, output->debug_info.metadata_size,
               &output->cache_hit) == CTEX_RESULT_SUCCESS;
}

static int serialized_fanout_is_named_once_and_inspectable(void) {
    graph_blob graph = {0};
    uint64_t node_id = 0;
    if (!create_fanout_graph(&graph, &node_id)) {
        return 0;
    }
    const ctex_shader_material_source_descriptor source = {
        .size = CTEX_SHADER_MATERIAL_SOURCE_DESCRIPTOR_CURRENT_SIZE,
        .kind = CTEX_SHADER_MATERIAL_SOURCE_SERIALIZED_GRAPH,
        .graph_serialized = graph.data,
        .graph_serialized_size = graph.size,
        .workspace = NULL,
        .material_identifier = NULL,
    };
    const ctex_shader_material_request request = request_for(CTEX_MATERIAL_GRAPH_TARGET_WGSL);
    inspectable_output output = {0};
    int passed = read_output(&source, &request, &output);
    char variable[64];
    (void)snprintf(variable, sizeof(variable), "ctex_n%llu_s76616c7565",
                   (unsigned long long)node_id);
    if (passed) {
        passed = expect(output.debug_info.node_attribution_count == 1 &&
                            output.debug_info.binary_companion == 0 &&
                            occurrences((const char*)output.fragment, variable) == 4 &&
                            strstr((const char*)output.fragment,
                                   "// node ctex.input.constant-value") != NULL,
                        "fan-out source was duplicated or lost node attribution") &&
                 expect(strstr(output.debug_metadata, variable) != NULL &&
                            strstr(output.debug_metadata,
                                   "\"node_path\":\"ctex.input.constant-value[") != NULL &&
                            strstr(output.debug_metadata, "\"output_socket\":\"value\"") != NULL,
                        "structured debug metadata omitted the emitted node mapping");
    }
    if (passed) {
        memset(output.vertex, 0xa5, output.info.vertex_artifact_size);
        const ctex_result short_result = ctex_shader_emit_material_inspectable(
            NULL, NULL, &source, &request, &output.info, output.vertex,
            output.info.vertex_artifact_size, output.fragment, output.info.fragment_artifact_size,
            output.pass_plan, output.info.pass_plan_size, output.workarounds,
            output.info.workaround_report_size, &output.debug_info, output.debug_metadata,
            output.debug_info.metadata_size - 1, &output.cache_hit);
        passed = expect(short_result == CTEX_RESULT_BUFFER_TOO_SMALL && output.vertex[0] == 0xa5,
                        "short debug metadata output partially published shader artifacts");
    }
    release_output(&output);
    release_graph(&graph);
    return passed;
}

static int spirv_has_binary_companion_metadata(void) {
    graph_blob graph = {0};
    uint64_t node_id = 0;
    if (!create_fanout_graph(&graph, &node_id)) {
        return 0;
    }
    const ctex_shader_material_source_descriptor source = {
        .size = CTEX_SHADER_MATERIAL_SOURCE_DESCRIPTOR_CURRENT_SIZE,
        .kind = CTEX_SHADER_MATERIAL_SOURCE_SERIALIZED_GRAPH,
        .graph_serialized = graph.data,
        .graph_serialized_size = graph.size,
    };
    const ctex_shader_material_request request = request_for(CTEX_MATERIAL_GRAPH_TARGET_SPIRV);
    inspectable_output output = {0};
    int passed = read_output(&source, &request, &output);
    if (passed) {
        uint32_t magic = 0;
        memcpy(&magic, output.vertex, sizeof(magic));
        passed = expect(magic == UINT32_C(0x07230203) && output.debug_info.binary_companion == 1 &&
                            strstr(output.debug_metadata, "\"target\":\"SPIR-V\"") != NULL &&
                            strstr(output.debug_metadata, "\"binary_companion\":true") != NULL,
                        "SPIR-V attribution was absent or represented as source text");
    }
    release_output(&output);
    release_graph(&graph);
    return passed;
}

static ctex_material_graph_socket_descriptor scalar_output(void) {
    const ctex_material_graph_socket_descriptor result = {
        .size = CTEX_MATERIAL_GRAPH_SOCKET_DESCRIPTOR_CURRENT_SIZE,
        .identifier = "value",
        .display_name = "Value",
        .type = CTEX_SMART_MATERIAL_VALUE_SCALAR,
        .default_value = NULL,
    };
    return result;
}

static int create_constant_group(ctex_material_graph_workspace* workspace, const char* identifier,
                                 uint64_t* out_constant_id) {
    const ctex_material_graph_socket_descriptor output = scalar_output();
    const ctex_material_graph_group_descriptor group = {
        .size = CTEX_MATERIAL_GRAPH_GROUP_DESCRIPTOR_CURRENT_SIZE,
        .identifier = identifier,
        .display_name = identifier,
        .inputs = NULL,
        .input_count = 0,
        .outputs = &output,
        .output_count = 1,
    };
    if (ctex_material_graph_workspace_create_group(workspace, &group) != CTEX_RESULT_SUCCESS ||
        ctex_material_graph_workspace_add_builtin_node(
            workspace, CTEX_MATERIAL_GRAPH_OWNER_GROUP, identifier, "ctex.input.constant-value",
            (ctex_vec2f){0}, out_constant_id) != CTEX_RESULT_SUCCESS) {
        return 0;
    }
    ctex_material_graph_info group_info = {.size = CTEX_MATERIAL_GRAPH_INFO_CURRENT_SIZE};
    if (ctex_material_graph_workspace_get_graph(workspace, CTEX_MATERIAL_GRAPH_OWNER_GROUP,
                                                identifier, &group_info, NULL, 0, NULL,
                                                0) != CTEX_RESULT_SUCCESS) {
        return 0;
    }
    const ctex_material_graph_link_descriptor link = {
        .size = CTEX_MATERIAL_GRAPH_LINK_DESCRIPTOR_CURRENT_SIZE,
        .source_node = *out_constant_id,
        .source_socket = "value",
        .target_node = group_info.output_node_id,
        .target_socket = "value",
    };
    ctex_material_graph_link_info link_info = {.size = CTEX_MATERIAL_GRAPH_LINK_INFO_CURRENT_SIZE};
    return ctex_material_graph_workspace_add_link(workspace, CTEX_MATERIAL_GRAPH_OWNER_GROUP,
                                                  identifier, &link, &link_info, NULL,
                                                  0) == CTEX_RESULT_SUCCESS;
}

static int link_workspace_node(ctex_material_graph_workspace* workspace, uint64_t source_node,
                               uint64_t output_node, const char* target_socket) {
    const ctex_material_graph_link_descriptor link = {
        .size = CTEX_MATERIAL_GRAPH_LINK_DESCRIPTOR_CURRENT_SIZE,
        .source_node = source_node,
        .source_socket = "value",
        .target_node = output_node,
        .target_socket = target_socket,
    };
    ctex_material_graph_link_info info = {.size = CTEX_MATERIAL_GRAPH_LINK_INFO_CURRENT_SIZE};
    return ctex_material_graph_workspace_add_link(workspace, CTEX_MATERIAL_GRAPH_OWNER_MATERIAL,
                                                  "fixture", &link, &info, NULL,
                                                  0) == CTEX_RESULT_SUCCESS;
}

static int group_paths_prevent_result_name_collisions(void) {
    graph_blob graph = {0};
    ctex_material_graph_info graph_info;
    ctex_material_graph_workspace* workspace = NULL;
    uint64_t left_constant = 0;
    uint64_t right_constant = 0;
    uint64_t left_instance = 0;
    uint64_t right_instance = 0;
    int passed =
        create_default_graph(&graph, &graph_info) &&
        ctex_material_graph_workspace_create(&workspace) == CTEX_RESULT_SUCCESS &&
        ctex_material_graph_workspace_add_material(workspace, "fixture", graph.data, graph.size) ==
            CTEX_RESULT_SUCCESS &&
        create_constant_group(workspace, "left", &left_constant) &&
        create_constant_group(workspace, "right", &right_constant) &&
        ctex_material_graph_workspace_instantiate_group(
            workspace, "left", CTEX_MATERIAL_GRAPH_OWNER_MATERIAL, "fixture", (ctex_vec2f){0},
            &left_instance) == CTEX_RESULT_SUCCESS &&
        ctex_material_graph_workspace_instantiate_group(
            workspace, "right", CTEX_MATERIAL_GRAPH_OWNER_MATERIAL, "fixture", (ctex_vec2f){0},
            &right_instance) == CTEX_RESULT_SUCCESS &&
        link_workspace_node(workspace, left_instance, graph_info.output_node_id, "pbr.roughness") &&
        link_workspace_node(workspace, right_instance, graph_info.output_node_id, "pbr.metallic");
    const ctex_shader_material_source_descriptor source = {
        .size = CTEX_SHADER_MATERIAL_SOURCE_DESCRIPTOR_CURRENT_SIZE,
        .kind = CTEX_SHADER_MATERIAL_SOURCE_WORKSPACE,
        .workspace = workspace,
        .material_identifier = "fixture",
    };
    const ctex_shader_material_request request = request_for(CTEX_MATERIAL_GRAPH_TARGET_WGSL);
    inspectable_output output = {0};
    passed = passed && read_output(&source, &request, &output);
    if (passed) {
        char left_name[128];
        char right_name[128];
        (void)snprintf(left_name, sizeof(left_name), "ctex_g6c656674_i%llu_n%llu_s76616c7565",
                       (unsigned long long)left_instance, (unsigned long long)left_constant);
        (void)snprintf(right_name, sizeof(right_name), "ctex_g7269676874_i%llu_n%llu_s76616c7565",
                       (unsigned long long)right_instance, (unsigned long long)right_constant);
        passed =
            expect(strcmp(left_name, right_name) != 0 &&
                       strstr((const char*)output.fragment, left_name) != NULL &&
                       strstr((const char*)output.fragment, right_name) != NULL,
                   "group-qualified result variables collided or were unavailable through C") &&
            expect(strstr(output.debug_metadata, "left[") != NULL &&
                       strstr(output.debug_metadata, "right[") != NULL &&
                       strstr(output.debug_metadata, "ctex.input.constant-value") != NULL,
                   "workspace debug metadata omitted qualified group paths");
    }
    release_output(&output);
    ctex_material_graph_workspace_destroy(workspace);
    release_graph(&graph);
    return passed;
}

static int backend_attribution_is_public(void) {
    ctex_shader_backend_attribution_info info = {
        .size = CTEX_SHADER_BACKEND_ATTRIBUTION_INFO_CURRENT_SIZE};
    if (ctex_shader_get_backend_attribution(&info, NULL, 0) != CTEX_RESULT_SUCCESS) {
        return 0;
    }
    char* report = (char*)malloc(info.report_size);
    const int passed =
        report != NULL &&
        ctex_shader_get_backend_attribution(&info, report, info.report_size) ==
            CTEX_RESULT_SUCCESS &&
        expect(strstr(report, "\"name\":\"Kongruent minikong\"") != NULL &&
                   strstr(report, "\"license\":\"Zlib\"") != NULL &&
                   strstr(report, "c5ccdf27818a36e67decb691009a3db457d59ab3") != NULL &&
                   strstr(report, "1b0f3b70673122e3b20701cdb434781a065555d7") != NULL &&
                   strstr(report, "thirdparty/licenses/kongruent.txt") != NULL,
               "public shader backend attribution omitted its licence or pinned revisions");
    free(report);
    return passed;
}

int main(void) {
    return serialized_fanout_is_named_once_and_inspectable() &&
                   spirv_has_binary_companion_metadata() &&
                   group_paths_prevent_result_name_collisions() && backend_attribution_is_public()
               ? EXIT_SUCCESS
               : EXIT_FAILURE;
}
