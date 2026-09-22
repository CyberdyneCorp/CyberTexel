#include <ctex/capi.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char* recovery_directory = "ctex-capi-autosave-fixture";
static const char* recovery_path = "ctex-capi-autosave-fixture/document-c.ctex-recovery";
static const char* malformed_path = "ctex-capi-autosave-fixture/broken.ctex-recovery";

static int expect(int condition, const char* message) {
    if (!condition) {
        fprintf(stderr, "%s\n", message);
    }
    return condition;
}

static void release_reservation(void* user_data) {
    ctex_resource_reservation* reservation = (ctex_resource_reservation*)user_data;
    ctex_resource_reservation_destroy(reservation);
}

static unsigned char* empty_project(size_t* size) {
    unsigned char* bytes = NULL;
    if (!expect(ctex_project_container_create_empty(NULL, 0, size) == CTEX_RESULT_SUCCESS,
                "empty project sizing failed")) {
        return NULL;
    }
    bytes = (unsigned char*)malloc(*size);
    if (!expect(bytes != NULL, "empty project allocation failed") ||
        !expect(ctex_project_container_create_empty(bytes, *size, size) == CTEX_RESULT_SUCCESS,
                "empty project creation failed")) {
        free(bytes);
        return NULL;
    }
    return bytes;
}

static int autosave_coalesces_and_publishes(unsigned char* project, size_t project_size,
                                            ctex_project_autosave_session** out_session) {
    const ctex_project_autosave_config_descriptor config = {
        .size = CTEX_PROJECT_AUTOSAVE_CONFIG_DESCRIPTOR_CURRENT_SIZE,
        .recovery_directory = recovery_directory,
        .recovery_key = "document-c",
        .interval_milliseconds = 60000,
    };
    ctex_project_autosave_info info = {.size = CTEX_PROJECT_AUTOSAVE_INFO_CURRENT_SIZE};
    uint32_t status = UINT32_MAX;
    uint32_t idle = 1;
    char path[256] = {0};
    char error[128] = {0};

    if (!expect(ctex_project_autosave_session_create(&config, out_session) == CTEX_RESULT_SUCCESS,
                "autosave session creation failed") ||
        !expect(ctex_project_autosave_session_submit(*out_session, 1, project, project_size, NULL,
                                                     &status) == CTEX_RESULT_SUCCESS &&
                    status == CTEX_PROJECT_AUTOSAVE_QUEUED,
                "first autosave revision was not queued") ||
        !expect(ctex_project_autosave_session_submit(*out_session, 2, project, project_size, NULL,
                                                     &status) == CTEX_RESULT_SUCCESS &&
                    status == CTEX_PROJECT_AUTOSAVE_QUEUED,
                "newer autosave revision was not queued") ||
        !expect(ctex_project_autosave_session_submit(*out_session, 1, project, project_size, NULL,
                                                     &status) == CTEX_RESULT_SUCCESS &&
                    status == CTEX_PROJECT_AUTOSAVE_STALE_REVISION,
                "stale autosave revision was accepted")) {
        return 0;
    }

    project[0] ^= 0xffU;
    if (!expect(ctex_project_autosave_session_wait(*out_session, 0, &idle) == CTEX_RESULT_SUCCESS &&
                    idle == 0,
                "deferred autosave unexpectedly reported idle") ||
        !expect(ctex_project_autosave_session_get_info(*out_session, &info, NULL, 0, NULL, 0) ==
                        CTEX_RESULT_SUCCESS &&
                    info.has_pending_revision == 1 && info.pending_revision == 2 &&
                    info.required_recovery_path_size <= sizeof(path) &&
                    info.required_last_error_size <= sizeof(error),
                "queued autosave status is incorrect") ||
        !expect(ctex_project_autosave_session_flush(*out_session) == CTEX_RESULT_SUCCESS,
                "autosave flush failed")) {
        project[0] ^= 0xffU;
        return 0;
    }
    project[0] ^= 0xffU;
    info.size = CTEX_PROJECT_AUTOSAVE_INFO_CURRENT_SIZE;
    return expect(
               ctex_project_autosave_session_get_info(*out_session, &info, path, sizeof(path),
                                                      error, sizeof(error)) == CTEX_RESULT_SUCCESS,
               "saved autosave status query failed") &&
           expect(info.has_last_saved_revision == 1 && info.last_saved_revision == 2 &&
                      info.has_pending_revision == 0 && info.has_saving_revision == 0 &&
                      info.successful_writes == 1 && strcmp(path, recovery_path) == 0 &&
                      error[0] == '\0',
                  "saved autosave status is incorrect");
}

