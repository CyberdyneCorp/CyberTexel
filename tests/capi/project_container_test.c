#include <ctex/capi.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int expect(int condition, const char* message) {
    if (!condition) {
        fprintf(stderr, "%s\n", message);
    }
    return condition;
}

static uint32_t read_u32(const unsigned char* bytes) {
    return (uint32_t)bytes[0] | ((uint32_t)bytes[1] << 8u) | ((uint32_t)bytes[2] << 16u) |
           ((uint32_t)bytes[3] << 24u);
}

static uint64_t read_u64(const unsigned char* bytes) {
    uint64_t value = 0;
    unsigned int index = 0;
    for (index = 0; index < 8; ++index) {
        value |= (uint64_t)bytes[index] << (index * 8u);
    }
    return value;
}

static void write_u32(unsigned char* bytes, uint32_t value) {
    unsigned int index = 0;
    for (index = 0; index < 4; ++index) {
        bytes[index] = (unsigned char)(value >> (index * 8u));
    }
}

static void write_u64(unsigned char* bytes, uint64_t value) {
    unsigned int index = 0;
    for (index = 0; index < 8; ++index) {
        bytes[index] = (unsigned char)(value >> (index * 8u));
    }
}

typedef struct byte_builder {
    unsigned char* bytes;
    size_t capacity;
    size_t size;
} byte_builder;

static int append_bytes(byte_builder* builder, const void* source, size_t size) {
    if (size > builder->capacity - builder->size) {
        return 0;
    }
    memcpy(builder->bytes + builder->size, source, size);
    builder->size += size;
    return 1;
}

static int append_u32(byte_builder* builder, uint32_t value) {
    unsigned char bytes[4];
    write_u32(bytes, value);
    return append_bytes(builder, bytes, sizeof(bytes));
}

static int append_u64(byte_builder* builder, uint64_t value) {
    unsigned char bytes[8];
    write_u64(bytes, value);
    return append_bytes(builder, bytes, sizeof(bytes));
}

static int append_string(byte_builder* builder, const char* value) {
    const size_t size = strlen(value);
    return size <= UINT32_MAX && append_u32(builder, (uint32_t)size) &&
           append_bytes(builder, value, size);
}

static int append_section(byte_builder* container, uint32_t kind, const byte_builder* payload) {
    return append_u32(container, kind) && append_u32(container, 1) &&
           append_u64(container, payload->size) &&
           append_bytes(container, payload->bytes, payload->size);
}

static int append_asset(byte_builder* payload, const char* identifier, const char* kind) {
    return append_string(payload, identifier) && append_string(payload, kind) &&
           append_u32(payload, 1) && append_u32(payload, 0) && append_u32(payload, 0) &&
           append_u64(payload, strlen(kind)) && append_bytes(payload, kind, strlen(kind));
}

