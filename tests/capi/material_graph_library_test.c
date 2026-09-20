#include <ctex/capi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct byte_blob {
    unsigned char* data;
    size_t size;
} byte_blob;

static int expect(int condition, const char* message) {
    if (!condition) {
        fprintf(stderr, "%s\n", message);
    }
    return condition;
}

static void release_blob(byte_blob* blob) {
    free(blob->data);
    blob->data = NULL;
    blob->size = 0;
}

static int create_default_graph(byte_blob* graph) {
    ctex_material_graph_info info = {.size = CTEX_MATERIAL_GRAPH_INFO_CURRENT_SIZE};
    if (ctex_material_graph_create_default(&info, NULL, 0, NULL, 0) != CTEX_RESULT_SUCCESS) {
        return 0;
    }
    graph->data = (unsigned char*)malloc(info.canonical_size);
    if (graph->data == NULL ||
        ctex_material_graph_create_default(&info, graph->data, info.canonical_size, NULL, 0) !=
            CTEX_RESULT_SUCCESS) {
        release_blob(graph);
        return 0;
    }
    graph->size = info.canonical_size;
    return 1;
}

static int add_constant_node(const byte_blob* source, byte_blob* graph) {
    ctex_material_graph_info info = {.size = CTEX_MATERIAL_GRAPH_INFO_CURRENT_SIZE};
    uint64_t node_id = 0;
    if (ctex_material_graph_add_builtin_node(source->data, source->size,
                                             "ctex.input.constant-value", (ctex_vec2f){0}, &info,
                                             &node_id, NULL, 0) != CTEX_RESULT_SUCCESS) {
        return 0;
    }
    graph->data = (unsigned char*)malloc(info.canonical_size);
    if (graph->data == NULL ||
        ctex_material_graph_add_builtin_node(
            source->data, source->size, "ctex.input.constant-value", (ctex_vec2f){0}, &info,
            &node_id, graph->data, info.canonical_size) != CTEX_RESULT_SUCCESS) {
        release_blob(graph);
        return 0;
    }
    graph->size = info.canonical_size;
    return 1;
}

static int create_empty_library(byte_blob* library) {
    ctex_material_graph_library_info info = {.size = CTEX_MATERIAL_GRAPH_LIBRARY_INFO_CURRENT_SIZE};
    if (ctex_material_graph_library_create_empty(&info, NULL, 0, NULL, 0) != CTEX_RESULT_SUCCESS) {
        return 0;
    }
    library->data = (unsigned char*)malloc(info.canonical_size);
    if (library->data == NULL ||
        ctex_material_graph_library_create_empty(&info, library->data, info.canonical_size, NULL,
                                                 0) != CTEX_RESULT_SUCCESS) {
        release_blob(library);
        return 0;
    }
    library->size = info.canonical_size;
    return 1;
}

static int add_preset(const byte_blob* source, const char* stable_id, const char* name,
                      const char* thumbnail, const byte_blob* graph, byte_blob* library) {
    const ctex_material_graph_preset_descriptor preset = {
        .size = CTEX_MATERIAL_GRAPH_PRESET_DESCRIPTOR_CURRENT_SIZE,
        .stable_id = stable_id,
        .name = name,
        .thumbnail_resource = thumbnail,
        .graph_serialized = graph->data,
        .graph_serialized_size = graph->size,
    };
    ctex_material_graph_library_info info = {.size = CTEX_MATERIAL_GRAPH_LIBRARY_INFO_CURRENT_SIZE};
    if (ctex_material_graph_library_add_preset(source->data, source->size, &preset, &info, NULL, 0,
                                               NULL, 0) != CTEX_RESULT_SUCCESS) {
        return 0;
    }
    library->data = (unsigned char*)malloc(info.canonical_size);
    if (library->data == NULL || ctex_material_graph_library_add_preset(
                                     source->data, source->size, &preset, &info, library->data,
                                     info.canonical_size, NULL, 0) != CTEX_RESULT_SUCCESS) {
        release_blob(library);
        return 0;
    }
    library->size = info.canonical_size;
    return 1;
}

static int build_library(int reverse, const byte_blob* wood, const byte_blob* metal,
                         byte_blob* library) {
    byte_blob empty = {0};
    byte_blob first = {0};
    int passed = create_empty_library(&empty);
    if (passed && reverse) {
        passed = add_preset(&empty, "org.cybertexel.material.wood", "Wood", "thumbs/wood.png", wood,
                            &first) &&
                 add_preset(&first, "org.cybertexel.material.metal", "Metal", "thumbs/metal.png",
                            metal, library);
    } else if (passed) {
        passed = add_preset(&empty, "org.cybertexel.material.metal", "Metal", "thumbs/metal.png",
                            metal, &first) &&
                 add_preset(&first, "org.cybertexel.material.wood", "Wood", "thumbs/wood.png", wood,
                            library);
    }
    release_blob(&first);
    release_blob(&empty);
    return passed;
}

