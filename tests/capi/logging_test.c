#include <ctex/capi.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

typedef struct log_capture {
    size_t count;
    ctex_log_severity severity;
    ctex_diagnostic_code code;
    void* user_data;
    char category[64];
    char message[256];
} log_capture;

static void copy_text(char* destination, size_t capacity, const char* source) {
    size_t length = strlen(source);
    if (length >= capacity) {
        length = capacity - 1;
    }
    memcpy(destination, source, length);
    destination[length] = '\0';
}

static void capture_log(ctex_log_severity severity, const char* category, const char* message,
                        void* user_data) {
    log_capture* capture = (log_capture*)user_data;
    ++capture->count;
    capture->severity = severity;
    capture->code = ctex_get_last_diagnostic_code();
    capture->user_data = user_data;
    copy_text(capture->category, sizeof(capture->category), category);
    copy_text(capture->message, sizeof(capture->message), message);
}

static int expect_texture_set_failure(ctex_document* document,
                                      const ctex_texture_set_descriptor* descriptor,
                                      ctex_diagnostic_code code) {
    return ctex_document_create_texture_set(document, descriptor) == CTEX_RESULT_INVALID_ARGUMENT &&
           ctex_get_last_diagnostic_code() == code;
}

static int texture_set_codes_are_specific(void) {
    ctex_document* document = NULL;
    ctex_texture_set_descriptor descriptor = {
        CTEX_TEXTURE_SET_DESCRIPTOR_CURRENT_SIZE,
        "Body",
        CTEX_PARTITION_SOURCE_MATERIAL,
        "material:body",
        "UV0",
        16,
        16,
        8,
        0,
    };
    if (ctex_document_create(&document) != CTEX_RESULT_SUCCESS) {
        return 0;
    }

    descriptor.display_name = "";
    if (!expect_texture_set_failure(document, &descriptor,
                                    CTEX_DIAGNOSTIC_EMPTY_TEXTURE_SET_DISPLAY_NAME)) {
        ctex_document_destroy(document);
        return 0;
    }
    descriptor.display_name = "Body";
    descriptor.partition_key = "";
    if (!expect_texture_set_failure(document, &descriptor,
                                    CTEX_DIAGNOSTIC_EMPTY_TEXTURE_SET_PARTITION_KEY)) {
        ctex_document_destroy(document);
        return 0;
    }
    descriptor.partition_key = "material:body";
    descriptor.uv_set = "";
    if (!expect_texture_set_failure(document, &descriptor,
                                    CTEX_DIAGNOSTIC_EMPTY_TEXTURE_SET_UV_SET)) {
        ctex_document_destroy(document);
        return 0;
    }
    descriptor.uv_set = "UV0";
    descriptor.width = 0;
    if (!expect_texture_set_failure(document, &descriptor,
                                    CTEX_DIAGNOSTIC_INVALID_TEXTURE_SET_RESOLUTION)) {
        ctex_document_destroy(document);
        return 0;
    }
    descriptor.width = 16;
    descriptor.default_bit_depth = 7;
    if (!expect_texture_set_failure(document, &descriptor,
                                    CTEX_DIAGNOSTIC_INVALID_TEXTURE_SET_BIT_DEPTH)) {
        ctex_document_destroy(document);
        return 0;
    }
    descriptor.default_bit_depth = 8;
    if (ctex_document_create_texture_set(document, &descriptor) != CTEX_RESULT_SUCCESS ||
        !expect_texture_set_failure(document, &descriptor, CTEX_DIAGNOSTIC_DUPLICATE_TEXTURE_SET)) {
        ctex_document_destroy(document);
        return 0;
    }

    ctex_document_destroy(document);
    return 1;
}

int main(void) {
    log_capture capture = {0};
    ctex_log_sink_descriptor sink = {
        CTEX_LOG_SINK_DESCRIPTOR_CURRENT_SIZE,
        capture_log,
        &capture,
        CTEX_LOG_SEVERITY_ERROR,
    };

    if (ctex_set_log_sink(&sink) != CTEX_RESULT_SUCCESS) {
        return 1;
    }
    if (ctex_document_create(NULL) != CTEX_RESULT_INVALID_ARGUMENT || capture.count != 1) {
        return 2;
    }
    if (capture.severity != CTEX_LOG_SEVERITY_ERROR ||
        capture.code != CTEX_DIAGNOSTIC_NULL_ARGUMENT || capture.user_data != &capture ||
        strcmp(capture.category, "capi.diagnostic") != 0 ||
        strstr(capture.message, "ctex_document_create") == NULL ||
        strstr(capture.message, "out_document=null") == NULL) {
        return 3;
    }
    if (ctex_get_last_diagnostic_code() != CTEX_DIAGNOSTIC_NULL_ARGUMENT) {
        return 4;
    }

    sink.minimum_severity = CTEX_LOG_SEVERITY_FATAL;
    if (ctex_set_log_sink(&sink) != CTEX_RESULT_SUCCESS ||
        ctex_document_create(NULL) != CTEX_RESULT_INVALID_ARGUMENT || capture.count != 1) {
        return 5;
    }

    sink.minimum_severity = CTEX_LOG_SEVERITY_ERROR;
    if (ctex_set_log_sink(&sink) != CTEX_RESULT_SUCCESS) {
        return 6;
    }
    sink.minimum_severity = UINT32_MAX;
    if (ctex_set_log_sink(&sink) != CTEX_RESULT_INVALID_ARGUMENT || capture.count != 2 ||
        capture.code != CTEX_DIAGNOSTIC_INVALID_ENUM_VALUE ||
        strstr(capture.message, "minimum_severity") == NULL) {
        return 7;
    }

    sink.minimum_severity = CTEX_LOG_SEVERITY_ERROR;
    sink.size = 0;
    if (ctex_set_log_sink(&sink) != CTEX_RESULT_INVALID_ARGUMENT || capture.count != 3 ||
        capture.code != CTEX_DIAGNOSTIC_INVALID_DESCRIPTOR_SIZE ||
        strstr(capture.message, "descriptor.size") == NULL) {
        return 8;
    }

    sink.size = CTEX_LOG_SINK_DESCRIPTOR_CURRENT_SIZE;
    sink.callback = NULL;
    if (ctex_set_log_sink(&sink) != CTEX_RESULT_INVALID_ARGUMENT || capture.count != 4 ||
        capture.code != CTEX_DIAGNOSTIC_NULL_ARGUMENT ||
        strstr(capture.message, "descriptor.callback=null") == NULL) {
        return 9;
    }

    if (ctex_set_log_sink(NULL) != CTEX_RESULT_SUCCESS ||
        ctex_document_create(NULL) != CTEX_RESULT_INVALID_ARGUMENT || capture.count != 4) {
        return 10;
    }
    if (!texture_set_codes_are_specific()) {
        return 11;
    }
    return 0;
}
