#include <ctex/capi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int expect(int condition, const char* message) {
    if (!condition) {
        fprintf(stderr, "%s\n", message);
    }
    return condition;
}

static size_t count_occurrences(const char* text, const char* needle) {
    size_t count = 0;
    const size_t needle_size = strlen(needle);
    while ((text = strstr(text, needle)) != NULL) {
        ++count;
        text += needle_size;
    }
    return count;
}

static int catalogue_is_complete_and_deterministic(void) {
    ctex_material_graph_catalogue_info info = {.size =
                                                   CTEX_MATERIAL_GRAPH_CATALOGUE_INFO_CURRENT_SIZE};
    if (!expect(ctex_material_graph_get_builtin_catalogue(&info, NULL, 0) == CTEX_RESULT_SUCCESS,
                "material graph catalogue sizing failed") ||
        !expect(info.node_count == 52 && info.input_node_count == 10 &&
                    info.texture_node_count == 10 && info.colour_filter_node_count == 18 &&
                    info.vector_math_node_count == 14 && info.math_operation_count == 40 &&
                    info.vector_math_operation_count == 27 && info.report_size > 1,
                "material graph catalogue counts differ from the specification")) {
        return 0;
    }

    char sentinel = 'x';
    if (!expect(ctex_material_graph_get_builtin_catalogue(&info, &sentinel, 0) ==
                        CTEX_RESULT_BUFFER_TOO_SMALL &&
                    sentinel == 'x',
                "undersized catalogue output was not atomic")) {
        return 0;
    }

    char* first = (char*)malloc(info.report_size);
    char* second = (char*)malloc(info.report_size);
    if (!expect(first != NULL && second != NULL, "catalogue output allocation failed")) {
        free(first);
        free(second);
        return 0;
    }
    const size_t report_size = info.report_size;
    const int calls_succeeded = ctex_material_graph_get_builtin_catalogue(
                                    &info, first, report_size) == CTEX_RESULT_SUCCESS &&
                                ctex_material_graph_get_builtin_catalogue(
                                    &info, second, report_size) == CTEX_RESULT_SUCCESS;
    const int passed =
        expect(calls_succeeded, "material graph catalogue output failed") &&
        expect(first[report_size - 1] == '\0' && strcmp(first, second) == 0,
               "material graph catalogue output is not deterministic text") &&
        expect(count_occurrences(first, "\"type_id\":") == 52,
               "catalogue report omitted or duplicated a built-in node") &&
        expect(strstr(first, "\"type_id\":\"ctex.input.mesh-map\"") != NULL &&
                   strstr(first, "\"type_id\":\"ctex.texture.image\"") != NULL &&
                   strstr(first, "\"type_id\":\"ctex.colour.blend\"") != NULL &&
                   strstr(first, "\"type_id\":\"ctex.vector.mix-normal-map\"") != NULL,
               "catalogue report omitted a required node family") &&
        expect(strstr(first, "\"allowed_values\":[\"normal\",\"darken\"") != NULL &&
                   strstr(first,
                          "\"allowed_values\":[\"partial_derivative\",\"whiteout\","
                          "\"reoriented\"]") != NULL,
               "catalogue report omitted required blend or normal modes") &&
        expect(strstr(first, "\"formula\":\"a + b\"") != NULL &&
                   strstr(first, "\"formula\":\"cross(a, b)\"") != NULL,
               "catalogue report omitted documented math formulas");
    free(first);
    free(second);
    return passed;
}

static int invalid_inputs_are_refused(void) {
    ctex_material_graph_catalogue_info info = {.size = 0};
    return expect(ctex_material_graph_get_builtin_catalogue(NULL, NULL, 0) ==
                      CTEX_RESULT_INVALID_ARGUMENT,
                  "null catalogue info was accepted") &&
           expect(ctex_material_graph_get_builtin_catalogue(&info, NULL, 0) ==
                      CTEX_RESULT_INVALID_ARGUMENT,
                  "invalid catalogue info size was accepted");
}

int main(void) {
    return catalogue_is_complete_and_deterministic() && invalid_inputs_are_refused() ? 0 : 1;
}
