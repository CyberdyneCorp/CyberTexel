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

static ctex_host_resource_descriptor resource(const char* logical_id, uint64_t generation,
                                              const char* role, uint32_t output) {
    const ctex_host_resource_descriptor value = {
        .size = CTEX_HOST_RESOURCE_DESCRIPTOR_CURRENT_SIZE,
        .logical_id = logical_id,
        .generation = generation,
        .role = role,
        .format = CTEX_EXECUTOR_TEXTURE_RGBA8_UNORM,
        .width = 64,
        .height = 32,
        .layers = 1,
        .mip_levels = 1,
        .tile_width = 64,
        .tile_height = 32,
        .externally_initialized = output ? 0U : 1U,
        .owner = CTEX_HOST_RESOURCE_HOST,
        .required_state =
            output ? CTEX_HOST_RESOURCE_RENDER_TARGET : CTEX_HOST_RESOURCE_SHADER_READ,
        .output = output,
    };
    return value;
}

static ctex_host_completed_resource_descriptor completed(const char* logical_id,
                                                         uint64_t generation) {
    const ctex_host_completed_resource_descriptor value = {
        .size = CTEX_HOST_COMPLETED_RESOURCE_DESCRIPTOR_CURRENT_SIZE,
        .logical_id = logical_id,
        .generation = generation,
        .format = CTEX_EXECUTOR_TEXTURE_RGBA8_UNORM,
        .width = 64,
        .height = 32,
        .layers = 1,
    };
    return value;
}

static int read_completion(const ctex_host_completion_result* result, uint32_t disposition,
                           uint64_t token, uint32_t has_revision, uint64_t revision,
                           const char* released_id, uint64_t released_generation) {
    ctex_host_completion_result_info info = {
        .size = CTEX_HOST_COMPLETION_RESULT_INFO_CURRENT_SIZE,
    };
    if (!expect(ctex_host_completion_result_get_info(result, &info, NULL, 0, NULL, 0, NULL, 0) ==
                    CTEX_RESULT_SUCCESS,
                "completion sizing query failed")) {
        return 0;
    }
    ctex_host_resource_version* released =
        info.released_resource_count == 0
            ? NULL
            : (ctex_host_resource_version*)malloc(info.released_resource_count *
                                                  sizeof(ctex_host_resource_version));
    char* identities = info.required_released_identity_size == 0
                           ? NULL
                           : (char*)malloc(info.required_released_identity_size);
    char* message = (char*)malloc(info.required_message_size);
    const int allocated = (info.released_resource_count == 0 || released != NULL) &&
                          (info.required_released_identity_size == 0 || identities != NULL) &&
                          message != NULL;
    const int queried = allocated && ctex_host_completion_result_get_info(
                                         result, &info, released, info.released_resource_count,
                                         identities, info.required_released_identity_size, message,
                                         info.required_message_size) == CTEX_RESULT_SUCCESS;
    int passed = expect(queried, "completion result read failed");
    passed = expect(info.disposition == disposition && info.completion_token == token &&
                        info.has_published_revision == has_revision &&
                        info.published_revision == revision && info.required_message_size > 1,
                    "completion result metadata is incorrect") &&
             passed;
    if (released_id == NULL) {
        passed = expect(info.released_resource_count == 0,
                        "completion released an unexpected resource") &&
                 passed;
    } else {
        passed = expect(info.released_resource_count == 1 && released != NULL &&
                            identities != NULL && released[0].logical_id_offset == 0 &&
                            released[0].generation == released_generation &&
                            strcmp(identities + released[0].logical_id_offset, released_id) == 0,
                        "completion released-resource identity is incorrect") &&
                 passed;
    }
    free(message);
    free(identities);
    free(released);
    return passed;
}

static int submit_pair(ctex_host_execution_session* session, uint64_t revision,
                       const char* input_id, uint64_t input_generation, const char* output_id,
                       uint64_t output_generation, uint32_t replay_semantics,
                       ctex_host_submission_info* out_info) {
    const ctex_host_resource_descriptor resources[] = {
        resource(input_id, input_generation, "input", 0),
        resource(output_id, output_generation, "output", 1),
    };
    const ctex_host_submission_descriptor submission = {
        .size = CTEX_HOST_SUBMISSION_DESCRIPTOR_CURRENT_SIZE,
        .operation = "paint",
        .base_revision = revision,
        .resources = resources,
        .resource_count = 2,
        .replay_semantics = replay_semantics,
    };
    out_info->size = CTEX_HOST_SUBMISSION_INFO_CURRENT_SIZE;
    return ctex_host_execution_session_submit(session, &submission, out_info) ==
           CTEX_RESULT_SUCCESS;
}

