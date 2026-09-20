#include <ctex/capi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct render_state {
    double values[4];
    size_t value_count;
    size_t calls;
    ctex_result result;
    ctex_parity_rendered_channel_descriptor channel;
} render_state;

static int expect(int condition, const char* message) {
    if (!condition) {
        fprintf(stderr, "%s\n", message);
    }
    return condition;
}

static ctex_result render_fixture(const ctex_parity_fixture_descriptor* fixture,
                                  ctex_parity_rendered_fixture_descriptor* out_rendered,
                                  void* user_data) {
    render_state* state = (render_state*)user_data;
    ++state->calls;
    if (state->result != CTEX_RESULT_SUCCESS) {
        return state->result;
    }
    if (fixture == NULL || strcmp(fixture->identifier, "paint-basic") != 0) {
        return CTEX_RESULT_INVALID_ARGUMENT;
    }
    state->channel = (ctex_parity_rendered_channel_descriptor){
        .size = CTEX_PARITY_RENDERED_CHANNEL_DESCRIPTOR_CURRENT_SIZE,
        .semantic = "base_color",
        .values = state->values,
        .value_count = state->value_count,
    };
    *out_rendered = (ctex_parity_rendered_fixture_descriptor){
        .size = CTEX_PARITY_RENDERED_FIXTURE_DESCRIPTOR_CURRENT_SIZE,
        .width = 1,
        .height = 1,
        .channels = &state->channel,
        .channel_count = 1,
    };
    return CTEX_RESULT_SUCCESS;
}

static int find_executor_indices(const ctex_executor_registry* registry, size_t* cpu_index,
                                 size_t* host_index) {
    size_t count = 0;
    if (ctex_executor_registry_get_count(registry, &count) != CTEX_RESULT_SUCCESS) {
        return 0;
    }
    *cpu_index = count;
    *host_index = count;
    for (size_t index = 0; index < count; ++index) {
        ctex_executor_info info = {.size = CTEX_EXECUTOR_INFO_CURRENT_SIZE};
        if (ctex_executor_registry_get_info(registry, index, &info, NULL, 0, NULL, 0, NULL, 0, NULL,
                                            0) != CTEX_RESULT_SUCCESS) {
            return 0;
        }
        uint32_t formats[16] = {0};
        char identifier[32] = {0};
        char display_name[64] = {0};
        char device_name[64] = {0};
        if (info.supported_texture_format_count > 16 || info.required_identifier_size > 32 ||
            info.required_display_name_size > 64 || info.required_device_name_size > 64 ||
            ctex_executor_registry_get_info(registry, index, &info, formats, 16, identifier, 32,
                                            display_name, 64, device_name,
                                            64) != CTEX_RESULT_SUCCESS) {
            return 0;
        }
        if (strcmp(identifier, "cpu") == 0) {
            *cpu_index = index;
        } else if (strcmp(identifier, "host") == 0) {
            *host_index = index;
        }
    }
    return *cpu_index < count && *host_index < count;
}

static ctex_executor_registry* create_registry(uint32_t attached, size_t* cpu_index,
                                               size_t* host_index) {
    const uint32_t format = CTEX_EXECUTOR_TEXTURE_RGBA8_UNORM;
    const ctex_host_executor_descriptor host = {
        .size = CTEX_HOST_EXECUTOR_DESCRIPTOR_CURRENT_SIZE,
        .device_name = "Parity test GPU",
        .binding_budget = 8,
        .maximum_texture_dimension = 4096,
        .supported_texture_formats = &format,
        .supported_texture_format_count = 1,
        .floating_point_filtering = 1,
        .compute_available = 1,
        .attached = attached,
    };
    ctex_executor_registry* registry = NULL;
    if (ctex_executor_registry_create(&host, &registry) != CTEX_RESULT_SUCCESS ||
        !find_executor_indices(registry, cpu_index, host_index)) {
        ctex_executor_registry_destroy(registry);
        return NULL;
    }
    return registry;
}

