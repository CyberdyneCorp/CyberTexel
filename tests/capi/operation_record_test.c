#include <ctex/capi.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int expect(int condition, const char* message) {
    if (!condition) {
        fprintf(stderr, "%s: %s\n", message, ctex_get_last_diagnostic());
    }
    return condition;
}

static ctex_operation_record_descriptor descriptor(uint8_t* pinned_bytes) {
    static const double defaults[] = {0.0, 0.0, 0.0};
    static ctex_operation_channel_descriptor channel;
    static ctex_pinned_operation_resource_descriptor resource;
    channel = (ctex_operation_channel_descriptor){
        .size = CTEX_OPERATION_CHANNEL_DESCRIPTOR_CURRENT_SIZE,
        .semantic_id = "pbr.base_color",
        .component_count = 3,
        .scalar_representation = CTEX_SCALAR_REPRESENTATION_UNSIGNED_NORMALIZED,
        .bit_depth = 8,
        .color_space = CTEX_COLOR_SPACE_SRGB_REC709,
        .default_value = defaults,
        .default_value_count = 3,
    };
    resource = (ctex_pinned_operation_resource_descriptor){
        .size = CTEX_PINNED_OPERATION_RESOURCE_DESCRIPTOR_CURRENT_SIZE,
        .role = "tip-alpha",
        .content_identity = "sha256:pinned-alpha",
        .bytes = pinned_bytes,
        .byte_count = 3,
    };
    ctex_operation_record_descriptor result = {
        .size = CTEX_OPERATION_RECORD_DESCRIPTOR_CURRENT_SIZE,
        .identifier = "operations/stroke-9",
        .algorithm_identifier = "cybertexel.paint.brush",
        .algorithm_version = 2,
        .preset_identifier = "brushes/ink",
        .preset_version = 4,
        .replay_class = CTEX_OPERATION_REPLAY_RESOLUTION_INDEPENDENT,
        .input_document_revision = 8,
        .seed = 99,
        .mesh_content_identity = "sha256:mesh",
        .coordinate_frame = {1, 0, 0, 4, 0, 1, 0, 5, 0, 0, 1, 6, 0, 0, 0, 1},
        .payload_kind = CTEX_OPERATION_PAYLOAD_RESOLVED_STAMPS,
        .payload_version = 1,
        .channels = &channel,
        .channel_count = 1,
        .pinned_resources = &resource,
        .pinned_resource_count = 1,
        .payload = "stamps-v1",
        .payload_size = 9,
    };
    return result;
}

