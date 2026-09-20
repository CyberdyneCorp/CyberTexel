#include <ctex/capi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct graph_blob {
    unsigned char* data;
    size_t size;
} graph_blob;

typedef struct callback_capture {
    size_t cpu_calls;
    size_t emission_calls;
} callback_capture;

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

static const ctex_material_graph_host_property_value* find_property(
    const ctex_material_graph_host_evaluation_request* request, const char* identifier) {
    for (size_t index = 0; index < request->property_count; ++index) {
        if (strcmp(request->properties[index].identifier, identifier) == 0) {
            return &request->properties[index];
        }
    }
    return NULL;
}

static ctex_result evaluate_scale(const ctex_material_graph_host_evaluation_request* request,
                                  ctex_smart_material_value_descriptor* outputs,
                                  size_t output_count, void* user_data) {
    callback_capture* capture = (callback_capture*)user_data;
    const ctex_material_graph_host_property_value* factor = find_property(request, "factor");
    ++capture->cpu_calls;
    if (request->size != CTEX_MATERIAL_GRAPH_HOST_EVALUATION_REQUEST_CURRENT_SIZE ||
        request->input_count != 1 || output_count != 1 || factor == NULL ||
        request->inputs[0].type != CTEX_SMART_MATERIAL_VALUE_SCALAR ||
        factor->value.type != CTEX_SMART_MATERIAL_VALUE_SCALAR) {
        return CTEX_RESULT_INVALID_ARGUMENT;
    }
    outputs[0].type = CTEX_SMART_MATERIAL_VALUE_SCALAR;
    outputs[0].scalar = request->inputs[0].scalar * factor->value.scalar;
    return CTEX_RESULT_SUCCESS;
}

static ctex_result emit_scale(const ctex_material_graph_host_emission_request* request,
                              ctex_material_graph_host_emission_result* out_result,
                              void* user_data) {
    static const char* output_expressions[] = {"(fixture_input * 3.0)"};
    static const char* resources[] = {"textures/source.png"};
    callback_capture* capture = (callback_capture*)user_data;
    ++capture->emission_calls;
    if (request->size != CTEX_MATERIAL_GRAPH_HOST_EMISSION_REQUEST_CURRENT_SIZE ||
        request->target != CTEX_MATERIAL_GRAPH_TARGET_WGSL ||
        request->input_expression_count != 1 || request->property_count != 2 ||
        out_result->size != CTEX_MATERIAL_GRAPH_HOST_EMISSION_RESULT_CURRENT_SIZE) {
        return CTEX_RESULT_INVALID_ARGUMENT;
    }
    out_result->output_expressions = output_expressions;
    out_result->output_expression_count = 1;
    out_result->resource_identifiers = resources;
    out_result->resource_identifier_count = 1;
    return CTEX_RESULT_SUCCESS;
}