static ctex_parity_fixture_descriptor fixture_descriptor(
    const ctex_parity_fixture_channel_descriptor* channel) {
    return (ctex_parity_fixture_descriptor){
        .size = CTEX_PARITY_FIXTURE_DESCRIPTOR_CURRENT_SIZE,
        .identifier = "paint-basic",
        .document = "document-v1",
        .stroke = "stroke-v1",
        .camera = "camera-v1",
        .material = "material-v1",
        .channels = channel,
        .channel_count = 1,
    };
}

static int read_gate_info(const ctex_parity_gate_result* result, ctex_parity_gate_info* out_info,
                          char** out_report) {
    ctex_parity_gate_info info = {.size = CTEX_PARITY_GATE_INFO_CURRENT_SIZE};
    if (ctex_parity_gate_result_get_info(result, &info, NULL, 0) != CTEX_RESULT_SUCCESS) {
        return 0;
    }
    char* report = (char*)malloc(info.required_report_size);
    if (report == NULL ||
        ctex_parity_gate_result_get_info(result, &info, report, info.required_report_size) !=
            CTEX_RESULT_SUCCESS) {
        free(report);
        return 0;
    }
    *out_info = info;
    *out_report = report;
    return 1;
}

static int pass_and_drift_are_reported(void) {
    size_t cpu_index = 0;
    size_t host_index = 0;
    ctex_executor_registry* registry = create_registry(1, &cpu_index, &host_index);
    const ctex_parity_fixture_channel_descriptor channel = {
        .size = CTEX_PARITY_FIXTURE_CHANNEL_DESCRIPTOR_CURRENT_SIZE,
        .semantic = "base_color",
        .value_class = CTEX_PARITY_UNORM8,
        .filtered = 0,
        .component_count = 4,
    };
    const ctex_parity_fixture_descriptor fixture = fixture_descriptor(&channel);
    render_state cpu = {
        .values = {0.25, 0.5, 0.75, 1.0}, .value_count = 4, .result = CTEX_RESULT_SUCCESS};
    render_state host = {
        .values = {0.252, 0.5, 0.75, 1.0}, .value_count = 4, .result = CTEX_RESULT_SUCCESS};
    const ctex_parity_executor_binding_descriptor bindings[] = {
        {.size = CTEX_PARITY_EXECUTOR_BINDING_DESCRIPTOR_CURRENT_SIZE,
         .executor_index = host_index,
         .render = render_fixture,
         .user_data = &host},
        {.size = CTEX_PARITY_EXECUTOR_BINDING_DESCRIPTOR_CURRENT_SIZE,
         .executor_index = cpu_index,
         .render = render_fixture,
         .user_data = &cpu},
    };
    ctex_parity_gate_result* result = NULL;
    ctex_parity_gate_info info = {0};
    char* report = NULL;
    int passed = expect(
        registry != NULL &&
            ctex_executor_run_parity_gate(registry, &fixture, 1, bindings, 2, &result) ==
                CTEX_RESULT_SUCCESS &&
            read_gate_info(result, &info, &report) && info.passed == 1 &&
            info.executor_count == 2 && info.reference_count == 1 && info.passed_count == 1 &&
            info.failed_count == 0 && strstr(report, "\"executor\":\"cpu\"") != NULL &&
            strstr(report, "\"status\":\"passed\"") != NULL,
        "in-tolerance parity gate did not pass");
    free(report);
    ctex_parity_gate_result_destroy(result);

    host.values[0] = 0.35;
    result = NULL;
    report = NULL;
    passed = expect(ctex_executor_run_parity_gate(registry, &fixture, 1, bindings, 2, &result) ==
                            CTEX_RESULT_SUCCESS &&
                        read_gate_info(result, &info, &report) && info.passed == 0 &&
                        info.failed_count == 1 && strstr(report, "paint-basic") != NULL &&
                        strstr(report, "base_color") != NULL &&
                        strstr(report, "measuredDeviation") != NULL,
                    "out-of-tolerance parity gate omitted failure details") &&
             passed;
    free(report);
    ctex_parity_gate_result_destroy(result);
    ctex_executor_registry_destroy(registry);
    return passed;
}