static unsigned char* create_complete_document_container(size_t* out_size) {
    static const struct {
        const char* identifier;
        const char* kind;
    } assets[] = {
        {"document/texture-sets", "texture-sets"},
        {"document/layers", "layer-stack"},
        {"document/masks", "masks"},
        {"document/groups", "groups"},
        {"document/filters", "filters"},
        {"document/material-graphs", "material-graphs"},
        {"document/node-groups", "node-groups"},
        {"document/stroke-presets", "stroke-presets"},
        {"document/export-presets", "export-presets"},
        {"document/mesh-map-bindings", "mesh-map-bindings"},
        {"document/channel-descriptors", "channel-descriptors"},
        {"document/editable-entries", "editable-authoring"},
        {"document/replay-records", "operation-records"},
        {"document/settings", "document-settings"},
    };
    unsigned char tiled_storage[4];
    unsigned char resource_storage[256];
    unsigned char asset_storage[4096];
    unsigned char checkpoint_storage[8];
    byte_builder tiled = {tiled_storage, sizeof(tiled_storage), 0};
    byte_builder resources = {resource_storage, sizeof(resource_storage), 0};
    byte_builder asset_payload = {asset_storage, sizeof(asset_storage), 0};
    byte_builder checkpoint = {checkpoint_storage, sizeof(checkpoint_storage), 0};
    unsigned char* encoded = (unsigned char*)malloc(8192);
    byte_builder container = {encoded, encoded == NULL ? 0 : 8192, 0};
    size_t index = 0;

    if (encoded == NULL || !append_u32(&tiled, 0) || !append_u32(&resources, 1) ||
        !append_string(&resources, "resources/reference-image") ||
        !append_string(&resources, "image") || !append_string(&resources, "images/reference.png") ||
        !append_u32(&resources, 0) || !append_u64(&resources, 0) ||
        !append_u32(&asset_payload, (uint32_t)(sizeof(assets) / sizeof(assets[0])))) {
        free(encoded);
        return NULL;
    }
    for (index = 0; index < sizeof(assets) / sizeof(assets[0]); ++index) {
        if (!append_asset(&asset_payload, assets[index].identifier, assets[index].kind)) {
            free(encoded);
            return NULL;
        }
    }
    if (!append_u64(&checkpoint, 37) || !append_bytes(&container, "CTEXPRJ\0", 8) ||
        !append_u32(&container, 40) || !append_u32(&container, ctex_get_version().major) ||
        !append_u32(&container, ctex_get_version().minor) ||
        !append_u32(&container, ctex_get_version().patch) || !append_u32(&container, 4) ||
        !append_u32(&container, 0) || !append_u64(&container, 0) ||
        !append_section(&container, 1, &tiled) || !append_section(&container, 2, &resources) ||
        !append_section(&container, 3, &asset_payload) ||
        !append_section(&container, 4, &checkpoint)) {
        free(encoded);
        return NULL;
    }
    write_u64(encoded + 32, container.size - 40);
    *out_size = container.size;
    return encoded;
}

static unsigned char* create_empty_container(size_t* out_size) {
    unsigned char* encoded = NULL;
    unsigned char* short_buffer = NULL;
    size_t required = 0;
    if (!expect(ctex_project_container_create_empty(NULL, 0, &required) == CTEX_RESULT_SUCCESS,
                "empty-container sizing query failed") ||
        !expect(required > 40, "empty container is missing its framed sections")) {
        return NULL;
    }
    short_buffer = (unsigned char*)malloc(required - 1);
    if (!expect(short_buffer != NULL, "could not allocate short test buffer")) {
        return NULL;
    }
    memset(short_buffer, 0xa5, required - 1);
    if (!expect(ctex_project_container_create_empty(short_buffer, required - 1, &required) ==
                    CTEX_RESULT_BUFFER_TOO_SMALL,
                "short empty-container buffer was accepted") ||
        !expect(short_buffer[0] == 0xa5 && short_buffer[required - 2] == 0xa5 &&
                    ctex_get_last_diagnostic_code() == CTEX_DIAGNOSTIC_BUFFER_TOO_SMALL,
                "short empty-container write was not atomic")) {
        free(short_buffer);
        return NULL;
    }
    free(short_buffer);
    encoded = (unsigned char*)malloc(required);
    if (!expect(encoded != NULL, "could not allocate test container") ||
        !expect(ctex_project_container_create_empty(encoded, required, &required) ==
                    CTEX_RESULT_SUCCESS,
                "empty-container creation failed")) {
        free(encoded);
        return NULL;
    }
    *out_size = required;
    return encoded;
}

static int probe_reports_the_header_version(const unsigned char* encoded, size_t encoded_size) {
    ctex_project_container_version version = {CTEX_PROJECT_CONTAINER_VERSION_CURRENT_SIZE, 0, 0, 0};
    const ctex_version library = ctex_get_version();
    return expect(ctex_project_container_probe_version(encoded, encoded_size, &version) ==
                      CTEX_RESULT_SUCCESS,
                  "container version probe failed") &&
           expect(version.major == library.major && version.minor == library.minor &&
                      version.patch == library.patch,
                  "container and library versions differ") &&
           expect(ctex_project_container_probe_version(encoded, 12, &version) ==
                      CTEX_RESULT_INVALID_ARGUMENT,
                  "truncated container header was accepted") &&
           expect(ctex_get_last_diagnostic_code() == CTEX_DIAGNOSTIC_INVALID_PROJECT_CONTAINER,
                  "truncated header has the wrong diagnostic code");
}