static int complete_success(ctex_host_execution_session* session, uint64_t token,
                            const char* output_id, uint64_t output_generation,
                            const ctex_host_recovery_descriptor* recovery,
                            ctex_host_completion_result** out_result) {
    const ctex_host_completed_resource_descriptor output = completed(output_id, output_generation);
    const ctex_host_completion_descriptor completion = {
        .size = CTEX_HOST_COMPLETION_DESCRIPTOR_CURRENT_SIZE,
        .completion_token = token,
        .status = CTEX_HOST_EXECUTION_SUCCEEDED,
        .outputs = &output,
        .output_count = 1,
        .recovery = recovery,
        .detail = NULL,
    };
    return ctex_host_execution_session_complete(session, &completion, out_result) ==
           CTEX_RESULT_SUCCESS;
}

static int publication_requires_recovery(ctex_host_execution_session* session) {
    ctex_host_submission_info submission = {0};
    int passed = expect(submit_pair(session, 0, "source", 1, "paint", 1,
                                    CTEX_HOST_REPLAY_DETERMINISTIC, &submission),
                        "deterministic host submission failed") &&
                 expect(submission.completion_token == 1 && submission.base_revision == 0 &&
                            submission.resource_count == 2,
                        "host submission metadata is incorrect");
    const ctex_host_recovery_descriptor recovery = {
        .size = CTEX_HOST_RECOVERY_DESCRIPTOR_CURRENT_SIZE,
        .kind = CTEX_HOST_RECOVERY_DETERMINISTIC_RECORD,
        .checkpoint_complete = 1,
        .checkpoint_revision = 0,
        .operation_record_version = "paint-v1",
        .inputs_pinned = 1,
        .retained_bytes = 96,
    };
    ctex_host_completion_result* result = NULL;
    passed = expect(complete_success(session, submission.completion_token, "paint", 1, &recovery,
                                     &result),
                    "recoverable host completion failed") &&
             passed;
    passed =
        read_completion(result, CTEX_HOST_COMPLETION_PUBLISHED, 1, 1, 1, "source", 1) && passed;
    ctex_host_completion_result_destroy(result);

    uint32_t found = 0;
    uint32_t held = 0;
    uint64_t generation = 0;
    passed = expect(ctex_host_execution_session_get_committed_resource(
                        session, "paint", &found, &generation) == CTEX_RESULT_SUCCESS &&
                        found == 1 && generation == 1 &&
                        ctex_host_execution_session_resource_is_held(session, "paint", 1, &held) ==
                            CTEX_RESULT_SUCCESS &&
                        held == 1,
                    "published resource authority was not retained by identity") &&
             passed;

    result = NULL;
    passed = expect(complete_success(session, submission.completion_token, "paint", 1, &recovery,
                                     &result),
                    "duplicate completion query failed") &&
             read_completion(result, CTEX_HOST_COMPLETION_DUPLICATE, 1, 0, 0, NULL, 0) && passed;
    ctex_host_completion_result_destroy(result);
    return passed;
}

