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

static int create_default_graph(graph_blob* graph, char** report,
                                ctex_material_graph_info* out_info) {
    ctex_material_graph_info info = {.size = CTEX_MATERIAL_GRAPH_INFO_CURRENT_SIZE};
    if (ctex_material_graph_create_default(&info, NULL, 0, NULL, 0) != CTEX_RESULT_SUCCESS) {
        return 0;
    }
    graph->data = (unsigned char*)malloc(info.canonical_size);
    *report = (char*)malloc(info.report_size);
    if (graph->data == NULL || *report == NULL) {
        release_graph(graph);
        free(*report);
        *report = NULL;
        return 0;
    }
    if (ctex_material_graph_create_default(&info, graph->data, info.canonical_size, *report,
                                           info.report_size) != CTEX_RESULT_SUCCESS) {
        release_graph(graph);
        free(*report);
        *report = NULL;
        return 0;
    }
    graph->size = info.canonical_size;
    *out_info = info;
    return 1;
}

static int add_node(const graph_blob* source, const char* type_id, ctex_vec2f position,
                    graph_blob* result, uint64_t* out_node_id) {
    ctex_material_graph_info info = {.size = CTEX_MATERIAL_GRAPH_INFO_CURRENT_SIZE};
    uint64_t node_id = 0;
    if (ctex_material_graph_add_builtin_node(source->data, source->size, type_id, position, &info,
                                             &node_id, NULL, 0) != CTEX_RESULT_SUCCESS) {
        return 0;
    }
    result->data = (unsigned char*)malloc(info.canonical_size);
    if (result->data == NULL || ctex_material_graph_add_builtin_node(
                                    source->data, source->size, type_id, position, &info, &node_id,
                                    result->data, info.canonical_size) != CTEX_RESULT_SUCCESS) {
        release_graph(result);
        return 0;
    }
    result->size = info.canonical_size;
    *out_node_id = node_id;
    return 1;
}

static int set_property(const graph_blob* source, uint64_t node_id, const char* property_id,
                        const ctex_smart_material_value_descriptor* value, graph_blob* result) {
    ctex_material_graph_info info = {.size = CTEX_MATERIAL_GRAPH_INFO_CURRENT_SIZE};
    if (ctex_material_graph_set_property_value(source->data, source->size, node_id, property_id,
                                               value, &info, NULL, 0) != CTEX_RESULT_SUCCESS) {
        return 0;
    }
    result->data = (unsigned char*)malloc(info.canonical_size);
    if (result->data == NULL || ctex_material_graph_set_property_value(
                                    source->data, source->size, node_id, property_id, value, &info,
                                    result->data, info.canonical_size) != CTEX_RESULT_SUCCESS) {
        release_graph(result);
        return 0;
    }
    result->size = info.canonical_size;
    return 1;
}

static int set_input(const graph_blob* source, uint64_t node_id, const char* input_id,
                     const ctex_smart_material_value_descriptor* value, graph_blob* result) {
    ctex_material_graph_info info = {.size = CTEX_MATERIAL_GRAPH_INFO_CURRENT_SIZE};
    if (ctex_material_graph_set_input_value(source->data, source->size, node_id, input_id, value,
                                            &info, NULL, 0) != CTEX_RESULT_SUCCESS) {
        return 0;
    }
    result->data = (unsigned char*)malloc(info.canonical_size);
    if (result->data == NULL || ctex_material_graph_set_input_value(
                                    source->data, source->size, node_id, input_id, value, &info,
                                    result->data, info.canonical_size) != CTEX_RESULT_SUCCESS) {
        release_graph(result);
        return 0;
    }
    result->size = info.canonical_size;
    return 1;
}