static int normalize_is_deterministic_and_atomic(const unsigned char* encoded,
                                                 size_t encoded_size) {
    ctex_project_container_info info = {.size = CTEX_PROJECT_CONTAINER_INFO_CURRENT_SIZE};
    unsigned char* canonical = NULL;
    unsigned char* untouched = NULL;
    char* report = NULL;
    char short_report = 'x';
    int passed = 1;
    if (!expect(ctex_project_container_normalize(encoded, encoded_size, NULL, &info, NULL, 0, NULL,
                                                 0) == CTEX_RESULT_SUCCESS,
                "container normalization sizing query failed") ||
        !expect(info.canonical_size == encoded_size && info.report_size > 1,
                "normalization returned incorrect output sizes")) {
        return 0;
    }
    canonical = (unsigned char*)malloc(info.canonical_size);
    untouched = (unsigned char*)malloc(info.canonical_size);
    report = (char*)malloc(info.report_size);
    if (!expect(canonical != NULL && untouched != NULL && report != NULL,
                "could not allocate normalization outputs")) {
        free(canonical);
        free(untouched);
        free(report);
        return 0;
    }
    memset(canonical, 0xa5, info.canonical_size);
    memcpy(untouched, canonical, info.canonical_size);
    passed = expect(ctex_project_container_normalize(encoded, encoded_size, NULL, &info, canonical,
                                                     info.canonical_size, &short_report,
                                                     1) == CTEX_RESULT_BUFFER_TOO_SMALL,
                    "short report buffer was accepted") &&
             expect(memcmp(canonical, untouched, info.canonical_size) == 0 && short_report == 'x',
                    "multi-buffer normalization wrote a partial result") &&
             expect(ctex_project_container_normalize(encoded, encoded_size, NULL, &info, canonical,
                                                     info.canonical_size, report,
                                                     info.report_size) == CTEX_RESULT_SUCCESS,
                    "container normalization failed") &&
             expect(memcmp(canonical, encoded, encoded_size) == 0,
                    "unchanged container did not normalize byte-identically") &&
             expect(strstr(report, "\"images\":[]") != NULL &&
                        strstr(report, "\"resources\":[]") != NULL &&
                        strstr(report, "\"assets\":[]") != NULL,
                    "normalization report omitted empty container inventories");
    free(canonical);
    free(untouched);
    free(report);
    return passed;
}

static int bounded_read_refuses_the_input(const unsigned char* encoded, size_t encoded_size) {
    ctex_project_container_read_limits_descriptor limits;
    ctex_project_container_info info = {.size = CTEX_PROJECT_CONTAINER_INFO_CURRENT_SIZE};
    memset(&limits, 0xff, sizeof(limits));
    limits.size = CTEX_PROJECT_CONTAINER_READ_LIMITS_DESCRIPTOR_CURRENT_SIZE;
    limits.maximum_input_bytes = encoded_size - 1;
    return expect(ctex_project_container_normalize(encoded, encoded_size, &limits, &info, NULL, 0,
                                                   NULL, 0) == CTEX_RESULT_OVER_BUDGET,
                  "container input ceiling was not enforced") &&
           expect(ctex_get_last_diagnostic_code() == CTEX_DIAGNOSTIC_INVALID_PROJECT_CONTAINER &&
                      strstr(ctex_get_last_diagnostic(), "input byte limit") != NULL,
                  "bounded-read refusal omitted its stable diagnostic");
}

static int versioned_structures_reject_unknown_layouts(const unsigned char* encoded,
                                                       size_t encoded_size) {
    ctex_project_container_version version = {.size = 0};
    ctex_project_container_info info = {.size = CTEX_PROJECT_CONTAINER_INFO_CURRENT_SIZE};
    ctex_project_container_read_limits_descriptor limits;
    memset(&limits, 0xff, sizeof(limits));
    limits.size = CTEX_PROJECT_CONTAINER_READ_LIMITS_DESCRIPTOR_CURRENT_SIZE + 1;
    return expect(ctex_project_container_probe_version(encoded, encoded_size, &version) ==
                      CTEX_RESULT_INVALID_ARGUMENT,
                  "truncated version structure was accepted") &&
           expect(ctex_get_last_diagnostic_code() == CTEX_DIAGNOSTIC_INVALID_DESCRIPTOR_SIZE,
                  "truncated version structure has the wrong diagnostic") &&
           expect(ctex_project_container_normalize(encoded, encoded_size, &limits, &info, NULL, 0,
                                                   NULL, 0) == CTEX_RESULT_INVALID_ARGUMENT,
                  "future limits structure was accepted") &&
           expect(ctex_get_last_diagnostic_code() == CTEX_DIAGNOSTIC_INVALID_DESCRIPTOR_SIZE,
                  "future limits structure has the wrong diagnostic");
}