static int deferred_recovery_and_cancellation(ctex_host_execution_session* session) {
    ctex_host_submission_info submission = {0};
    int passed = expect(submit_pair(session, 1, "paint", 1, "paint", 2,
                                    CTEX_HOST_REPLAY_CHECKPOINT_ONLY, &submission),
                        "checkpoint-only submission failed");
    const ctex_host_recovery_descriptor invalid_recovery = {
        .size = CTEX_HOST_RECOVERY_DESCRIPTOR_CURRENT_SIZE,
        .kind = CTEX_HOST_RECOVERY_DETERMINISTIC_RECORD,
        .checkpoint_complete = 1,
        .checkpoint_revision = 1,
        .operation_record_version = "paint-v1",
        .inputs_pinned = 1,
        .retained_bytes = 64,
    };
    ctex_host_completion_result* result = NULL;
    passed = expect(complete_success(session, submission.completion_token, "paint", 2,
                                     &invalid_recovery, &result),
                    "checkpoint-only completion call failed") &&
             read_completion(result, CTEX_HOST_COMPLETION_AWAITING_RECOVERY,
                             submission.completion_token, 0, 0, NULL, 0) &&
             passed;
    ctex_host_completion_result_destroy(result);

    const ctex_host_recovery_descriptor checkpoint = {
        .size = CTEX_HOST_RECOVERY_DESCRIPTOR_CURRENT_SIZE,
        .kind = CTEX_HOST_RECOVERY_RESULT_CHECKPOINT,
        .checkpoint_complete = 1,
        .checkpoint_revision = 2,
        .retained_bytes = 128,
    };
    result = NULL;
    passed = expect(ctex_host_execution_session_establish_recovery(
                        session, submission.completion_token, &checkpoint, &result) ==
                        CTEX_RESULT_SUCCESS,
                    "deferred checkpoint recovery failed") &&
             read_completion(result, CTEX_HOST_COMPLETION_PUBLISHED, submission.completion_token, 1,
                             2, "paint", 1) &&
             passed;
    ctex_host_completion_result_destroy(result);

    passed = expect(submit_pair(session, 2, "paint", 2, "cancelled-output", 1,
                                CTEX_HOST_REPLAY_DETERMINISTIC, &submission),
                    "cancellable submission failed") &&
             passed;
    uint32_t cancelled = 0;
    passed = expect(ctex_host_execution_session_cancel(session, submission.completion_token,
                                                       &cancelled) == CTEX_RESULT_SUCCESS &&
                        cancelled == 1,
                    "host cancellation failed") &&
             passed;
    result = NULL;
    passed = expect(complete_success(session, submission.completion_token, "cancelled-output", 1,
                                     NULL, &result),
                    "late cancelled completion call failed") &&
             read_completion(result, CTEX_HOST_COMPLETION_CANCELLED, submission.completion_token, 0,
                             0, "cancelled-output", 1) &&
             passed;
    ctex_host_completion_result_destroy(result);
    return passed;
}

static int device_loss_cancels_pending_work(ctex_host_execution_session* session) {
    ctex_host_submission_info submission = {0};
    int passed = expect(submit_pair(session, 2, "paint", 2, "pending-output", 1,
                                    CTEX_HOST_REPLAY_DETERMINISTIC, &submission),
                        "pending submission failed");
    ctex_host_recovery_report* report = NULL;
    passed = expect(ctex_host_execution_session_report_device_loss(session, &report) ==
                            CTEX_RESULT_SUCCESS &&
                        report != NULL,
                    "device-loss report failed") &&
             passed;
    ctex_host_device_loss_info info = {.size = CTEX_HOST_DEVICE_LOSS_INFO_CURRENT_SIZE};
    passed = expect(ctex_host_recovery_report_get_info(report, &info, NULL, 0, NULL, 0, NULL, 0) ==
                            CTEX_RESULT_SUCCESS &&
                        info.recovered_revision == 2 && info.cancelled_submission_count == 1 &&
                        info.released_resource_count == 1 && info.retained_recovery_bytes == 128 &&
                        info.restored == 1,
                    "device-loss sizing report is incorrect") &&
             passed;
    uint64_t cancelled[1] = {0};
    ctex_host_resource_version released[1] = {{0}};
    char identities[32] = {0};
    passed =
        expect(
            ctex_host_recovery_report_get_info(report, &info, cancelled, 1, released, 1, identities,
                                               sizeof(identities)) == CTEX_RESULT_SUCCESS &&
                cancelled[0] == submission.completion_token && released[0].generation == 1 &&
                strcmp(identities + released[0].logical_id_offset, "pending-output") == 0,
            "device loss did not return cancelled work and released identities") &&
        passed;
    ctex_host_recovery_report_destroy(report);
    return passed;
}

