#include <ctex/capi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct graph_blob {
    unsigned char* data;
    size_t size;
} graph_blob;

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

static int create_default_graph(graph_blob* graph) {
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

static int get_graph(ctex_material_graph_workspace* workspace, uint32_t owner_kind,
                     const char* owner_identifier, graph_blob* graph, char** report) {
    ctex_material_graph_info info = {.size = CTEX_MATERIAL_GRAPH_INFO_CURRENT_SIZE};
    if (ctex_material_graph_workspace_get_graph(workspace, owner_kind, owner_identifier, &info,
                                                NULL, 0, NULL, 0) != CTEX_RESULT_SUCCESS) {
        return 0;
    }
    graph->data = (unsigned char*)malloc(info.canonical_size);
    *report = (char*)malloc(info.report_size);
    if (graph->data == NULL || *report == NULL ||
        ctex_material_graph_workspace_get_graph(workspace, owner_kind, owner_identifier, &info,
                                                graph->data, info.canonical_size, *report,
                                                info.report_size) != CTEX_RESULT_SUCCESS) {
        release_graph(graph);
        free(*report);
        *report = NULL;
        return 0;
    }
    graph->size = info.canonical_size;
    return 1;
}

static ctex_material_graph_socket_descriptor scalar_socket(
    const char* identifier, const ctex_smart_material_value_descriptor* default_value) {
    const ctex_material_graph_socket_descriptor result = {
        .size = CTEX_MATERIAL_GRAPH_SOCKET_DESCRIPTOR_CURRENT_SIZE,
        .identifier = identifier,
        .display_name = identifier,
        .type = CTEX_SMART_MATERIAL_VALUE_SCALAR,
        .default_value = default_value,
    };
    return result;
}

static int propagation_preserves_values_and_subgraph_edits(void) {
    graph_blob base = {0};
    ctex_material_graph_workspace* workspace = NULL;
    if (!create_default_graph(&base) ||
        ctex_material_graph_workspace_create(&workspace) != CTEX_RESULT_SUCCESS) {
        release_graph(&base);
        return 0;
    }

    const char* material_ids[] = {"oak", "pine", "walnut"};
    int passed = 1;
    for (size_t index = 0; index < 3; ++index) {
        passed =
            expect(ctex_material_graph_workspace_add_material(
                       workspace, material_ids[index], base.data, base.size) == CTEX_RESULT_SUCCESS,
                   "material graph was not added to the workspace") &&
            passed;
    }

    const ctex_smart_material_value_descriptor factor_default = {
        .size = CTEX_SMART_MATERIAL_VALUE_DESCRIPTOR_CURRENT_SIZE,
        .type = CTEX_SMART_MATERIAL_VALUE_SCALAR,
        .scalar = 0.25,
    };
    const ctex_material_graph_socket_descriptor input = scalar_socket("factor", &factor_default);
    const ctex_material_graph_socket_descriptor output = scalar_socket("value", NULL);
    const ctex_material_graph_group_descriptor group = {
        .size = CTEX_MATERIAL_GRAPH_GROUP_DESCRIPTOR_CURRENT_SIZE,
        .identifier = "weathering",
        .display_name = "Weathering",
        .inputs = &input,
        .input_count = 1,
        .outputs = &output,
        .output_count = 1,
    };
    passed =
        expect(ctex_material_graph_workspace_create_group(workspace, &group) == CTEX_RESULT_SUCCESS,
               "node group creation failed") &&
        passed;

    uint64_t instance_ids[3] = {0};
    const double stored_values[] = {0.1, 0.2, 0.3};
    for (size_t index = 0; index < 3; ++index) {
        ctex_smart_material_value_descriptor value = {
            .size = CTEX_SMART_MATERIAL_VALUE_DESCRIPTOR_CURRENT_SIZE,
            .type = CTEX_SMART_MATERIAL_VALUE_SCALAR,
            .scalar = stored_values[index],
        };
        passed = expect(ctex_material_graph_workspace_instantiate_group(
                            workspace, "weathering", CTEX_MATERIAL_GRAPH_OWNER_MATERIAL,
                            material_ids[index], (ctex_vec2f){0},
                            &instance_ids[index]) == CTEX_RESULT_SUCCESS,
                        "material group instance creation failed") &&
                 passed;
        passed = expect(ctex_material_graph_workspace_set_input_value(
                            workspace, CTEX_MATERIAL_GRAPH_OWNER_MATERIAL, material_ids[index],
                            instance_ids[index], "factor", &value) == CTEX_RESULT_SUCCESS,
                        "material group instance value could not be set") &&
                 passed;
    }

    uint64_t constant_id = 0;
    passed = expect(ctex_material_graph_workspace_add_builtin_node(
                        workspace, CTEX_MATERIAL_GRAPH_OWNER_GROUP, "weathering",
                        "ctex.input.constant-value", (ctex_vec2f){0},
                        &constant_id) == CTEX_RESULT_SUCCESS,
                    "group subgraph could not be edited") &&
             passed;
    const ctex_material_graph_link_descriptor link = {
        .size = CTEX_MATERIAL_GRAPH_LINK_DESCRIPTOR_CURRENT_SIZE,
        .source_node = constant_id,
        .source_socket = "value",
        .target_node = 1,
        .target_socket = "value",
    };
    ctex_material_graph_link_info link_info = {.size = CTEX_MATERIAL_GRAPH_LINK_INFO_CURRENT_SIZE};
    passed = expect(ctex_material_graph_workspace_add_link(
                        workspace, CTEX_MATERIAL_GRAPH_OWNER_GROUP, "weathering", &link, &link_info,
                        NULL, 0) == CTEX_RESULT_SUCCESS,
                    "group subgraph link could not be added") &&
             passed;

    const ctex_smart_material_value_descriptor detail_default = {
        .size = CTEX_SMART_MATERIAL_VALUE_DESCRIPTOR_CURRENT_SIZE,
        .type = CTEX_SMART_MATERIAL_VALUE_SCALAR,
        .scalar = 0.75,
    };
    const ctex_smart_material_value_descriptor changed_factor_default = {
        .size = CTEX_SMART_MATERIAL_VALUE_DESCRIPTOR_CURRENT_SIZE,
        .type = CTEX_SMART_MATERIAL_VALUE_SCALAR,
        .scalar = 0.5,
    };
    const ctex_material_graph_socket_descriptor changed_inputs[] = {
        scalar_socket("detail", &detail_default),
        scalar_socket("factor", &changed_factor_default),
    };
    const ctex_material_graph_group_interface_descriptor changed = {
        .size = CTEX_MATERIAL_GRAPH_GROUP_INTERFACE_DESCRIPTOR_CURRENT_SIZE,
        .inputs = changed_inputs,
        .input_count = 2,
        .outputs = &output,
        .output_count = 1,
    };
    ctex_material_graph_group_update_info update = {
        .size = CTEX_MATERIAL_GRAPH_GROUP_UPDATE_INFO_CURRENT_SIZE};
    passed = expect(ctex_material_graph_workspace_update_group_interface(
                        workspace, "weathering", &changed, &update) == CTEX_RESULT_SUCCESS &&
                        update.group_version == 2 && update.instances_updated == 3,
                    "group interface edit did not propagate to all instances") &&
             passed;

    for (size_t index = 0; index < 3; ++index) {
        graph_blob material = {0};
        char* report = NULL;
        char stored[64];
        snprintf(stored, sizeof(stored), "\"default\":%.6f", stored_values[index]);
        passed = expect(get_graph(workspace, CTEX_MATERIAL_GRAPH_OWNER_MATERIAL,
                                  material_ids[index], &material, &report) &&
                            strstr(report, "\"id\":\"detail\"") != NULL &&
                            strstr(report, "\"default\":0.750000") != NULL &&
                            strstr(report, stored) != NULL,
                        "propagation did not preserve the stored instance value") &&
                 passed;
        release_graph(&material);
        free(report);
    }

    ctex_material_graph_workspace_info workspace_info = {
        .size = CTEX_MATERIAL_GRAPH_WORKSPACE_INFO_CURRENT_SIZE};
    passed = expect(ctex_material_graph_workspace_get_info(workspace, &workspace_info) ==
                            CTEX_RESULT_SUCCESS &&
                        workspace_info.material_count == 3 && workspace_info.group_count == 1,
                    "workspace inventory is incorrect") &&
             passed;
    ctex_material_graph_workspace_destroy(workspace);
    release_graph(&base);
    return passed;
}

static int recursive_placement_is_atomic(void) {
    ctex_material_graph_workspace* workspace = NULL;
    if (ctex_material_graph_workspace_create(&workspace) != CTEX_RESULT_SUCCESS) {
        return 0;
    }
    const char* group_ids[] = {"a", "b", "c"};
    int passed = 1;
    for (size_t index = 0; index < 3; ++index) {
        const ctex_material_graph_group_descriptor group = {
            .size = CTEX_MATERIAL_GRAPH_GROUP_DESCRIPTOR_CURRENT_SIZE,
            .identifier = group_ids[index],
            .display_name = group_ids[index],
        };
        passed = expect(ctex_material_graph_workspace_create_group(workspace, &group) ==
                            CTEX_RESULT_SUCCESS,
                        "recursion fixture group creation failed") &&
                 passed;
    }
    uint64_t node_id = 0;
    passed = expect(ctex_material_graph_workspace_instantiate_group(
                        workspace, "a", CTEX_MATERIAL_GRAPH_OWNER_GROUP, "a", (ctex_vec2f){0},
                        &node_id) == CTEX_RESULT_INVALID_ARGUMENT &&
                        strstr(ctex_get_last_diagnostic(), "a -> a") != NULL,
                    "self-recursive group placement was not refused") &&
             passed;
    passed = expect(ctex_material_graph_workspace_instantiate_group(
                        workspace, "b", CTEX_MATERIAL_GRAPH_OWNER_GROUP, "a", (ctex_vec2f){0},
                        &node_id) == CTEX_RESULT_SUCCESS,
                    "first nested group placement failed") &&
             passed;
    passed = expect(ctex_material_graph_workspace_instantiate_group(
                        workspace, "c", CTEX_MATERIAL_GRAPH_OWNER_GROUP, "b", (ctex_vec2f){0},
                        &node_id) == CTEX_RESULT_SUCCESS,
                    "second nested group placement failed") &&
             passed;

    graph_blob before = {0};
    graph_blob after = {0};
    char* report = NULL;
    passed = expect(get_graph(workspace, CTEX_MATERIAL_GRAPH_OWNER_GROUP, "c", &before, &report),
                    "group could not be read before recursive placement") &&
             passed;
    free(report);
    report = NULL;
    const ctex_result recursion = ctex_material_graph_workspace_instantiate_group(
        workspace, "a", CTEX_MATERIAL_GRAPH_OWNER_GROUP, "c", (ctex_vec2f){0}, &node_id);
    passed = expect(recursion == CTEX_RESULT_INVALID_ARGUMENT &&
                        ctex_get_last_diagnostic_code() == CTEX_DIAGNOSTIC_INVALID_MATERIAL_GRAPH &&
                        strstr(ctex_get_last_diagnostic(), "c -> a -> b -> c") != NULL,
                    "recursive placement did not name the complete cycle") &&
             passed;
    passed = expect(get_graph(workspace, CTEX_MATERIAL_GRAPH_OWNER_GROUP, "c", &after, &report),
                    "group could not be read after recursive placement") &&
             passed;
    uint32_t equal = 0;
    passed = expect(ctex_material_graph_compare(before.data, before.size, after.data, after.size,
                                                &equal) == CTEX_RESULT_SUCCESS &&
                        equal == 1,
                    "recursive placement changed the containing group") &&
             passed;

    free(report);
    release_graph(&before);
    release_graph(&after);
    ctex_material_graph_workspace_destroy(workspace);
    return passed;
}

int main(void) {
    return propagation_preserves_values_and_subgraph_edits() && recursive_placement_is_atomic() ? 0
                                                                                                : 1;
}