static int unavailable_executor_is_unmeasured(void) {
    size_t cpu_index = 0;
    size_t host_index = 0;
    ctex_executor_registry* registry = create_registry(0, &cpu_index, &host_index);
    const ctex_parity_fixture_channel_descriptor channel = {
        .size = CTEX_PARITY_FIXTURE_CHANNEL_DESCRIPTOR_CURRENT_SIZE,
        .semantic = "base_color",
        .value_class = CTEX_PARITY_UNORM8,
        .component_count = 4,
    };
    const ctex_parity_fixture_descriptor fixture = fixture_descriptor(&channel);
    render_state cpu = {
        .values = {0.25, 0.5, 0.75, 1.0}, .value_count = 4, .result = CTEX_RESULT_SUCCESS};
    render_state host = {
        .values = {0.25, 0.5, 0.75, 1.0}, .value_count = 4, .result = CTEX_RESULT_SUCCESS};
    const ctex_parity_executor_binding_descriptor bindings[] = {
        {.size = CTEX_PARITY_EXECUTOR_BINDING_DESCRIPTOR_CURRENT_SIZE,
         .executor_index = cpu_index,
         .render = render_fixture,
         .user_data = &cpu},
        {.size = CTEX_PARITY_EXECUTOR_BINDING_DESCRIPTOR_CURRENT_SIZE,
         .executor_index = host_index,
         .render = render_fixture,
         .user_data = &host},
    };
    ctex_parity_gate_result* result = NULL;
    ctex_parity_gate_info info = {0};
    char* report = NULL;
    const int passed = expect(
        registry != NULL &&
            ctex_executor_run_parity_gate(registry, &fixture, 1, bindings, 2, &result) ==
                CTEX_RESULT_SUCCESS &&
            read_gate_info(result, &info, &report) && info.passed == 1 &&
            info.unmeasured_count == 1 && host.calls == 0 && strstr(report, "unmeasured") != NULL &&
            strstr(report, "host-not-attached") != NULL,
        "unavailable executor was rendered or was not reported as unmeasured");
    free(report);
    ctex_parity_gate_result_destroy(result);
    ctex_executor_registry_destroy(registry);
    return passed;
}

static int invalid_reference_output_is_atomic(void) {
    size_t cpu_index = 0;
    size_t host_index = 0;
    ctex_executor_registry* registry = create_registry(1, &cpu_index, &host_index);
    const ctex_parity_fixture_channel_descriptor channel = {
        .size = CTEX_PARITY_FIXTURE_CHANNEL_DESCRIPTOR_CURRENT_SIZE,
        .semantic = "base_color",
        .value_class = CTEX_PARITY_UNORM8,
        .component_count = 4,
    };
    const ctex_parity_fixture_descriptor fixture = fixture_descriptor(&channel);
    render_state cpu = {
        .values = {0.25, 0.5, 0.75, 1.0}, .value_count = 3, .result = CTEX_RESULT_SUCCESS};
    const ctex_parity_executor_binding_descriptor binding = {
        .size = CTEX_PARITY_EXECUTOR_BINDING_DESCRIPTOR_CURRENT_SIZE,
        .executor_index = cpu_index,
        .render = render_fixture,
        .user_data = &cpu,
    };
    ctex_parity_gate_result* result = (ctex_parity_gate_result*)1;
    int passed = expect(
        registry != NULL &&
            ctex_executor_run_parity_gate(registry, &fixture, 1, &binding, 1, &result) ==
                CTEX_RESULT_INVALID_ARGUMENT &&
            result == NULL && ctex_get_last_diagnostic_code() == CTEX_DIAGNOSTIC_INVALID_EXECUTOR,
        "malformed CPU reference output published a partial result");
    cpu.value_count = 4;
    cpu.result = CTEX_RESULT_CANCELLED;
    result = (ctex_parity_gate_result*)1;
    passed = expect(ctex_executor_run_parity_gate(registry, &fixture, 1, &binding, 1, &result) ==
                            CTEX_RESULT_CANCELLED &&
                        result == NULL,
                    "CPU callback failure did not propagate atomically") &&
             passed;
    ctex_executor_registry_destroy(registry);
    return passed;
}

int main(void) {
    return pass_and_drift_are_reported() && unavailable_executor_is_unmeasured() &&
                   invalid_reference_output_is_atomic()
               ? 0
               : 1;
}
