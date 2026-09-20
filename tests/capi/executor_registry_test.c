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

static int read_executor(const ctex_executor_registry* registry, size_t index,
                         ctex_executor_info* out_info, char** out_identifier) {
    ctex_executor_info info = {.size = CTEX_EXECUTOR_INFO_CURRENT_SIZE};
    if (ctex_executor_registry_get_info(registry, index, &info, NULL, 0, NULL, 0, NULL, 0, NULL,
                                        0) != CTEX_RESULT_SUCCESS) {
        return 0;
    }
    uint32_t* formats = (uint32_t*)malloc(info.supported_texture_format_count * sizeof(uint32_t));
    char* identifier = (char*)malloc(info.required_identifier_size);
    char* display_name = (char*)malloc(info.required_display_name_size);
    char* device_name = (char*)malloc(info.required_device_name_size);
    const int allocated =
        formats != NULL && identifier != NULL && display_name != NULL && device_name != NULL;
    const int read =
        allocated &&
        ctex_executor_registry_get_info(
            registry, index, &info, formats, info.supported_texture_format_count, identifier,
            info.required_identifier_size, display_name, info.required_display_name_size,
            device_name, info.required_device_name_size) == CTEX_RESULT_SUCCESS;
    free(device_name);
    free(display_name);
    free(formats);
    if (!read) {
        free(identifier);
        return 0;
    }
    *out_info = info;
    *out_identifier = identifier;
    return 1;
}

static int registry_exposes_routes_features_and_selection(void) {
    const uint32_t formats[] = {CTEX_EXECUTOR_TEXTURE_RGBA8_UNORM,
                                CTEX_EXECUTOR_TEXTURE_RGBA16_FLOAT};
    const ctex_host_executor_descriptor host = {
        .size = CTEX_HOST_EXECUTOR_DESCRIPTOR_CURRENT_SIZE,
        .device_name = "Test host GPU",
        .binding_budget = 24,
        .maximum_texture_dimension = 16384,
        .supported_texture_formats = formats,
        .supported_texture_format_count = 2,
        .floating_point_filtering = 1,
        .compute_available = 1,
        .attached = 1,
    };
    ctex_executor_registry* registry = NULL;
    size_t count = 0;
    int passed = expect(ctex_executor_registry_create(&host, &registry) == CTEX_RESULT_SUCCESS,
                        "executor registry creation failed") &&
                 expect(ctex_executor_registry_get_count(registry, &count) == CTEX_RESULT_SUCCESS &&
                            count >= 2,
                        "executor registry omitted an always-available route");

    size_t cpu_index = count;
    size_t host_index = count;
    for (size_t index = 0; passed && index < count; ++index) {
        ctex_executor_info info = {0};
        char* identifier = NULL;
        passed = expect(read_executor(registry, index, &info, &identifier),
                        "executor descriptor read failed");
        if (passed && strcmp(identifier, "cpu") == 0) {
            cpu_index = index;
            passed = expect(info.route == CTEX_EXECUTOR_ROUTE_CPU_REFERENCE &&
                                info.availability == CTEX_EXECUTOR_AVAILABLE &&
                                info.supported_texture_format_count == 13,
                            "CPU executor descriptor is incomplete");
        } else if (passed && strcmp(identifier, "host") == 0) {
            host_index = index;
            passed =
                expect(info.route == CTEX_EXECUTOR_ROUTE_HOST_EXECUTED &&
                           info.availability == CTEX_EXECUTOR_AVAILABLE &&
                           info.binding_budget == 24 && info.maximum_texture_dimension == 16384 &&
                           info.supported_texture_format_count == 2 &&
                           info.floating_point_filtering == 1 && info.compute_available == 1,
                       "host executor features changed across the C boundary");
        }
        free(identifier);
    }
    passed = expect(cpu_index < count && host_index < count,
                    "executor registry did not enumerate CPU and host routes") &&
             passed;

    ctex_executor_selection_info selection = {.size = CTEX_EXECUTOR_SELECTION_INFO_CURRENT_SIZE};
    passed = expect(ctex_executor_registry_select(registry, "host", &selection, NULL, 0, NULL, 0) ==
                            CTEX_RESULT_SUCCESS &&
                        selection.source == CTEX_EXECUTOR_SELECTION_EXPLICIT &&
                        selection.selected_executor_index == host_index,
                    "explicit host selection failed") &&
             passed;
    char* requested = (char*)malloc(selection.required_requested_identifier_size);
    char* message = (char*)malloc(selection.required_message_size);
    passed = expect(requested != NULL && message != NULL &&
                        ctex_executor_registry_select(registry, "host", &selection, requested,
                                                      selection.required_requested_identifier_size,
                                                      message, selection.required_message_size) ==
                            CTEX_RESULT_SUCCESS &&
                        strcmp(requested, "host") == 0 && strstr(message, "requested") != NULL,
                    "selection report did not use caller-owned buffers") &&
             passed;
    free(message);
    free(requested);

    selection.size = CTEX_EXECUTOR_SELECTION_INFO_CURRENT_SIZE;
    passed = expect(ctex_executor_registry_pin_default(registry, "cpu") == CTEX_RESULT_SUCCESS &&
                        ctex_executor_registry_select(registry, NULL, &selection, NULL, 0, NULL,
                                                      0) == CTEX_RESULT_SUCCESS &&
                        selection.selected_executor_index == cpu_index &&
                        selection.source == CTEX_EXECUTOR_SELECTION_EXPLICIT &&
                        ctex_executor_registry_clear_default(registry) == CTEX_RESULT_SUCCESS,
                    "pinned executor default was not applied and cleared") &&
             passed;

    ctex_executor_registry_destroy(registry);
    return passed;
}

