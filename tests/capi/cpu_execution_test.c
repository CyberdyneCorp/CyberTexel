#include <ctex/capi.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct execution_state {
    unsigned char* document;
    size_t document_size;
    atomic_size_t executed;
    atomic_size_t latest_progress;
    atomic_size_t progress_reports;
    atomic_size_t commits;
    size_t cancel_after;
    ctex_result work_result;
} execution_state;

static int expect(int condition, const char* message) {
    if (!condition) {
        fprintf(stderr, "%s\n", message);
    }
    return condition;
}

static ctex_result execute_byte(size_t work_item, void* shared_working_memory,
                                size_t shared_working_memory_size, void* worker_working_memory,
                                size_t worker_working_memory_size, void* user_data) {
    execution_state* state = (execution_state*)user_data;
    atomic_fetch_add_explicit(&state->executed, 1, memory_order_relaxed);
    if (state->work_result != CTEX_RESULT_SUCCESS) {
        return state->work_result;
    }
    if (worker_working_memory_size != 0) {
        ((unsigned char*)worker_working_memory)[0] = 0x5a;
    }
    if (work_item < shared_working_memory_size) {
        ((unsigned char*)shared_working_memory)[work_item] = (unsigned char)((work_item % 251) + 1);
    }
    return CTEX_RESULT_SUCCESS;
}

static ctex_result commit_bytes(const void* shared_working_memory,
                                size_t shared_working_memory_size, void* user_data) {
    execution_state* state = (execution_state*)user_data;
    const size_t copy_size = state->document_size < shared_working_memory_size
                                 ? state->document_size
                                 : shared_working_memory_size;
    memcpy(state->document, shared_working_memory, copy_size);
    atomic_fetch_add_explicit(&state->commits, 1, memory_order_relaxed);
    return CTEX_RESULT_SUCCESS;
}

static uint32_t cancel_at_threshold(void* user_data) {
    execution_state* state = (execution_state*)user_data;
    return atomic_load_explicit(&state->latest_progress, memory_order_relaxed) >=
           state->cancel_after;
}

static void record_progress(size_t completed_work_items, size_t total_work_items, void* user_data) {
    execution_state* state = (execution_state*)user_data;
    (void)total_work_items;
    atomic_store_explicit(&state->latest_progress, completed_work_items, memory_order_relaxed);
    atomic_fetch_add_explicit(&state->progress_reports, 1, memory_order_relaxed);
}

static ctex_cpu_bounded_execution_descriptor descriptor_for(
    execution_state* state, size_t work_items, size_t shared_bytes, size_t worker_bytes,
    size_t maximum_workers, size_t memory_ceiling, size_t progress_interval) {
    const ctex_cpu_bounded_execution_descriptor descriptor = {
        .size = CTEX_CPU_BOUNDED_EXECUTION_DESCRIPTOR_CURRENT_SIZE,
        .operation = "staged-fill",
        .work_item_count = work_items,
        .shared_working_memory_bytes = shared_bytes,
        .working_memory_bytes_per_worker = worker_bytes,
        .maximum_workers = maximum_workers,
        .memory_ceiling_bytes = memory_ceiling,
        .progress_interval = progress_interval,
        .execute_work_item = execute_byte,
        .commit = commit_bytes,
        .is_cancelled = NULL,
        .report_progress = NULL,
        .user_data = state,
    };
    return descriptor;
}

static int read_result(const ctex_cpu_execution_result* result, uint32_t expected_status,
                       size_t expected_completed, size_t expected_total, size_t expected_memory,
                       size_t expected_workers, const char* message_fragment) {
    ctex_cpu_execution_info info = {.size = CTEX_CPU_EXECUTION_INFO_CURRENT_SIZE};
    if (!expect(ctex_cpu_execution_result_get_info(result, &info, NULL, 0) == CTEX_RESULT_SUCCESS,
                "CPU execution sizing query failed")) {
        return 0;
    }
    char* message = (char*)malloc(info.required_message_size);
    const int queried = message != NULL && ctex_cpu_execution_result_get_info(
                                               result, &info, message,
                                               info.required_message_size) == CTEX_RESULT_SUCCESS;
    const int passed = expect(queried && info.status == expected_status &&
                                  info.completed_work_items == expected_completed &&
                                  info.total_work_items == expected_total &&
                                  info.required_memory_bytes == expected_memory &&
                                  info.worker_count == expected_workers &&
                                  strstr(message, message_fragment) != NULL,
                              "CPU execution result metadata is incorrect");
    free(message);
    return passed;
}