static ctex_material_graph_host_node_registration_descriptor scale_registration(
    uint32_t version, uint32_t deterministic, callback_capture* capture,
    ctex_material_graph_cpu_evaluate_callback cpu_callback) {
    static const ctex_smart_material_value_descriptor input_default = {
        .size = CTEX_SMART_MATERIAL_VALUE_DESCRIPTOR_CURRENT_SIZE,
        .type = CTEX_SMART_MATERIAL_VALUE_SCALAR,
        .scalar = 1.0,
    };
    static const ctex_smart_material_value_descriptor factor_default = {
        .size = CTEX_SMART_MATERIAL_VALUE_DESCRIPTOR_CURRENT_SIZE,
        .type = CTEX_SMART_MATERIAL_VALUE_SCALAR,
        .scalar = 3.0,
    };
    static const ctex_smart_material_value_descriptor image_default = {
        .size = CTEX_SMART_MATERIAL_VALUE_DESCRIPTOR_CURRENT_SIZE,
        .type = CTEX_SMART_MATERIAL_VALUE_IMAGE,
        .text = "textures/source.png",
    };
    static const ctex_material_graph_socket_descriptor inputs[] = {{
        .size = CTEX_MATERIAL_GRAPH_SOCKET_DESCRIPTOR_CURRENT_SIZE,
        .identifier = "value",
        .display_name = "Value",
        .type = CTEX_SMART_MATERIAL_VALUE_SCALAR,
        .default_value = &input_default,
    }};
    static const ctex_material_graph_socket_descriptor outputs[] = {{
        .size = CTEX_MATERIAL_GRAPH_SOCKET_DESCRIPTOR_CURRENT_SIZE,
        .identifier = "result",
        .display_name = "Result",
        .type = CTEX_SMART_MATERIAL_VALUE_SCALAR,
    }};
    static const ctex_material_graph_property_descriptor properties[] = {
        {
            .size = CTEX_MATERIAL_GRAPH_PROPERTY_DESCRIPTOR_CURRENT_SIZE,
            .identifier = "factor",
            .display_name = "Factor",
            .default_value = &factor_default,
        },
        {
            .size = CTEX_MATERIAL_GRAPH_PROPERTY_DESCRIPTOR_CURRENT_SIZE,
            .identifier = "source_image",
            .display_name = "Source Image",
            .default_value = &image_default,
        },
    };
    static const char* dependencies[] = {"source_image"};
    static const uint32_t targets[] = {CTEX_MATERIAL_GRAPH_TARGET_WGSL};
    static const ctex_smart_material_value_descriptor fixture_inputs[] = {{
        .size = CTEX_SMART_MATERIAL_VALUE_DESCRIPTOR_CURRENT_SIZE,
        .type = CTEX_SMART_MATERIAL_VALUE_SCALAR,
        .scalar = 2.0,
    }};
    static const ctex_smart_material_value_descriptor fixture_outputs[] = {{
        .size = CTEX_SMART_MATERIAL_VALUE_DESCRIPTOR_CURRENT_SIZE,
        .type = CTEX_SMART_MATERIAL_VALUE_SCALAR,
        .scalar = 6.0,
    }};
    static const ctex_material_graph_parity_fixture_descriptor fixtures[] = {{
        .size = CTEX_MATERIAL_GRAPH_PARITY_FIXTURE_DESCRIPTOR_CURRENT_SIZE,
        .identifier = "positive-scale",
        .inputs = fixture_inputs,
        .input_count = 1,
        .expected_outputs = fixture_outputs,
        .expected_output_count = 1,
        .tolerance = 0.0,
    }};
    const ctex_material_graph_host_node_registration_descriptor result = {
        .size = CTEX_MATERIAL_GRAPH_HOST_NODE_REGISTRATION_DESCRIPTOR_CURRENT_SIZE,
        .type_id = "acme.texture.scale",
        .type_version = version,
        .display_name = "Host Scale",
        .inputs = inputs,
        .input_count = 1,
        .outputs = outputs,
        .output_count = 1,
        .properties = properties,
        .property_count = 2,
        .cpu_evaluate = cpu_callback,
        .emit = emit_scale,
        .user_data = capture,
        .deterministic = deterministic,
        .resource_dependencies = dependencies,
        .resource_dependency_count = 1,
        .supported_targets = targets,
        .supported_target_count = 1,
        .parity_fixtures = fixtures,
        .parity_fixture_count = 1,
    };
    return result;
}

static int add_registered_node(const ctex_material_graph_node_registry* registry,
                               const graph_blob* source, graph_blob* result,
                               uint64_t* out_node_id) {
    ctex_material_graph_info info = {.size = CTEX_MATERIAL_GRAPH_INFO_CURRENT_SIZE};
    if (ctex_material_graph_add_registered_node(registry, source->data, source->size,
                                                "acme.texture.scale", 1, (ctex_vec2f){0}, &info,
                                                out_node_id, NULL, 0) != CTEX_RESULT_SUCCESS) {
        fprintf(stderr, "add registered sizing: %s\n", ctex_get_last_diagnostic());
        return 0;
    }
    result->data = (unsigned char*)malloc(info.canonical_size);
    if (result->data == NULL ||
        ctex_material_graph_add_registered_node(
            registry, source->data, source->size, "acme.texture.scale", 1, (ctex_vec2f){0}, &info,
            out_node_id, result->data, info.canonical_size) != CTEX_RESULT_SUCCESS) {
        fprintf(stderr, "add registered output: %s\n", ctex_get_last_diagnostic());
        release_graph(result);
        return 0;
    }
    result->size = info.canonical_size;
    return 1;
}