static int make_malformed_recovery(void) {
    FILE* file = fopen(malformed_path, "wb");
    if (file == NULL) {
        return 0;
    }
    fputs("bad", file);
    return fclose(file) == 0;
}

static int recovery_is_enumerable_and_readable(const unsigned char* project, size_t project_size) {
    ctex_project_recovery_enumeration_info enumeration = {
        .size = CTEX_PROJECT_RECOVERY_ENUMERATION_INFO_CURRENT_SIZE};
    ctex_project_recovery_entry recoverable[2] = {{0}};
    ctex_project_recovery_rejection rejected[2] = {{0}};
    ctex_project_container_info container = {.size = CTEX_PROJECT_CONTAINER_INFO_CURRENT_SIZE};
    ctex_project_recovery_checkpoint_info checkpoint = {
        .size = CTEX_PROJECT_RECOVERY_CHECKPOINT_INFO_CURRENT_SIZE};
    char strings[1024] = {0};
    unsigned char* opened = NULL;
    char* report = NULL;
    int passed = 0;

    if (!expect(make_malformed_recovery(), "could not create malformed recovery fixture") ||
        !expect(ctex_project_recovery_enumerate(recovery_directory, &enumeration, NULL, 0, NULL, 0,
                                                NULL, 0) == CTEX_RESULT_SUCCESS,
                "recovery enumeration sizing failed") ||
        !expect(enumeration.required_recoverable_count == 1 &&
                    enumeration.required_rejected_count == 1 &&
                    enumeration.required_string_size <= sizeof(strings),
                "recovery enumeration counts are incorrect") ||
        !expect(ctex_project_recovery_enumerate(recovery_directory, &enumeration, recoverable, 2,
                                                rejected, 2, strings,
                                                sizeof(strings)) == CTEX_RESULT_SUCCESS,
                "recovery enumeration copy failed") ||
        !expect(strcmp(strings + recoverable[0].recovery_key_offset, "document-c") == 0 &&
                    strcmp(strings + recoverable[0].path_offset, recovery_path) == 0 &&
                    recoverable[0].schema.major == ctex_get_version().major &&
                    strstr(strings + rejected[0].path_offset, "broken.ctex-recovery") != NULL &&
                    rejected[0].message_size > 1,
                "recovery entries omitted identity, schema, or rejection details") ||
        !expect(ctex_project_recovery_read(recovery_path, NULL, &container, NULL, 0, NULL, 0) ==
                        CTEX_RESULT_SUCCESS &&
                    container.canonical_size == project_size && container.report_size > 1,
                "recovery read sizing failed")) {
        goto cleanup;
    }
    opened = (unsigned char*)malloc(container.canonical_size);
    report = (char*)malloc(container.report_size);
    if (!expect(opened != NULL && report != NULL, "recovery output allocation failed") ||
        !expect(ctex_project_recovery_resume(recovery_path, NULL, &checkpoint, &container, opened,
                                             container.canonical_size, report,
                                             container.report_size) == CTEX_RESULT_SUCCESS,
                "recovery read failed") ||
        !expect(checkpoint.has_revision == 1 && checkpoint.revision == 2 &&
                    memcmp(opened, project, project_size) == 0 &&
                    strstr(report, "\"images\":[]") != NULL,
                "recovery content did not round-trip canonically")) {
        goto cleanup;
    }
    passed = 1;

cleanup:
    free(report);
    free(opened);
    return passed;
}