static int add_link(const graph_blob* source, uint64_t source_node, const char* source_socket,
                    uint64_t target_node, const char* target_socket, graph_blob* result,
                    ctex_material_graph_link_info* out_info, char** replaced_socket) {
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
    *replaced_socket = info.replaced_source_socket_size == 0
                           ? NULL
                           : (char*)malloc(info.replaced_source_socket_size);
    if (result->data == NULL ||
        (info.replaced_source_socket_size != 0 && *replaced_socket == NULL) ||
        ctex_material_graph_add_link(source->data, source->size, &link, &info, result->data,
                                     info.graph.canonical_size, *replaced_socket,
                                     info.replaced_source_socket_size) != CTEX_RESULT_SUCCESS) {
        release_graph(result);
        free(*replaced_socket);
        *replaced_socket = NULL;
        return 0;
    }
    result->size = info.graph.canonical_size;
    *out_info = info;
    return 1;
}

static int default_graph_round_trips_and_compares(void) {
    graph_blob original = {0};
    char* report = NULL;
    ctex_material_graph_info info;
    if (!expect(create_default_graph(&original, &report, &info),
                "default material graph creation failed")) {
        return 0;
    }
    int passed = expect(info.output_node_id == 1 && info.node_count == 1 && info.link_count == 0 &&
                            info.output_channel_count == 9,
                        "default graph does not expose exactly one nine-channel output") &&
                 expect(strstr(report, "\"role\":\"output\"") != NULL &&
                            strstr(report, "\"id\":\"pbr.base_color\"") != NULL &&
                            strstr(report, "\"id\":\"pbr.roughness\"") != NULL &&
                            strstr(report, "\"id\":\"pbr.subsurface\"") != NULL,
                        "default graph inspection omitted output channels");

    ctex_material_graph_info restored = {.size = CTEX_MATERIAL_GRAPH_INFO_CURRENT_SIZE};
    if (passed) {
        passed = expect(ctex_material_graph_inspect(original.data, original.size, &restored, NULL,
                                                    0, NULL, 0) == CTEX_RESULT_SUCCESS &&
                            restored.canonical_size == original.size,
                        "graph inspection sizing failed");
    }
    unsigned char* canonical = passed ? (unsigned char*)malloc(restored.canonical_size) : NULL;
    char* restored_report = passed ? (char*)malloc(restored.report_size) : NULL;
    if (passed) {
        passed = expect(
            canonical != NULL && restored_report != NULL &&
                ctex_material_graph_inspect(original.data, original.size, &restored, canonical,
                                            restored.canonical_size, restored_report,
                                            restored.report_size) == CTEX_RESULT_SUCCESS &&
                memcmp(canonical, original.data, original.size) == 0 &&
                strcmp(restored_report, report) == 0,
            "graph round trip changed its canonical document");
    }
    uint32_t equal = 0;
    if (passed) {
        passed = expect(
            ctex_material_graph_compare(original.data, original.size, canonical,
                                        restored.canonical_size, &equal) == CTEX_RESULT_SUCCESS &&
                equal == 1,
            "equal material graphs did not compare equal");
    }
    free(canonical);
    free(restored_report);
    free(report);
    release_graph(&original);
    return passed;
}

static int coercion_and_replacement_are_reported(void) {
    graph_blob graph = {0};
    char* report = NULL;
    ctex_material_graph_info graph_info;
    graph_blob scalar_graph = {0};
    graph_blob colour_graph = {0};
    graph_blob first_link = {0};
    graph_blob replacement = {0};
    uint64_t scalar = 0;
    uint64_t colour = 0;
    ctex_material_graph_link_info link_info;
    char* replaced = NULL;
    int passed = create_default_graph(&graph, &report, &graph_info) &&
                 add_node(&graph, "ctex.input.constant-value", (ctex_vec2f){2.0F, 3.0F},
                          &scalar_graph, &scalar) &&
                 add_node(&scalar_graph, "ctex.input.constant-colour", (ctex_vec2f){4.0F, 5.0F},
                          &colour_graph, &colour) &&
                 add_link(&colour_graph, scalar, "value", graph_info.output_node_id,
                          "pbr.roughness", &first_link, &link_info, &replaced);
    if (passed) {
        passed = expect(link_info.coercion == CTEX_MATERIAL_GRAPH_COERCION_IDENTITY &&
                            link_info.replaced == 0 && replaced == NULL,
                        "initial scalar link reported the wrong coercion or replacement");
    }
    free(replaced);
    replaced = NULL;
    if (passed) {
        passed = add_link(&first_link, colour, "colour", graph_info.output_node_id, "pbr.roughness",
                          &replacement, &link_info, &replaced) &&
                 expect(link_info.coercion == CTEX_MATERIAL_GRAPH_COERCION_COLOUR_TO_SCALAR &&
                            link_info.replaced == 1 && link_info.replaced_source_node == scalar &&
                            replaced != NULL && strcmp(replaced, "value") == 0 &&
                            link_info.graph.link_count == 1,
                        "occupied input replacement did not report its prior link");
    }
    uint32_t equal = 1;
    if (passed) {
        passed =
            expect(ctex_material_graph_compare(graph.data, graph.size, replacement.data,
                                               replacement.size, &equal) == CTEX_RESULT_SUCCESS &&
                       equal == 0,
                   "mutated graph still compared equal to its source");
    }
    free(replaced);
    free(report);
    release_graph(&replacement);
    release_graph(&first_link);
    release_graph(&colour_graph);
    release_graph(&scalar_graph);
    release_graph(&graph);
    return passed;
}