static unsigned char* add_future_opaque_section(const unsigned char* encoded, size_t encoded_size,
                                                size_t* out_size) {
    static const unsigned char payload[] = {0x13, 0x37, 0x42};
    const size_t section_size = 16 + sizeof(payload);
    unsigned char* future = (unsigned char*)malloc(encoded_size + section_size);
    if (future == NULL) {
        return NULL;
    }
    memcpy(future, encoded, encoded_size);
    write_u32(future + 12, ctex_get_version().major + 1);
    write_u32(future + 24, read_u32(future + 24) + 1);
    write_u64(future + 32, read_u64(future + 32) + section_size);
    write_u32(future + encoded_size, 4242);
    write_u32(future + encoded_size + 4, 9);
    write_u64(future + encoded_size + 8, sizeof(payload));
    memcpy(future + encoded_size + 16, payload, sizeof(payload));
    *out_size = encoded_size + section_size;
    return future;
}

static int future_content_is_reported_and_preserved(const unsigned char* encoded,
                                                    size_t encoded_size) {
    ctex_project_container_info info = {.size = CTEX_PROJECT_CONTAINER_INFO_CURRENT_SIZE};
    unsigned char* future = NULL;
    unsigned char* canonical = NULL;
    char* report = NULL;
    size_t future_size = 0;
    int passed = 1;
    future = add_future_opaque_section(encoded, encoded_size, &future_size);
    if (!expect(future != NULL, "could not allocate future container")) {
        return 0;
    }
    if (!expect(ctex_project_container_normalize(future, future_size, NULL, &info, NULL, 0, NULL,
                                                 0) == CTEX_RESULT_SUCCESS,
                "future container sizing query failed")) {
        free(future);
        return 0;
    }
    canonical = (unsigned char*)malloc(info.canonical_size);
    report = (char*)malloc(info.report_size);
    if (!expect(canonical != NULL && report != NULL, "could not allocate future outputs")) {
        free(future);
        free(canonical);
        free(report);
        return 0;
    }
    passed =
        expect(info.newer_schema == 1 && info.opaque_section_count == 1,
               "future container metadata was not reported") &&
        expect(ctex_project_container_normalize(future, future_size, NULL, &info, canonical,
                                                info.canonical_size, report,
                                                info.report_size) == CTEX_RESULT_SUCCESS,
               "future container normalization failed") &&
        expect(info.canonical_size == future_size && memcmp(canonical, future, future_size) == 0,
               "future opaque content was not preserved") &&
        expect(strstr(report, "\"newer_schema\":true") != NULL &&
                   strstr(report, "\"kind\":4242") != NULL &&
                   strstr(report, "\"version\":9") != NULL,
               "future opaque content was absent from the report");
    free(future);
    free(canonical);
    free(report);
    return passed;
}

static int atomic_save_publishes_canonical_bytes(const unsigned char* encoded, size_t encoded_size);