static int lifecycle_quiesce_reports_the_last_durable_revision(
    ctex_project_autosave_session* autosave) {
    ctex_resource_ledger* ledger = NULL;
    ctex_resource_reservation* reservation = NULL;
    ctex_resource_requirement requirement = {
        .size = CTEX_RESOURCE_REQUIREMENT_CURRENT_SIZE,
        .category = CTEX_RESOURCE_TEMPORARY,
        .physical_bytes = 64,
        .roles = CTEX_RESOURCE_CPU_RESIDENT,
    };
    ctex_resource_admission_descriptor admission = {
        .size = CTEX_RESOURCE_ADMISSION_DESCRIPTOR_CURRENT_SIZE,
        .operation = "in-flight edit",
        .limits = {.size = CTEX_RESOURCE_BUDGET_LIMITS_CURRENT_SIZE,
                   .cpu_bytes = 1024,
                   .gpu_bytes = 1024,
                   .backing_store_bytes = 1024,
                   .temporary_bytes = 1024},
        .fixed_requirements = &requirement,
        .fixed_requirement_count = 1,
    };
    ctex_resource_admission_report admission_report = {
        .size = CTEX_RESOURCE_ADMISSION_REPORT_CURRENT_SIZE};
    ctex_project_quiesce_report quiesce = {.size = CTEX_PROJECT_QUIESCE_REPORT_CURRENT_SIZE};
    int passed = expect(ctex_resource_ledger_create(&ledger) == CTEX_RESULT_SUCCESS,
                        "lifecycle resource ledger creation failed") &&
                 expect(ctex_resource_ledger_admit(ledger, &admission, &reservation,
                                                   &admission_report) == CTEX_RESULT_SUCCESS &&
                            reservation != NULL,
                        "lifecycle in-flight edit was not admitted");
    ctex_project_quiesce_descriptor descriptor = {
        .size = CTEX_PROJECT_QUIESCE_DESCRIPTOR_CURRENT_SIZE,
        .current_revision = 3,
        .deadline_milliseconds = 0,
        .request_cancel = release_reservation,
        .user_data = reservation,
    };
    passed = passed &&
             expect(ctex_project_lifecycle_quiesce(ledger, autosave, &descriptor, &quiesce) ==
                            CTEX_RESULT_SUCCESS &&
                        quiesce.status == CTEX_PROJECT_QUIESCE_DEADLINE_EXCEEDED &&
                        quiesce.admissions_stopped == 1 && quiesce.cancellation_requested == 1 &&
                        quiesce.work_drained == 1 && quiesce.active_operation_count == 0 &&
                        quiesce.has_durable_revision == 1 && quiesce.durable_revision == 2 &&
                        quiesce.has_uncheckpointed_range == 1 &&
                        quiesce.uncheckpointed_first_revision == 3 &&
                        quiesce.uncheckpointed_last_revision == 3,
                    "quiesce did not report its deadline and durable revision exactly");
    reservation = NULL;
    ctex_resource_reservation* refused = NULL;
    admission_report.size = CTEX_RESOURCE_ADMISSION_REPORT_CURRENT_SIZE;
    passed = passed &&
             expect(ctex_resource_ledger_admit(ledger, &admission, &refused, &admission_report) ==
                            CTEX_RESULT_SUCCESS &&
                        refused == NULL && admission_report.status == CTEX_RESOURCE_QUIESCING,
                    "quiesce admitted new edit work") &&
             expect(ctex_project_lifecycle_resume(ledger) == CTEX_RESULT_SUCCESS,
                    "lifecycle resume failed") &&
             expect(ctex_resource_ledger_admit(ledger, &admission, &reservation,
                                               &admission_report) == CTEX_RESULT_SUCCESS &&
                        reservation != NULL,
                    "lifecycle resume did not restore edit admission");
    ctex_resource_reservation_destroy(reservation);
    descriptor.current_revision = 2;
    descriptor.request_cancel = NULL;
    descriptor.user_data = NULL;
    quiesce.size = CTEX_PROJECT_QUIESCE_REPORT_CURRENT_SIZE;
    passed =
        passed &&
        expect(ctex_project_lifecycle_quiesce(ledger, autosave, &descriptor, &quiesce) ==
                       CTEX_RESULT_SUCCESS &&
                   quiesce.status == CTEX_PROJECT_QUIESCE_DURABLE && quiesce.work_drained == 1 &&
                   quiesce.durable_revision == 2 && quiesce.has_uncheckpointed_range == 0,
               "quiesce did not report an already durable checkpoint") &&
        expect(ctex_project_lifecycle_resume(ledger) == CTEX_RESULT_SUCCESS,
               "lifecycle did not resume after a durable checkpoint");
    ctex_resource_ledger_destroy(ledger);
    return passed;
}

static int invalid_configuration_is_refused(void) {
    const ctex_project_autosave_config_descriptor config = {
        .size = CTEX_PROJECT_AUTOSAVE_CONFIG_DESCRIPTOR_CURRENT_SIZE,
        .recovery_directory = recovery_directory,
        .recovery_key = "../escape",
        .interval_milliseconds = 1,
    };
    ctex_project_autosave_session* session = NULL;
    return expect(ctex_project_autosave_session_create(&config, &session) ==
                      CTEX_RESULT_INVALID_ARGUMENT,
                  "unsafe autosave recovery key was accepted") &&
           expect(session == NULL, "failed autosave creation published a handle");
}

int main(void) {
    ctex_project_autosave_session* session = NULL;
    size_t project_size = 0;
    unsigned char* project = empty_project(&project_size);
    int passed = project != NULL &&
                 autosave_coalesces_and_publishes(project, project_size, &session) &&
                 lifecycle_quiesce_reports_the_last_durable_revision(session) &&
                 recovery_is_enumerable_and_readable(project, project_size) &&
                 invalid_configuration_is_refused();
    ctex_project_autosave_session_destroy(session);
    free(project);
    remove(malformed_path);
    remove(recovery_path);
    remove(recovery_directory);
    return passed ? 0 : 1;
}
