#include <ctex/capi.h>
#include <stdio.h>
#include <string.h>

static int expect(int condition, const char* message) {
    if (!condition) {
        fprintf(stderr, "%s\n", message);
    }
    return condition;
}

typedef struct eviction_tracker {
    uint64_t identity;
    size_t calls;
} eviction_tracker;

static void record_eviction(uint64_t identity, void* user_data) {
    eviction_tracker* tracker = (eviction_tracker*)user_data;
    tracker->identity = identity;
    ++tracker->calls;
}

static ctex_resource_allocation_descriptor allocation(uint64_t identity, uint32_t category,
                                                      size_t bytes, uint32_t roles) {
    ctex_resource_allocation_descriptor result = {
        .size = CTEX_RESOURCE_ALLOCATION_DESCRIPTOR_CURRENT_SIZE,
        .allocation_identity = identity,
        .category = category,
        .physical_bytes = bytes,
        .roles = roles,
    };
    return result;
}

int main(void) {
    ctex_resource_ledger* ledger = NULL;
    if (!expect(ctex_resource_ledger_create(&ledger) == CTEX_RESULT_SUCCESS && ledger != NULL,
                "resource ledger creation failed")) {
        return 1;
    }

    ctex_resource_allocation_descriptor shared =
        allocation(41, CTEX_RESOURCE_DOCUMENT_STORAGE, 4096,
                   CTEX_RESOURCE_CPU_RESIDENT | CTEX_RESOURCE_GPU_RESIDENT | CTEX_RESOURCE_PINNED);
    shared.device_backend = "metal";
    shared.device_identifier = "apple-gpu";
    shared.heap_identifier = "shared";
    ctex_resource_allocation_descriptor history = allocation(
        42, CTEX_RESOURCE_HISTORY, 1024, CTEX_RESOURCE_CPU_RESIDENT | CTEX_RESOURCE_IN_FLIGHT);
    ctex_resource_allocation_descriptor recovery =
        allocation(43, CTEX_RESOURCE_RECOVERY_RECORD, 2048, CTEX_RESOURCE_BACKING_STORE);
    if (!expect(ctex_resource_ledger_upsert(ledger, &shared) == CTEX_RESULT_SUCCESS &&
                    ctex_resource_ledger_upsert(ledger, &history) == CTEX_RESULT_SUCCESS &&
                    ctex_resource_ledger_upsert(ledger, &recovery) == CTEX_RESULT_SUCCESS,
                "valid resource allocations were refused")) {
        ctex_resource_ledger_destroy(ledger);
        return 1;
    }

    ctex_resource_accounting_report sizing = {
        .size = CTEX_RESOURCE_ACCOUNTING_REPORT_CURRENT_SIZE,
    };
    if (!expect(ctex_resource_ledger_get_report(ledger, &sizing, NULL, 0) == CTEX_RESULT_SUCCESS &&
                    sizing.category_count == CTEX_RESOURCE_CATEGORY_COUNT,
                "resource report sizing failed")) {
        ctex_resource_ledger_destroy(ledger);
        return 1;
    }

    ctex_resource_category_report short_output[CTEX_RESOURCE_CATEGORY_COUNT];
    memset(short_output, 0x5a, sizeof(short_output));
    ctex_resource_accounting_report short_report = {
        .size = CTEX_RESOURCE_ACCOUNTING_REPORT_CURRENT_SIZE,
        .physical_bytes = 77,
    };
    const ctex_resource_category_report short_before = short_output[0];
    if (!expect(ctex_resource_ledger_get_report(ledger, &short_report, short_output,
                                                CTEX_RESOURCE_CATEGORY_COUNT - 1) ==
                        CTEX_RESULT_BUFFER_TOO_SMALL &&
                    short_report.physical_bytes == 77 &&
                    memcmp(&short_output[0], &short_before, sizeof(short_before)) == 0,
                "short resource report mutated caller output")) {
        ctex_resource_ledger_destroy(ledger);
        return 1;
    }

    ctex_resource_category_report categories[CTEX_RESOURCE_CATEGORY_COUNT];
    ctex_resource_accounting_report report = {
        .size = CTEX_RESOURCE_ACCOUNTING_REPORT_CURRENT_SIZE,
    };
    if (!expect(
            ctex_resource_ledger_get_report(ledger, &report, categories,
                                            CTEX_RESOURCE_CATEGORY_COUNT) == CTEX_RESULT_SUCCESS,
            "resource report query failed")) {
        ctex_resource_ledger_destroy(ledger);
        return 1;
    }
    const ctex_resource_category_report document = categories[CTEX_RESOURCE_DOCUMENT_STORAGE];
    const int totals_ok =
        expect(report.allocation_count == 3 && report.physical_bytes == 7168 &&
                   report.cpu_resident_bytes == 5120 && report.gpu_resident_bytes == 4096 &&
                   report.backing_store_bytes == 2048 && report.pinned_bytes == 4096 &&
                   report.in_flight_bytes == 1024,
               "resource totals did not preserve overlapping roles") &&
        expect(document.category == CTEX_RESOURCE_DOCUMENT_STORAGE &&
                   document.physical_bytes == 4096 && document.cpu_resident_bytes == 4096 &&
                   document.gpu_resident_bytes == 4096,
               "document allocation was not categorized correctly");

    ctex_resource_allocation_descriptor invalid_gpu =
        allocation(44, CTEX_RESOURCE_TEMPORARY, 64, CTEX_RESOURCE_GPU_RESIDENT);
    uint32_t removed = 0;
    const int validation_ok =
        expect(ctex_resource_ledger_upsert(ledger, &invalid_gpu) == CTEX_RESULT_INVALID_ARGUMENT,
               "GPU allocation without a device descriptor was accepted") &&
        expect(ctex_resource_ledger_remove(ledger, 42, &removed) == CTEX_RESULT_SUCCESS &&
                   removed == 1,
               "resource allocation removal failed");

    ctex_resource_requirement fixed = {
        .size = CTEX_RESOURCE_REQUIREMENT_CURRENT_SIZE,
        .category = CTEX_RESOURCE_HISTORY,
        .physical_bytes = 100,
        .roles = CTEX_RESOURCE_CPU_RESIDENT,
    };
    ctex_resource_admission_descriptor admission = {
        .size = CTEX_RESOURCE_ADMISSION_DESCRIPTOR_CURRENT_SIZE,
        .operation = "large export",
        .limits = {.size = CTEX_RESOURCE_BUDGET_LIMITS_CURRENT_SIZE,
                   .cpu_bytes = 4596,
                   .gpu_bytes = 5000,
                   .backing_store_bytes = 3000,
                   .temporary_bytes = 200},
        .fixed_requirements = &fixed,
        .fixed_requirement_count = 1,
        .per_work_item = {.size = CTEX_RESOURCE_REQUIREMENT_CURRENT_SIZE,
                          .category = CTEX_RESOURCE_TEMPORARY,
                          .physical_bytes = 100,
                          .roles = CTEX_RESOURCE_CPU_RESIDENT},
        .work_item_count = 5,
    };
    ctex_resource_reservation* reservation = NULL;
    ctex_resource_admission_report admission_report = {
        .size = CTEX_RESOURCE_ADMISSION_REPORT_CURRENT_SIZE,
    };
    const int tiled_ok =
        expect(ctex_resource_ledger_admit(ledger, &admission, &reservation, &admission_report) ==
                       CTEX_RESULT_SUCCESS &&
                   reservation != NULL && admission_report.status == CTEX_RESOURCE_ADMITTED_TILED &&
                   admission_report.admitted_work_items == 2 &&
                   admission_report.projected_cpu_bytes == 4396 &&
                   admission_report.projected_temporary_bytes == 200,
               "resource admission did not schedule a bounded tile batch");
    ctex_resource_reservation_release(reservation);
    ctex_resource_reservation_destroy(reservation);

    ctex_resource_allocation_descriptor cache =
        allocation(50, CTEX_RESOURCE_CACHE, 300, CTEX_RESOURCE_CPU_RESIDENT);
    eviction_tracker tracker = {0};
    ctex_resource_admission_descriptor checkpoint = {
        .size = CTEX_RESOURCE_ADMISSION_DESCRIPTOR_CURRENT_SIZE,
        .operation = "checkpoint",
        .limits = {.size = CTEX_RESOURCE_BUDGET_LIMITS_CURRENT_SIZE,
                   .cpu_bytes = 4400,
                   .gpu_bytes = 5000,
                   .backing_store_bytes = 3000,
                   .temporary_bytes = 200},
        .fixed_requirements = &fixed,
        .fixed_requirement_count = 1,
    };
    ctex_resource_reservation* checkpoint_reservation = NULL;
    ctex_resource_admission_report checkpoint_report = {
        .size = CTEX_RESOURCE_ADMISSION_REPORT_CURRENT_SIZE,
    };
    uint64_t evicted_identity = 0;
    size_t evicted_count = 99;
    const int eviction_ok =
        expect(
            ctex_resource_ledger_set_cache_eviction_callback(ledger, record_eviction, &tracker) ==
                    CTEX_RESULT_SUCCESS &&
                ctex_resource_ledger_upsert(ledger, &cache) == CTEX_RESULT_SUCCESS &&
                ctex_resource_ledger_admit(ledger, &checkpoint, &checkpoint_reservation,
                                           &checkpoint_report) == CTEX_RESULT_SUCCESS &&
                checkpoint_reservation != NULL && checkpoint_report.evicted_allocation_count == 1,
            "required cache eviction was not reported") &&
        expect(tracker.calls == 1 && tracker.identity == 50,
               "cache eviction callback did not release the physical allocation") &&
        expect(ctex_resource_reservation_get_evicted_allocations(
                   checkpoint_reservation, NULL, 0, &evicted_count) == CTEX_RESULT_SUCCESS &&
                   evicted_count == 1,
               "evicted allocation sizing failed") &&
        expect(ctex_resource_reservation_get_evicted_allocations(
                   checkpoint_reservation, &evicted_identity, 1, &evicted_count) ==
                       CTEX_RESULT_SUCCESS &&
                   evicted_count == 1 && evicted_identity == 50,
               "evicted allocation identity was not queryable");
    ctex_resource_reservation_destroy(checkpoint_reservation);

    ctex_resource_ledger_destroy(ledger);
    return totals_ok && validation_ok && tiled_ok && eviction_ok ? 0 : 1;
}