static int fallback_reports_require_recovered_state(void) {
    ctex_executor_fallback_descriptor descriptor = {
        .size = CTEX_EXECUTOR_FALLBACK_DESCRIPTOR_CURRENT_SIZE,
        .failed_executor = "host",
        .failure = CTEX_EXECUTION_FAILURE_DEVICE_LOST,
        .failure_detail = "device removed",
        .disposition = CTEX_EXECUTOR_CPU_FALLBACK,
        .fallback_executor = "cpu",
        .recovery_restored = 0,
    };
    ctex_executor_fallback_info info = {.size = CTEX_EXECUTOR_FALLBACK_INFO_CURRENT_SIZE};
    int passed = expect(ctex_executor_make_fallback_report(&descriptor, &info, NULL, 0) ==
                                CTEX_RESULT_INVALID_ARGUMENT &&
                            ctex_get_last_diagnostic_code() == CTEX_DIAGNOSTIC_INVALID_EXECUTOR,
                        "CPU fallback was accepted without restored recovery state");
    descriptor.recovery_restored = 1;
    info.size = CTEX_EXECUTOR_FALLBACK_INFO_CURRENT_SIZE;
    passed = expect(ctex_executor_make_fallback_report(&descriptor, &info, NULL, 0) ==
                            CTEX_RESULT_SUCCESS &&
                        info.disposition == CTEX_EXECUTOR_CPU_FALLBACK &&
                        info.recovery_restored == 1 && info.required_message_size > 1,
                    "valid recovered fallback report sizing failed") &&
             passed;
    char* message = (char*)malloc(info.required_message_size);
    passed = expect(message != NULL &&
                        ctex_executor_make_fallback_report(&descriptor, &info, message,
                                                           info.required_message_size) ==
                            CTEX_RESULT_SUCCESS &&
                        strstr(message, "fell back to 'cpu'") != NULL,
                    "fallback report omitted the selected CPU route") &&
             passed;
    free(message);
    return passed;
}

static int invalid_host_descriptor_is_atomic(void) {
    const uint32_t format = CTEX_EXECUTOR_TEXTURE_RGBA8_UNORM;
    const ctex_host_executor_descriptor invalid = {
        .size = CTEX_HOST_EXECUTOR_DESCRIPTOR_CURRENT_SIZE,
        .device_name = "Broken host",
        .binding_budget = 1,
        .maximum_texture_dimension = 0,
        .supported_texture_formats = &format,
        .supported_texture_format_count = 1,
        .attached = 1,
    };
    ctex_executor_registry* registry = (ctex_executor_registry*)1;
    const int invalid_refused =
        ctex_executor_registry_create(&invalid, &registry) == CTEX_RESULT_INVALID_ARGUMENT &&
        registry == NULL;
    registry = (ctex_executor_registry*)1;
    return expect(
        invalid_refused &&
            ctex_executor_registry_create(NULL, &registry) == CTEX_RESULT_INVALID_ARGUMENT &&
            registry == NULL,
        "missing or invalid host executor published a partial registry");
}

int main(void) {
    return registry_exposes_routes_features_and_selection() &&
                   fallback_reports_require_recovered_state() && invalid_host_descriptor_is_atomic()
               ? 0
               : 1;
}