static int incompatible_links_and_cycles_are_atomic(void) {
    graph_blob graph = {0};
    char* report = NULL;
    ctex_material_graph_info graph_info;
    graph_blob blur_graph = {0};
    uint64_t blur = 0;
    int passed = create_default_graph(&graph, &report, &graph_info) &&
                 add_node(&graph, "ctex.filter.blur", (ctex_vec2f){0.0F, 0.0F}, &blur_graph, &blur);
    if (passed) {
        const ctex_material_graph_link_descriptor incompatible = {
            .size = CTEX_MATERIAL_GRAPH_LINK_DESCRIPTOR_CURRENT_SIZE,
            .source_node = blur,
            .source_socket = "image",
            .target_node = graph_info.output_node_id,
            .target_socket = "pbr.roughness",
        };
        ctex_material_graph_link_info info = {.size = CTEX_MATERIAL_GRAPH_LINK_INFO_CURRENT_SIZE};
        passed = expect(
            ctex_material_graph_add_link(blur_graph.data, blur_graph.size, &incompatible, &info,
                                         NULL, 0, NULL, 0) == CTEX_RESULT_INVALID_ARGUMENT &&
                ctex_get_last_diagnostic_code() == CTEX_DIAGNOSTIC_INVALID_MATERIAL_GRAPH &&
                strstr(ctex_get_last_diagnostic(), "image") != NULL &&
                strstr(ctex_get_last_diagnostic(), "scalar") != NULL,
            "incompatible graph link was not refused by named socket types");
    }

    graph_blob first_math = {0};
    graph_blob both_math = {0};
    graph_blob forward = {0};
    uint64_t first = 0;
    uint64_t second = 0;
    ctex_material_graph_link_info info;
    char* replaced = NULL;
    if (passed) {
        passed =
            add_node(&graph, "ctex.math.scalar", (ctex_vec2f){1.0F, 1.0F}, &first_math, &first) &&
            add_node(&first_math, "ctex.math.scalar", (ctex_vec2f){2.0F, 2.0F}, &both_math,
                     &second) &&
            add_link(&both_math, first, "value", second, "a", &forward, &info, &replaced);
    }
    free(replaced);
    if (passed) {
        const ctex_material_graph_link_descriptor cycle = {
            .size = CTEX_MATERIAL_GRAPH_LINK_DESCRIPTOR_CURRENT_SIZE,
            .source_node = second,
            .source_socket = "value",
            .target_node = first,
            .target_socket = "a",
        };
        ctex_material_graph_link_info cycle_info = {.size =
                                                        CTEX_MATERIAL_GRAPH_LINK_INFO_CURRENT_SIZE};
        passed =
            expect(ctex_material_graph_add_link(forward.data, forward.size, &cycle, &cycle_info,
                                                NULL, 0, NULL, 0) == CTEX_RESULT_INVALID_ARGUMENT &&
                       ctex_get_last_diagnostic_code() == CTEX_DIAGNOSTIC_INVALID_MATERIAL_GRAPH &&
                       strstr(ctex_get_last_diagnostic(), " -> ") != NULL,
                   "cycle-closing link was not refused with its path");
    }
    release_graph(&forward);
    release_graph(&both_math);
    release_graph(&first_math);
    release_graph(&blur_graph);
    free(report);
    release_graph(&graph);
    return passed;
}