static int link_registered_node(const graph_blob* source, uint64_t node_id, graph_blob* result) {
    const ctex_material_graph_link_descriptor link = {
        .size = CTEX_MATERIAL_GRAPH_LINK_DESCRIPTOR_CURRENT_SIZE,
        .source_node = node_id,
        .source_socket = "result",
        .target_node = 1,
        .target_socket = "pbr.roughness",
    };
    ctex_material_graph_link_info info = {.size = CTEX_MATERIAL_GRAPH_LINK_INFO_CURRENT_SIZE};
    if (ctex_material_graph_add_link(source->data, source->size, &link, &info, NULL, 0, NULL, 0) !=
        CTEX_RESULT_SUCCESS) {
        fprintf(stderr, "link registered sizing: %s\n", ctex_get_last_diagnostic());
        return 0;
    }
    result->data = (unsigned char*)malloc(info.graph.canonical_size);
    if (result->data == NULL ||
        ctex_material_graph_add_link(source->data, source->size, &link, &info, result->data,
                                     info.graph.canonical_size, NULL, 0) != CTEX_RESULT_SUCCESS) {
        fprintf(stderr, "link registered output: %s\n", ctex_get_last_diagnostic());
        release_graph(result);
        return 0;
    }
    result->size = info.graph.canonical_size;
    return 1;
}

static int verify_contract(ctex_material_graph_node_registry* registry, callback_capture* capture) {
    const char* pinned[] = {"source_image"};
    ctex_material_graph_host_contract_info info = {
        .size = CTEX_MATERIAL_GRAPH_HOST_CONTRACT_INFO_CURRENT_SIZE};
    if (ctex_material_graph_node_registry_verify_contract(
            registry, "acme.texture.scale", 1, pinned, 1, &info, NULL, 0) != CTEX_RESULT_SUCCESS) {
        return 0;
    }
    char* report = (char*)malloc(info.report_size);
    int passed = report != NULL &&
                 ctex_material_graph_node_registry_verify_contract(
                     registry, "acme.texture.scale", 1, pinned, 1, &info, report,
                     info.report_size) == CTEX_RESULT_SUCCESS &&
                 info.parity_passed == 1 && info.parity_failure_count == 0 &&
                 info.replay_eligible == 1 && info.unpinned_dependency_count == 0 &&
                 strstr(report, "\"parity_passed\":true") != NULL && capture->cpu_calls == 2 &&
                 capture->emission_calls == 2;
    free(report);
    info.size = CTEX_MATERIAL_GRAPH_HOST_CONTRACT_INFO_CURRENT_SIZE;
    passed =
        passed &&
        ctex_material_graph_node_registry_verify_contract(
            registry, "acme.texture.scale", 1, NULL, 0, &info, NULL, 0) == CTEX_RESULT_SUCCESS &&
        info.parity_passed == 1 && info.replay_eligible == 0 && info.unpinned_dependency_count == 1;
    return expect(passed, "host node reference contract did not execute and pass both callbacks");
}