int main(void) {
    uint8_t pinned_bytes[] = {0, 127, 255};
    ctex_operation_record_descriptor source = descriptor(pinned_bytes);
    ctex_operation_record_info record_info = {.size = CTEX_OPERATION_RECORD_INFO_CURRENT_SIZE};
    int passed = expect(ctex_operation_record_create(&source, &record_info, NULL, 0, NULL, 0) ==
                            CTEX_RESULT_SUCCESS,
                        "operation record sizing failed") &&
                 expect(record_info.canonical_size != 0 && record_info.report_size != 0 &&
                            record_info.pinned_resource_bytes == 3,
                        "operation record sizing metadata is wrong");
    void* record = malloc(record_info.canonical_size);
    char* record_report = malloc(record_info.report_size);
    if (record == NULL || record_report == NULL) return 1;
    passed = passed &&
             expect(ctex_operation_record_create(&source, &record_info, record,
                                                 record_info.canonical_size, record_report,
                                                 record_info.report_size) == CTEX_RESULT_SUCCESS,
                    "operation record creation failed") &&
             expect(strstr(record_report, "sha256:pinned-alpha") != NULL,
                    "operation record report omitted pinned content identity");
    const size_t record_size = record_info.canonical_size;
    pinned_bytes[0] = pinned_bytes[1] = pinned_bytes[2] = 42;

    size_t empty_size = 0;
    passed = passed && expect(ctex_project_container_create_empty(NULL, 0, &empty_size) ==
                                  CTEX_RESULT_SUCCESS,
                              "empty project sizing failed");
    void* empty = malloc(empty_size);
    if (empty == NULL) return 1;
    passed = passed && expect(ctex_project_container_create_empty(empty, empty_size, &empty_size) ==
                                  CTEX_RESULT_SUCCESS,
                              "empty project creation failed");

    ctex_project_container_info project_info = {.size = CTEX_PROJECT_CONTAINER_INFO_CURRENT_SIZE};
    passed = passed && expect(ctex_project_container_upsert_operation_record(
                                  empty, empty_size, NULL, record, record_size, &project_info, NULL,
                                  0, NULL, 0) == CTEX_RESULT_SUCCESS,
                              "operation record project sizing failed");
    void* project = malloc(project_info.canonical_size);
    char* project_report = malloc(project_info.report_size);
    if (project == NULL || project_report == NULL) return 1;
    passed =
        passed &&
        expect(ctex_project_container_upsert_operation_record(
                   empty, empty_size, NULL, record, record_size, &project_info, project,
                   project_info.canonical_size, project_report,
                   project_info.report_size) == CTEX_RESULT_SUCCESS,
               "operation record project insertion failed") &&
        expect(project_info.asset_count == 1 && strstr(project_report, "operation-record") != NULL,
               "project inventory omitted the operation record");

    size_t restored_size = 0;
    passed = passed && expect(ctex_project_container_get_operation_record(
                                  project, project_info.canonical_size, NULL, source.identifier,
                                  NULL, 0, &restored_size) == CTEX_RESULT_SUCCESS &&
                                  restored_size == record_size,
                              "operation record extraction sizing failed");
    void* restored = malloc(restored_size);
    if (restored == NULL) return 1;
    passed = passed &&
             expect(ctex_project_container_get_operation_record(
                        project, project_info.canonical_size, NULL, source.identifier, restored,
                        restored_size, &restored_size) == CTEX_RESULT_SUCCESS,
                    "operation record extraction failed") &&
             expect(memcmp(restored, record, record_size) == 0,
                    "project changed canonical operation-record bytes");

    ctex_operation_record_info inspected = {.size = CTEX_OPERATION_RECORD_INFO_CURRENT_SIZE};
    char* inspected_report = malloc(record_info.report_size);
    if (inspected_report == NULL) return 1;
    passed = passed &&
             expect(ctex_operation_record_inspect(restored, restored_size, &inspected, NULL, 0,
                                                  inspected_report,
                                                  record_info.report_size) == CTEX_RESULT_SUCCESS,
                    "operation record inspection failed") &&
             expect(inspected.input_document_revision == 8 && inspected.seed == 99 &&
                        strstr(inspected_report, "sha256:pinned-alpha") != NULL,
                    "pinned replay metadata changed after source mutation");

    static const char* checkpoints[] = {"checkpoints/stroke-9/base-color"};
    ctex_operation_record_descriptor replay_source = descriptor(pinned_bytes);
    replay_source.checkpoint_image_identifiers = checkpoints;
    replay_source.checkpoint_image_count = 1;
    ctex_operation_record_info replay_record_info = {
        .size = CTEX_OPERATION_RECORD_INFO_CURRENT_SIZE,
    };
    passed = passed && expect(ctex_operation_record_create(&replay_source, &replay_record_info,
                                                           NULL, 0, NULL, 0) == CTEX_RESULT_SUCCESS,
                              "checkpoint replay record sizing failed");
    void* replay_record = malloc(replay_record_info.canonical_size);
    if (replay_record == NULL) return 1;
    passed = passed &&
             expect(ctex_operation_record_create(&replay_source, &replay_record_info, replay_record,
                                                 replay_record_info.canonical_size, NULL,
                                                 0) == CTEX_RESULT_SUCCESS,
                    "checkpoint replay record creation failed");
    const size_t replay_record_size = replay_record_info.canonical_size;

    ctex_operation_algorithm_support_descriptor support = {
        .size = CTEX_OPERATION_ALGORITHM_SUPPORT_DESCRIPTOR_CURRENT_SIZE,
        .identifier = "cybertexel.paint.brush",
        .minimum_version = 2,
        .maximum_version = 2,
    };
    ctex_operation_replay_assessment_descriptor assessment = {
        .size = CTEX_OPERATION_REPLAY_ASSESSMENT_DESCRIPTOR_CURRENT_SIZE,
        .supported_algorithms = &support,
        .supported_algorithm_count = 1,
        .target_resolution_changed = 1,
    };
    ctex_operation_replay_info replay = {.size = CTEX_OPERATION_REPLAY_INFO_CURRENT_SIZE};
    passed =
        passed &&
        expect(ctex_operation_record_assess_replay(replay_record, replay_record_size, &assessment,
                                                   &replay, NULL, 0) == CTEX_RESULT_SUCCESS &&
                   replay.replay_available == 1 && replay.checkpoint_available == 1 &&
                   replay.disposition == CTEX_OPERATION_REPLAY_RESOLUTION_INDEPENDENT_AVAILABLE,
               "supported resolution-independent replay was not reported");
    support.minimum_version = 1;
    support.maximum_version = 1;
    replay.size = CTEX_OPERATION_REPLAY_INFO_CURRENT_SIZE;
    passed = passed && expect(ctex_operation_record_assess_replay(replay_record, replay_record_size,
                                                                  &assessment, &replay, NULL,
                                                                  0) == CTEX_RESULT_SUCCESS,
                              "unknown algorithm replay sizing failed");
    char* replay_report = malloc(replay.required_report_size);
    if (replay_report == NULL) return 1;
    const size_t replay_report_size = replay.required_report_size;
    replay.size = CTEX_OPERATION_REPLAY_INFO_CURRENT_SIZE;
    passed =
        passed && expect(ctex_operation_record_assess_replay(
                             replay_record, replay_record_size, &assessment, &replay, replay_report,
                             replay_report_size) == CTEX_RESULT_SUCCESS &&
                             replay.replay_available == 0 && replay.checkpoint_available == 1 &&
                             replay.disposition == CTEX_OPERATION_REPLAY_UNSUPPORTED_ALGORITHM &&
                             strstr(replay_report, "raster checkpoint retained") != NULL,
                         "unknown algorithm version did not report checkpoint fallback");

    ctex_resource_ledger* ledger = NULL;
    ctex_resource_reservation* reservation = NULL;
    ctex_resource_budget_limits budget = {
        .size = CTEX_RESOURCE_BUDGET_LIMITS_CURRENT_SIZE,
        .cpu_bytes = replay_record_size,
        .backing_store_bytes = 64,
    };
    ctex_resource_admission_report admission = {
        .size = CTEX_RESOURCE_ADMISSION_REPORT_CURRENT_SIZE,
    };
    passed = passed &&
             expect(ctex_resource_ledger_create(&ledger) == CTEX_RESULT_SUCCESS,
                    "resource ledger creation failed") &&
             expect(ctex_resource_ledger_admit_operation_recovery(
                        ledger, &budget, replay_record, replay_record_size, 64, &reservation,
                        &admission) == CTEX_RESULT_SUCCESS &&
                        admission.status == CTEX_RESOURCE_ADMITTED_WHOLE && reservation != NULL,
                    "bounded recovery data was not admitted");
    ctex_resource_reservation_destroy(reservation);
    reservation = NULL;
    budget.backing_store_bytes = 63;
    admission.size = CTEX_RESOURCE_ADMISSION_REPORT_CURRENT_SIZE;
    passed =
        passed && expect(ctex_resource_ledger_admit_operation_recovery(
                             ledger, &budget, replay_record, replay_record_size, 64, &reservation,
                             &admission) == CTEX_RESULT_SUCCESS &&
                             admission.status == CTEX_RESOURCE_OVER_BUDGET && reservation == NULL,
                         "over-budget recovery data was admitted");
    ctex_resource_ledger_destroy(ledger);

    source.replay_class = 99;
    record_info.size = CTEX_OPERATION_RECORD_INFO_CURRENT_SIZE;
    passed = passed && expect(ctex_operation_record_create(&source, &record_info, NULL, 0, NULL,
                                                           0) == CTEX_RESULT_INVALID_ARGUMENT,
                              "invalid replay class was accepted");

    free(inspected_report);
    free(replay_report);
    free(replay_record);
    free(restored);
    free(project_report);
    free(project);
    free(empty);
    free(record_report);
    free(record);
    return passed ? 0 : 1;
}