static int complete_document_round_trips_losslessly(void) {
    unsigned char* encoded = NULL;
    unsigned char* canonical = NULL;
    char* report = NULL;
    size_t encoded_size = 0;
    ctex_project_container_info info = {.size = CTEX_PROJECT_CONTAINER_INFO_CURRENT_SIZE};
    int passed = 1;

    encoded = create_complete_document_container(&encoded_size);
    if (!expect(encoded != NULL, "could not create the complete-document fixture") ||
        !expect(ctex_project_container_normalize(encoded, encoded_size, NULL, &info, NULL, 0, NULL,
                                                 0) == CTEX_RESULT_SUCCESS,
                "complete-document sizing query failed")) {
        free(encoded);
        return 0;
    }
    canonical = (unsigned char*)malloc(info.canonical_size);
    report = (char*)malloc(info.report_size);
    if (!expect(canonical != NULL && report != NULL,
                "could not allocate complete-document outputs")) {
        free(encoded);
        free(canonical);
        free(report);
        return 0;
    }
    passed =
        expect(info.asset_count == 14 && info.resource_count == 1,
               "complete-document inventory counts changed") &&
        expect(ctex_project_container_normalize(encoded, encoded_size, NULL, &info, canonical,
                                                info.canonical_size, report,
                                                info.report_size) == CTEX_RESULT_SUCCESS,
               "complete-document normalization failed") &&
        expect(info.canonical_size == encoded_size && memcmp(canonical, encoded, encoded_size) == 0,
               "complete-document payloads did not round-trip byte-identically") &&
        expect(strstr(report, "\"id\":\"document/texture-sets\"") != NULL &&
                   strstr(report, "\"id\":\"document/material-graphs\"") != NULL &&
                   strstr(report, "\"id\":\"document/editable-entries\"") != NULL &&
                   strstr(report, "\"id\":\"document/replay-records\"") != NULL &&
                   strstr(report, "\"id\":\"document/settings\"") != NULL &&
                   strstr(report, "\"id\":\"resources/reference-image\"") != NULL,
               "complete-document report omitted preserved domain content");
    if (passed) {
        passed = atomic_save_publishes_canonical_bytes(canonical, info.canonical_size);
    }
    free(encoded);
    free(canonical);
    free(report);
    return passed;
}

static int atomic_save_publishes_canonical_bytes(const unsigned char* encoded,
                                                 size_t encoded_size) {
    static const char path[] = "ctex-capi-project-container-test.ctex";
    ctex_project_container_info info = {.size = CTEX_PROJECT_CONTAINER_INFO_CURRENT_SIZE};
    unsigned char* saved = NULL;
    FILE* file = NULL;
    long file_size = 0;
    int passed = 1;
    remove(path);
    if (!expect(ctex_project_container_save_atomic(encoded, encoded_size, NULL, path, &info) ==
                    CTEX_RESULT_SUCCESS,
                "atomic project save failed")) {
        return 0;
    }
    file = fopen(path, "rb");
    if (!expect(file != NULL, "atomic project save did not publish its destination")) {
        remove(path);
        return 0;
    }
    passed = expect(fseek(file, 0, SEEK_END) == 0, "could not seek saved project") && passed;
    file_size = ftell(file);
    passed = expect(file_size >= 0 && (size_t)file_size == encoded_size,
                    "saved project has the wrong size") &&
             passed;
    rewind(file);
    saved = (unsigned char*)malloc(encoded_size);
    passed = expect(saved != NULL, "could not allocate saved-project buffer") && passed;
    if (saved != NULL) {
        passed = expect(fread(saved, 1, encoded_size, file) == encoded_size,
                        "could not read saved project") &&
                 expect(memcmp(saved, encoded, encoded_size) == 0,
                        "atomic save changed canonical project bytes") &&
                 passed;
    }
    fclose(file);
    remove(path);
    free(saved);
    return passed;
}

int main(void) {
    unsigned char* encoded = NULL;
    size_t encoded_size = 0;
    int passed = 1;
    encoded = create_empty_container(&encoded_size);
    if (encoded == NULL) {
        return 1;
    }
    passed = probe_reports_the_header_version(encoded, encoded_size) && passed;
    passed = normalize_is_deterministic_and_atomic(encoded, encoded_size) && passed;
    passed = bounded_read_refuses_the_input(encoded, encoded_size) && passed;
    passed = versioned_structures_reject_unknown_layouts(encoded, encoded_size) && passed;
    passed = future_content_is_reported_and_preserved(encoded, encoded_size) && passed;
    passed = complete_document_round_trips_losslessly() && passed;
    passed = atomic_save_publishes_canonical_bytes(encoded, encoded_size) && passed;
    free(encoded);
    return passed ? 0 : 1;
}