static int registered_and_unknown_nodes_validate(void) {
    callback_capture capture = {0};
    ctex_material_graph_node_registry* registry = NULL;
    ctex_material_graph_node_registry* empty_registry = NULL;
    graph_blob base = {0};
    graph_blob with_node = {0};
    graph_blob linked = {0};
    uint64_t node_id = 0;
    int passed = 1;
    if (!create_default_graph(&base) ||
        ctex_material_graph_node_registry_create(&registry) != CTEX_RESULT_SUCCESS ||
        ctex_material_graph_node_registry_create(&empty_registry) != CTEX_RESULT_SUCCESS) {
        release_graph(&base);
        ctex_material_graph_node_registry_destroy(registry);
        ctex_material_graph_node_registry_destroy(empty_registry);
        return 0;
    }
    const ctex_material_graph_host_node_registration_descriptor registration =
        scale_registration(1, 1, &capture, evaluate_scale);
    passed = expect(ctex_material_graph_node_registry_register(registry, &registration) ==
                        CTEX_RESULT_SUCCESS,
                    "valid host node registration was refused") &&
             passed;
    passed = verify_contract(registry, &capture) && passed;
    passed = expect(add_registered_node(registry, &base, &with_node, &node_id) &&
                        link_registered_node(&with_node, node_id, &linked),
                    "registered host node could not be serialized and linked") &&
             passed;

    const char* images[] = {"textures/source.png"};
    const ctex_material_graph_validation_resources_descriptor resources = {
        .size = CTEX_MATERIAL_GRAPH_VALIDATION_RESOURCES_DESCRIPTOR_CURRENT_SIZE,
        .image_resources = images,
        .image_resource_count = 1,
    };
    ctex_material_graph_validation_info validation = {
        .size = CTEX_MATERIAL_GRAPH_VALIDATION_INFO_CURRENT_SIZE};
    passed = expect(ctex_material_graph_validate_registered(
                        registry, linked.data, linked.size, CTEX_MATERIAL_GRAPH_TARGET_WGSL,
                        &resources, &validation, NULL, 0) == CTEX_RESULT_SUCCESS &&
                        validation.valid == 1,
                    "registered host node did not participate in validation") &&
             passed;

    validation.size = CTEX_MATERIAL_GRAPH_VALIDATION_INFO_CURRENT_SIZE;
    passed = expect(ctex_material_graph_validate_registered(
                        registry, linked.data, linked.size, CTEX_MATERIAL_GRAPH_TARGET_HLSL,
                        &resources, &validation, NULL, 0) == CTEX_RESULT_SUCCESS &&
                        validation.valid == 0 && validation.error_count == 1,
                    "unsupported host-node emission target was not reported") &&
             passed;

    validation.size = CTEX_MATERIAL_GRAPH_VALIDATION_INFO_CURRENT_SIZE;
    passed = expect(ctex_material_graph_validate_registered(
                        empty_registry, linked.data, linked.size, CTEX_MATERIAL_GRAPH_TARGET_WGSL,
                        &resources, &validation, NULL, 0) == CTEX_RESULT_SUCCESS &&
                        validation.valid == 0 && validation.error_count == 1,
                    "unknown host node was not marked non-emittable") &&
             passed;
    char* report = (char*)malloc(validation.report_size);
    passed = expect(report != NULL &&
                        ctex_material_graph_validate_registered(
                            empty_registry, linked.data, linked.size,
                            CTEX_MATERIAL_GRAPH_TARGET_WGSL, &resources, &validation, report,
                            validation.report_size) == CTEX_RESULT_SUCCESS &&
                        strstr(report, "acme.texture.scale@1") != NULL &&
                        strstr(report, "missing_node_type") != NULL,
                    "unknown host node diagnostic did not name the opaque type") &&
             passed;
    free(report);

    ctex_material_graph_info inspected = {.size = CTEX_MATERIAL_GRAPH_INFO_CURRENT_SIZE};
    uint32_t equal = 0;
    graph_blob canonical = {0};
    if (ctex_material_graph_inspect(linked.data, linked.size, &inspected, NULL, 0, NULL, 0) ==
        CTEX_RESULT_SUCCESS) {
        canonical.data = (unsigned char*)malloc(inspected.canonical_size);
        canonical.size = inspected.canonical_size;
    }
    passed =
        expect(canonical.data != NULL &&
                   ctex_material_graph_inspect(linked.data, linked.size, &inspected, canonical.data,
                                               canonical.size, NULL, 0) == CTEX_RESULT_SUCCESS &&
                   ctex_material_graph_compare(linked.data, linked.size, canonical.data,
                                               canonical.size, &equal) == CTEX_RESULT_SUCCESS &&
                   equal == 1,
               "unknown host node payload was not preserved canonically") &&
        passed;

    release_graph(&canonical);
    release_graph(&linked);
    release_graph(&with_node);
    release_graph(&base);
    ctex_material_graph_node_registry_destroy(empty_registry);
    ctex_material_graph_node_registry_destroy(registry);
    return passed;
}