static int canonical_library_and_stable_resolution(void) {
    byte_blob metal = {0};
    byte_blob wood = {0};
    byte_blob base = {0};
    byte_blob first = {0};
    byte_blob reverse = {0};
    int passed = create_default_graph(&base) && add_constant_node(&base, &wood);
    metal.data = (unsigned char*)malloc(base.size);
    if (metal.data != NULL) {
        memcpy(metal.data, base.data, base.size);
        metal.size = base.size;
    } else {
        passed = 0;
    }
    passed = passed && build_library(0, &wood, &metal, &first) &&
             build_library(1, &wood, &metal, &reverse);
    passed = expect(passed && first.size == reverse.size &&
                        memcmp(first.data, reverse.data, first.size) == 0,
                    "material library bytes depended on preset insertion order") &&
             passed;

    ctex_material_graph_library_info info = {.size = CTEX_MATERIAL_GRAPH_LIBRARY_INFO_CURRENT_SIZE};
    passed = expect(ctex_material_graph_library_inspect(first.data, first.size, &info, NULL, 0,
                                                        NULL, 0) == CTEX_RESULT_SUCCESS &&
                        info.preset_count == 2 && info.canonical_size == first.size,
                    "material library inspection reported the wrong inventory") &&
             passed;
    char* report = (char*)malloc(info.report_size);
    byte_blob transferred = {0};
    transferred.data = (unsigned char*)malloc(info.canonical_size);
    transferred.size = info.canonical_size;
    passed = expect(report != NULL && transferred.data != NULL &&
                        ctex_material_graph_library_inspect(
                            first.data, first.size, &info, transferred.data, transferred.size,
                            report, info.report_size) == CTEX_RESULT_SUCCESS &&
                        strstr(report, "org.cybertexel.material.metal") != NULL &&
                        strstr(report, "org.cybertexel.material.wood") != NULL &&
                        strstr(report, "thumbs/wood.png") != NULL,
                    "material library report lost identity, name, or thumbnail metadata") &&
             passed;

    ctex_material_graph_info graph_info = {.size = CTEX_MATERIAL_GRAPH_INFO_CURRENT_SIZE};
    passed = expect(ctex_material_graph_library_resolve_preset(
                        transferred.data, transferred.size, "org.cybertexel.material.wood",
                        &graph_info, NULL, 0, NULL, 0) == CTEX_RESULT_SUCCESS,
                    "stable material identity did not resolve after transfer") &&
             passed;
    byte_blob resolved = {0};
    resolved.data = (unsigned char*)malloc(graph_info.canonical_size);
    resolved.size = graph_info.canonical_size;
    uint32_t equal = 0;
    passed =
        expect(resolved.data != NULL &&
                   ctex_material_graph_library_resolve_preset(
                       transferred.data, transferred.size, "org.cybertexel.material.wood",
                       &graph_info, resolved.data, resolved.size, NULL, 0) == CTEX_RESULT_SUCCESS &&
                   ctex_material_graph_compare(resolved.data, resolved.size, wood.data, wood.size,
                                               &equal) == CTEX_RESULT_SUCCESS &&
                   equal == 1,
               "transferred stable identity resolved to a different graph") &&
        passed;

    unsigned char sentinel = 0x5a;
    info.size = CTEX_MATERIAL_GRAPH_LIBRARY_INFO_CURRENT_SIZE;
    passed = expect(ctex_material_graph_library_inspect(first.data, first.size, &info, &sentinel, 1,
                                                        NULL, 0) == CTEX_RESULT_BUFFER_TOO_SMALL &&
                        sentinel == 0x5a,
                    "too-small material library output was not atomic") &&
             passed;

    free(report);
    release_blob(&resolved);
    release_blob(&transferred);
    release_blob(&reverse);
    release_blob(&first);
    release_blob(&wood);
    release_blob(&metal);
    release_blob(&base);
    return passed;
}

static int invalid_identities_are_refused(void) {
    byte_blob graph = {0};
    byte_blob library = {0};
    byte_blob with_preset = {0};
    if (!create_default_graph(&graph) || !create_empty_library(&library) ||
        !add_preset(&library, "stable", "Material", "thumb.png", &graph, &with_preset)) {
        release_blob(&with_preset);
        release_blob(&library);
        release_blob(&graph);
        return 0;
    }
    const ctex_material_graph_preset_descriptor duplicate = {
        .size = CTEX_MATERIAL_GRAPH_PRESET_DESCRIPTOR_CURRENT_SIZE,
        .stable_id = "stable",
        .name = "Other",
        .thumbnail_resource = "other.png",
        .graph_serialized = graph.data,
        .graph_serialized_size = graph.size,
    };
    ctex_material_graph_library_info library_info = {
        .size = CTEX_MATERIAL_GRAPH_LIBRARY_INFO_CURRENT_SIZE};
    int passed = expect(ctex_material_graph_library_add_preset(
                            with_preset.data, with_preset.size, &duplicate, &library_info, NULL, 0,
                            NULL, 0) == CTEX_RESULT_INVALID_ARGUMENT &&
                            strstr(ctex_get_last_diagnostic(), "stable") != NULL,
                        "duplicate stable material identity was not named and refused");
    ctex_material_graph_info graph_info = {.size = CTEX_MATERIAL_GRAPH_INFO_CURRENT_SIZE};
    passed = expect(ctex_material_graph_library_resolve_preset(
                        with_preset.data, with_preset.size, "missing", &graph_info, NULL, 0, NULL,
                        0) == CTEX_RESULT_INVALID_ARGUMENT &&
                        strstr(ctex_get_last_diagnostic(), "missing") != NULL,
                    "missing stable material identity was not named and refused") &&
             passed;
    release_blob(&with_preset);
    release_blob(&library);
    release_blob(&graph);
    return passed;
}

int main(void) {
    return canonical_library_and_stable_resolution() && invalid_identities_are_refused() ? 0 : 1;
}