static int validation_names_missing_resources_without_emission(void) {
    graph_blob graph = {0};
    char* graph_report = NULL;
    ctex_material_graph_info graph_info;
    graph_blob image_graph = {0};
    graph_blob positioned_graph = {0};
    graph_blob named_graph = {0};
    graph_blob linked_graph = {0};
    uint64_t image = 0;
    const ctex_smart_material_value_descriptor value = {
        .size = CTEX_SMART_MATERIAL_VALUE_DESCRIPTOR_CURRENT_SIZE,
        .type = CTEX_SMART_MATERIAL_VALUE_IMAGE,
        .text = "library/downloaded/albedo.png",
    };
    const ctex_smart_material_value_descriptor vector = {
        .size = CTEX_SMART_MATERIAL_VALUE_DESCRIPTOR_CURRENT_SIZE,
        .type = CTEX_SMART_MATERIAL_VALUE_VECTOR,
        .vector = {1.0F, 2.0F, 3.0F},
    };
    ctex_material_graph_link_info link_info;
    char* replaced = NULL;
    int passed =
        create_default_graph(&graph, &graph_report, &graph_info) &&
        add_node(&graph, "ctex.texture.image", (ctex_vec2f){8.0F, 9.0F}, &image_graph, &image) &&
        set_input(&image_graph, image, "vector", &vector, &positioned_graph) &&
        set_property(&positioned_graph, image, "image", &value, &named_graph) &&
        add_link(&named_graph, image, "colour", graph_info.output_node_id, "pbr.base_color",
                 &linked_graph, &link_info, &replaced);
    free(replaced);

    ctex_material_graph_validation_info validation = {
        .size = CTEX_MATERIAL_GRAPH_VALIDATION_INFO_CURRENT_SIZE};
    if (passed) {
        passed = expect(ctex_material_graph_validate(linked_graph.data, linked_graph.size, NULL,
                                                     &validation, NULL, 0) == CTEX_RESULT_SUCCESS &&
                            validation.valid == 0 && validation.error_count == 1 &&
                            validation.warning_count == 0,
                        "missing graph resource did not fail independent validation");
    }
    char* report = passed ? (char*)malloc(validation.report_size) : NULL;
    if (passed) {
        passed = expect(report != NULL &&
                            ctex_material_graph_validate(
                                linked_graph.data, linked_graph.size, NULL, &validation, report,
                                validation.report_size) == CTEX_RESULT_SUCCESS &&
                            strstr(report, "missing_image_resource") != NULL &&
                            strstr(report, "library/downloaded/albedo.png") != NULL,
                        "validation report did not name the unavailable image");
    }
    const char* images[] = {"library/downloaded/albedo.png"};
    const ctex_material_graph_validation_resources_descriptor resources = {
        .size = CTEX_MATERIAL_GRAPH_VALIDATION_RESOURCES_DESCRIPTOR_CURRENT_SIZE,
        .image_resources = images,
        .image_resource_count = 1,
    };
    if (passed) {
        passed =
            expect(ctex_material_graph_validate(linked_graph.data, linked_graph.size, &resources,
                                                &validation, NULL, 0) == CTEX_RESULT_SUCCESS &&
                       validation.valid == 1 && validation.error_count == 0 &&
                       validation.warning_count == 0,
                   "available graph resource was still reported missing");
    }
    free(report);
    release_graph(&linked_graph);
    release_graph(&named_graph);
    release_graph(&positioned_graph);
    release_graph(&image_graph);
    free(graph_report);
    release_graph(&graph);
    return passed;
}

int main(void) {
    return default_graph_round_trips_and_compares() && coercion_and_replacement_are_reported() &&
                   incompatible_links_and_cycles_are_atomic() &&
                   validation_names_missing_resources_without_emission()
               ? 0
               : 1;
}