static int grouping_and_registration_refusals(void) {
    callback_capture capture = {0};
    ctex_material_graph_node_registry* registry = NULL;
    ctex_material_graph_workspace* workspace = NULL;
    if (ctex_material_graph_node_registry_create(&registry) != CTEX_RESULT_SUCCESS ||
        ctex_material_graph_workspace_create(&workspace) != CTEX_RESULT_SUCCESS) {
        ctex_material_graph_node_registry_destroy(registry);
        ctex_material_graph_workspace_destroy(workspace);
        return 0;
    }
    ctex_material_graph_host_node_registration_descriptor registration =
        scale_registration(1, 1, &capture, evaluate_scale);
    int passed = expect(
        ctex_material_graph_node_registry_register(registry, &registration) == CTEX_RESULT_SUCCESS,
        "grouping fixture registration failed");

    const ctex_material_graph_socket_descriptor output = {
        .size = CTEX_MATERIAL_GRAPH_SOCKET_DESCRIPTOR_CURRENT_SIZE,
        .identifier = "result",
        .display_name = "Result",
        .type = CTEX_SMART_MATERIAL_VALUE_SCALAR,
    };
    const ctex_material_graph_group_descriptor group = {
        .size = CTEX_MATERIAL_GRAPH_GROUP_DESCRIPTOR_CURRENT_SIZE,
        .identifier = "host-scale",
        .display_name = "Host Scale",
        .outputs = &output,
        .output_count = 1,
    };
    uint64_t node_id = 0;
    passed =
        expect(
            ctex_material_graph_workspace_create_group(workspace, &group) == CTEX_RESULT_SUCCESS &&
                ctex_material_graph_workspace_add_registered_node(
                    workspace, registry, CTEX_MATERIAL_GRAPH_OWNER_GROUP, "host-scale",
                    "acme.texture.scale", 1, (ctex_vec2f){0}, &node_id) == CTEX_RESULT_SUCCESS,
            "registered node could not participate in a reusable group") &&
        passed;
    const ctex_material_graph_link_descriptor link = {
        .size = CTEX_MATERIAL_GRAPH_LINK_DESCRIPTOR_CURRENT_SIZE,
        .source_node = node_id,
        .source_socket = "result",
        .target_node = 1,
        .target_socket = "result",
    };
    ctex_material_graph_link_info link_info = {.size = CTEX_MATERIAL_GRAPH_LINK_INFO_CURRENT_SIZE};
    passed = expect(ctex_material_graph_workspace_add_link(
                        workspace, CTEX_MATERIAL_GRAPH_OWNER_GROUP, "host-scale", &link, &link_info,
                        NULL, 0) == CTEX_RESULT_SUCCESS,
                    "registered node could not link inside a reusable group") &&
             passed;

    registration = scale_registration(2, 1, &capture, NULL);
    passed = expect(ctex_material_graph_node_registry_register(registry, &registration) ==
                            CTEX_RESULT_INVALID_ARGUMENT &&
                        strstr(ctex_get_last_diagnostic(), "CPU evaluation") != NULL,
                    "emission-only registration was not refused by name") &&
             passed;

    registration = scale_registration(2, 0, &capture, evaluate_scale);
    passed = expect(ctex_material_graph_node_registry_register(registry, &registration) ==
                        CTEX_RESULT_SUCCESS,
                    "non-deterministic fixture registration failed") &&
             passed;
    const char* pinned[] = {"source_image"};
    ctex_material_graph_host_contract_info contract = {
        .size = CTEX_MATERIAL_GRAPH_HOST_CONTRACT_INFO_CURRENT_SIZE};
    passed = expect(ctex_material_graph_node_registry_verify_contract(
                        registry, "acme.texture.scale", 2, pinned, 1, &contract, NULL, 0) ==
                            CTEX_RESULT_SUCCESS &&
                        contract.parity_passed == 1 && contract.replay_eligible == 0,
                    "non-deterministic host node was considered replay eligible") &&
             passed;

    ctex_material_graph_node_registry_info info = {
        .size = CTEX_MATERIAL_GRAPH_NODE_REGISTRY_INFO_CURRENT_SIZE};
    passed =
        expect(ctex_material_graph_node_registry_get_info(registry, &info) == CTEX_RESULT_SUCCESS &&
                   info.registration_count == 2,
               "host node registry inventory is incorrect") &&
        passed;
    ctex_material_graph_workspace_destroy(workspace);
    ctex_material_graph_node_registry_destroy(registry);
    return passed;
}

int main(void) {
    return registered_and_unknown_nodes_validate() && grouping_and_registration_refusals() ? 0 : 1;
}