static int worker_bounds_are_reproducible(void) {
    unsigned char serial_document[64] = {0};
    unsigned char parallel_document[64] = {0};
    execution_state serial = {
        .document = serial_document,
        .document_size = sizeof(serial_document),
        .cancel_after = SIZE_MAX,
        .work_result = CTEX_RESULT_SUCCESS,
    };
    execution_state parallel = {
        .document = parallel_document,
        .document_size = sizeof(parallel_document),
        .cancel_after = SIZE_MAX,
        .work_result = CTEX_RESULT_SUCCESS,
    };
    ctex_cpu_bounded_execution_descriptor serial_descriptor =
        descriptor_for(&serial, 64, 64, 8, 1, 72, 16);
    ctex_cpu_bounded_execution_descriptor parallel_descriptor =
        descriptor_for(&parallel, 64, 64, 8, 4, 96, 10);
    parallel_descriptor.report_progress = record_progress;
    ctex_cpu_execution_result* serial_result = NULL;
    ctex_cpu_execution_result* parallel_result = NULL;
    int passed =
        expect(ctex_cpu_execute_bounded(&serial_descriptor, &serial_result) == CTEX_RESULT_SUCCESS,
               "single-worker CPU execution failed") &&
        expect(
            ctex_cpu_execute_bounded(&parallel_descriptor, &parallel_result) == CTEX_RESULT_SUCCESS,
            "multi-worker CPU execution failed");
    passed = read_result(serial_result, CTEX_CPU_EXECUTION_COMPLETED, 64, 64, 72, 1, "completed") &&
             passed;
    passed =
        read_result(parallel_result, CTEX_CPU_EXECUTION_COMPLETED, 64, 64, 96, 4, "completed") &&
        passed;
    passed = expect(memcmp(serial_document, parallel_document, sizeof(serial_document)) == 0 &&
                        atomic_load(&serial.commits) == 1 && atomic_load(&parallel.commits) == 1 &&
                        atomic_load(&parallel.latest_progress) == 64 &&
                        atomic_load(&parallel.progress_reports) > 1,
                    "worker bounds changed output or omitted progress") &&
             passed;
    ctex_cpu_execution_result_destroy(parallel_result);
    ctex_cpu_execution_result_destroy(serial_result);
    return passed;
}

static int cancellation_discards_staged_output(void) {
    const size_t extent = 16384;
    unsigned char* document = (unsigned char*)malloc(extent);
    unsigned char* before = (unsigned char*)malloc(extent);
    if (!expect(document != NULL && before != NULL, "cancellation fixture allocation failed")) {
        free(before);
        free(document);
        return 0;
    }
    memset(document, 0x7f, extent);
    memcpy(before, document, extent);
    execution_state state = {
        .document = document,
        .document_size = extent,
        .cancel_after = extent / 2,
        .work_result = CTEX_RESULT_SUCCESS,
    };
    ctex_cpu_bounded_execution_descriptor descriptor =
        descriptor_for(&state, extent, extent, 16, 4, extent + 64, 256);
    descriptor.is_cancelled = cancel_at_threshold;
    descriptor.report_progress = record_progress;
    ctex_cpu_execution_result* result = NULL;
    int passed = expect(ctex_cpu_execute_bounded(&descriptor, &result) == CTEX_RESULT_SUCCESS,
                        "cancelled CPU execution call failed");
    ctex_cpu_execution_info info = {.size = CTEX_CPU_EXECUTION_INFO_CURRENT_SIZE};
    passed =
        expect(ctex_cpu_execution_result_get_info(result, &info, NULL, 0) == CTEX_RESULT_SUCCESS &&
                   info.status == CTEX_CPU_EXECUTION_CANCELLED &&
                   info.completed_work_items >= extent / 2 && info.completed_work_items < extent &&
                   info.worker_count == 4 && atomic_load(&state.commits) == 0 &&
                   memcmp(document, before, extent) == 0,
               "cancellation published staged bytes or reported the wrong outcome") &&
        passed;
    ctex_cpu_execution_result_destroy(result);
    free(before);
    free(document);
    return passed;
}

static int memory_ceiling_refuses_before_work(void) {
    unsigned char document[100] = {0};
    execution_state state = {
        .document = document,
        .document_size = sizeof(document),
        .cancel_after = SIZE_MAX,
        .work_result = CTEX_RESULT_SUCCESS,
    };
    ctex_cpu_bounded_execution_descriptor descriptor =
        descriptor_for(&state, 100, 100, 8, 4, 131, 16);
    ctex_cpu_execution_result* result = NULL;
    int passed = expect(ctex_cpu_execute_bounded(&descriptor, &result) == CTEX_RESULT_SUCCESS,
                        "memory refusal execution call failed");
    passed =
        read_result(result, CTEX_CPU_EXECUTION_MEMORY_CEILING_EXCEEDED, 0, 100, 132, 0, "131") &&
        expect(atomic_load(&state.executed) == 0 && atomic_load(&state.commits) == 0,
               "over-budget work ran before refusal") &&
        passed;
    ctex_cpu_execution_result_destroy(result);
    return passed;
}

static int callback_failure_and_bad_descriptor_are_atomic(void) {
    unsigned char document[8];
    memset(document, 0x3c, sizeof(document));
    execution_state state = {
        .document = document,
        .document_size = sizeof(document),
        .cancel_after = SIZE_MAX,
        .work_result = CTEX_RESULT_INVALID_ARGUMENT,
    };
    ctex_cpu_bounded_execution_descriptor descriptor = descriptor_for(&state, 8, 8, 0, 1, 8, 1);
    ctex_cpu_execution_result* result = (ctex_cpu_execution_result*)1;
    int passed =
        expect(ctex_cpu_execute_bounded(&descriptor, &result) == CTEX_RESULT_INVALID_ARGUMENT &&
                   result == NULL && atomic_load(&state.commits) == 0 && document[0] == 0x3c,
               "work callback failure published a result or committed bytes");
    descriptor.size = 0;
    result = (ctex_cpu_execution_result*)1;
    passed =
        expect(ctex_cpu_execute_bounded(&descriptor, &result) == CTEX_RESULT_INVALID_ARGUMENT &&
                   result == NULL &&
                   ctex_get_last_diagnostic_code() == CTEX_DIAGNOSTIC_INVALID_DESCRIPTOR_SIZE,
               "invalid CPU execution descriptor was not refused atomically") &&
        passed;
    return passed;
}

int main(void) {
    return worker_bounds_are_reproducible() && cancellation_discards_staged_output() &&
                   memory_ceiling_refuses_before_work() &&
                   callback_failure_and_bad_descriptor_are_atomic()
               ? 0
               : 1;
}