static int invalid_descriptor_is_atomic(ctex_host_execution_session* session) {
    const ctex_host_resource_descriptor output = resource("invalid", 1, "output", 1);
    const ctex_host_submission_descriptor descriptor = {
        .size = CTEX_HOST_SUBMISSION_DESCRIPTOR_CURRENT_SIZE,
        .operation = "paint",
        .base_revision = 2,
        .resources = &output,
        .resource_count = 1,
        .replay_semantics = CTEX_HOST_REPLAY_DETERMINISTIC,
    };
    ctex_host_submission_info info = {.size = 0};
    const int refused = ctex_host_execution_session_submit(session, &descriptor, &info) ==
                            CTEX_RESULT_INVALID_ARGUMENT &&
                        ctex_get_last_diagnostic_code() == CTEX_DIAGNOSTIC_INVALID_DESCRIPTOR_SIZE;
    ctex_host_execution_session_info session_info = {
        .size = CTEX_HOST_EXECUTION_SESSION_INFO_CURRENT_SIZE,
    };
    return expect(
        refused &&
            ctex_host_execution_session_get_info(session, &session_info) == CTEX_RESULT_SUCCESS &&
            session_info.revision == 2 && session_info.active_submission_count == 0 &&
            session_info.retained_recovery_bytes == 128,
        "invalid descriptor mutated the host execution session");
}

static int wrong_output_size_is_rejected(void) {
    ctex_host_execution_session* session = NULL;
    if (!expect(ctex_host_execution_session_create(0, &session) == CTEX_RESULT_SUCCESS,
                "size-validation session creation failed")) {
        return 0;
    }
    const ctex_host_resource_descriptor output = resource("sized-output", 1, "output", 1);
    const ctex_host_submission_descriptor descriptor = {
        .size = CTEX_HOST_SUBMISSION_DESCRIPTOR_CURRENT_SIZE,
        .operation = "paint",
        .base_revision = 0,
        .resources = &output,
        .resource_count = 1,
        .replay_semantics = CTEX_HOST_REPLAY_DETERMINISTIC,
    };
    ctex_host_submission_info submission = {.size = CTEX_HOST_SUBMISSION_INFO_CURRENT_SIZE};
    int passed = expect(ctex_host_execution_session_submit(session, &descriptor, &submission) ==
                            CTEX_RESULT_SUCCESS,
                        "size-validation submission failed");
    ctex_host_completed_resource_descriptor returned = completed("sized-output", 1);
    returned.width = 63;
    const ctex_host_completion_descriptor completion = {
        .size = CTEX_HOST_COMPLETION_DESCRIPTOR_CURRENT_SIZE,
        .completion_token = submission.completion_token,
        .status = CTEX_HOST_EXECUTION_SUCCEEDED,
        .outputs = &returned,
        .output_count = 1,
    };
    ctex_host_completion_result* result = NULL;
    passed = expect(ctex_host_execution_session_complete(session, &completion, &result) ==
                        CTEX_RESULT_SUCCESS,
                    "wrong-size completion call failed") &&
             read_completion(result, CTEX_HOST_COMPLETION_REJECTED, submission.completion_token, 0,
                             0, "sized-output", 1) &&
             passed;
    ctex_host_execution_session_info info = {
        .size = CTEX_HOST_EXECUTION_SESSION_INFO_CURRENT_SIZE,
    };
    passed = expect(ctex_host_execution_session_get_info(session, &info) == CTEX_RESULT_SUCCESS &&
                        info.revision == 0 && info.active_submission_count == 0,
                    "wrong-size completion changed the session revision") &&
             passed;
    ctex_host_completion_result_destroy(result);
    ctex_host_execution_session_destroy(session);
    return passed;
}

int main(void) {
    ctex_host_execution_session* session = NULL;
    int passed = expect(
        ctex_host_execution_session_create(0, &session) == CTEX_RESULT_SUCCESS && session != NULL,
        "host execution session creation failed");
    ctex_host_execution_session_info info = {
        .size = CTEX_HOST_EXECUTION_SESSION_INFO_CURRENT_SIZE,
    };
    passed = expect(ctex_host_execution_session_get_info(session, &info) == CTEX_RESULT_SUCCESS &&
                        info.revision == 0 && info.active_submission_count == 0,
                    "initial host execution session state is incorrect") &&
             passed;
    passed = publication_requires_recovery(session) && passed;
    passed = deferred_recovery_and_cancellation(session) && passed;
    passed = device_loss_cancels_pending_work(session) && passed;
    passed = invalid_descriptor_is_atomic(session) && passed;
    ctex_host_execution_session_destroy(session);
    passed = wrong_output_size_is_rejected() && passed;
    return passed ? 0 : 1;
}
