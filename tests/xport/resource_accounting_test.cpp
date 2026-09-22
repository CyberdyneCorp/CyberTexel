#include <ctex/xport/resource_accounting.hpp>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string_view>

namespace {

using namespace ctex::xport;

bool expect(bool condition, std::string_view message) {
    if (!condition) {
        std::cerr << message << '\n';
    }
    return condition;
}

DeviceAllocationDescriptor device() {
    return {.backend = "metal", .device_identifier = "apple-gpu", .heap_identifier = "shared"};
}

bool shared_views_count_physical_storage_once() {
    ResourceLedger ledger;
    ledger.upsert(
        {.allocation_identity = 41,
         .category = ResourceCategory::document_storage,
         .physical_bytes = 4096,
         .roles = resource_role_cpu_resident | resource_role_gpu_resident | resource_role_pinned,
         .device = device()});
    ledger.upsert({.allocation_identity = 42,
                   .category = ResourceCategory::history,
                   .physical_bytes = 1024,
                   .roles = resource_role_cpu_resident | resource_role_in_flight,
                   .device = std::nullopt});
    const ResourceAccountingReport report = ledger.report();
    const auto& document =
        report.categories[static_cast<std::size_t>(ResourceCategory::document_storage)];
    return expect(report.allocation_count == 2 && report.physical_bytes == 5120,
                  "physical allocation total was double-counted") &&
           expect(report.cpu_resident_bytes == 5120 && report.gpu_resident_bytes == 4096 &&
                      report.pinned_bytes == 4096 && report.in_flight_bytes == 1024,
                  "overlapping residency roles were not reported independently") &&
           expect(document.allocation_count == 1 && document.physical_bytes == 4096 &&
                      document.cpu_resident_bytes == 4096 && document.gpu_resident_bytes == 4096,
                  "document category report lost shared allocation roles");
}

bool updates_and_release_are_identity_stable() {
    ResourceLedger ledger;
    ledger.upsert({.allocation_identity = 7,
                   .category = ResourceCategory::cache,
                   .physical_bytes = 256,
                   .roles = resource_role_cpu_resident,
                   .device = std::nullopt});
    ledger.upsert({.allocation_identity = 7,
                   .category = ResourceCategory::cache,
                   .physical_bytes = 256,
                   .roles = resource_role_cpu_resident | resource_role_in_flight,
                   .device = std::nullopt});
    const ResourceAccountingReport updated = ledger.report();
    const bool removed = ledger.remove(7);
    return expect(updated.allocation_count == 1 && updated.physical_bytes == 256 &&
                      updated.in_flight_bytes == 256,
                  "upsert duplicated a physical allocation") &&
           expect(removed && ledger.report().allocation_count == 0,
                  "released allocation remained accounted");
}

bool invalid_or_reused_identities_are_refused() {
    ResourceLedger ledger;
    bool missing_device = false;
    bool changed_storage = false;
    try {
        ledger.upsert({.allocation_identity = 1,
                       .category = ResourceCategory::temporary,
                       .physical_bytes = 64,
                       .roles = resource_role_gpu_resident,
                       .device = std::nullopt});
    } catch (const std::invalid_argument&) {
        missing_device = true;
    }
    ledger.upsert({.allocation_identity = 2,
                   .category = ResourceCategory::temporary,
                   .physical_bytes = 64,
                   .roles = resource_role_cpu_resident,
                   .device = std::nullopt});
    try {
        ledger.upsert({.allocation_identity = 2,
                       .category = ResourceCategory::cache,
                       .physical_bytes = 64,
                       .roles = resource_role_cpu_resident,
                       .device = std::nullopt});
    } catch (const std::invalid_argument&) {
        changed_storage = true;
    }
    return expect(missing_device, "GPU allocation without a device descriptor was accepted") &&
           expect(changed_storage, "allocation identity changed category without refusal");
}

ResourceBudgetLimits limits(std::size_t cpu, std::size_t temporary) {
    return {.cpu_bytes = cpu,
            .gpu_bytes = std::numeric_limits<std::size_t>::max(),
            .backing_store_bytes = std::numeric_limits<std::size_t>::max(),
            .temporary_bytes = temporary};
}

struct EvictionTracker {
    std::uint64_t identity{};
    std::size_t calls{};
};

void record_eviction(std::uint64_t identity, void* user_data) noexcept {
    auto& tracker = *static_cast<EvictionTracker*>(user_data);
    tracker.identity = identity;
    ++tracker.calls;
}

bool admission_reserves_a_bounded_tile_batch() {
    ResourceLedger ledger;
    ledger.upsert({.allocation_identity = 10,
                   .category = ResourceCategory::document_storage,
                   .physical_bytes = 400,
                   .roles = resource_role_cpu_resident,
                   .device = std::nullopt});
    const ResourceRequirement fixed{.category = ResourceCategory::history,
                                    .physical_bytes = 100,
                                    .roles = resource_role_cpu_resident};
    ResourceReservation first = ledger.admit(
        limits(1000, 300), {.operation = "large export",
                            .fixed_requirements = std::span<const ResourceRequirement>(&fixed, 1),
                            .per_work_item = {.category = ResourceCategory::temporary,
                                              .physical_bytes = 100,
                                              .roles = resource_role_cpu_resident},
                            .work_item_count = 10});
    ResourceReservation concurrent = ledger.admit(
        limits(1000, 300), {.operation = "concurrent export",
                            .fixed_requirements = std::span<const ResourceRequirement>(&fixed, 1),
                            .per_work_item = {.category = ResourceCategory::temporary,
                                              .physical_bytes = 100,
                                              .roles = resource_role_cpu_resident},
                            .work_item_count = 1});
    const bool bounded =
        expect(first.active() && first.report().status == ResourceAdmissionStatus::admitted_tiled &&
                   first.report().admitted_work_items == 3 &&
                   first.report().projected_usage.cpu_bytes == 800 &&
                   first.report().projected_usage.temporary_bytes == 300,
               "large work was not admitted as the maximum bounded tile batch") &&
        expect(!concurrent.active() &&
                   concurrent.report().status == ResourceAdmissionStatus::over_budget,
               "concurrent work ignored an active reservation");
    first.release();
    ResourceReservation after_release =
        ledger.admit(limits(1000, 300), {.operation = "later export",
                                         .fixed_requirements = {},
                                         .per_work_item = {.category = ResourceCategory::temporary,
                                                           .physical_bytes = 100,
                                                           .roles = resource_role_cpu_resident},
                                         .work_item_count = 1});
    return bounded && expect(after_release.active(), "released budget remained reserved");
}

bool admission_evicts_only_when_required_and_refusal_is_atomic() {
    ResourceLedger ledger;
    EvictionTracker tracker;
    ledger.set_cache_eviction_callback(record_eviction, &tracker);
    ledger.upsert({.allocation_identity = 20,
                   .category = ResourceCategory::document_storage,
                   .physical_bytes = 500,
                   .roles = resource_role_cpu_resident,
                   .device = std::nullopt});
    ledger.upsert({.allocation_identity = 21,
                   .category = ResourceCategory::cache,
                   .physical_bytes = 300,
                   .roles = resource_role_cpu_resident,
                   .device = std::nullopt});
    ledger.upsert({.allocation_identity = 22,
                   .category = ResourceCategory::cache,
                   .physical_bytes = 200,
                   .roles = resource_role_cpu_resident | resource_role_pinned,
                   .device = std::nullopt});
    const ResourceRequirement fixed{.category = ResourceCategory::recovery_record,
                                    .physical_bytes = 100,
                                    .roles = resource_role_cpu_resident};
    ResourceReservation admitted = ledger.admit(
        limits(1000, 1000), {.operation = "checkpoint",
                             .fixed_requirements = std::span<const ResourceRequirement>(&fixed, 1),
                             .per_work_item = {},
                             .work_item_count = 0});
    const bool evicted =
        expect(admitted.active() && admitted.report().evicted_allocation_identities ==
                                        std::vector<std::uint64_t>{21},
               "admission did not evict the eligible cache allocation") &&
        expect(tracker.calls == 1 && tracker.identity == 21,
               "admission did not release the physical cache allocation") &&
        expect(ledger.report().physical_bytes == 700,
               "admission evicted pinned or unrelated storage");

    ResourceLedger refusing;
    refusing.upsert({.allocation_identity = 30,
                     .category = ResourceCategory::document_storage,
                     .physical_bytes = 900,
                     .roles = resource_role_cpu_resident,
                     .device = std::nullopt});
    refusing.upsert({.allocation_identity = 31,
                     .category = ResourceCategory::cache,
                     .physical_bytes = 50,
                     .roles = resource_role_cpu_resident,
                     .device = std::nullopt});
    const ResourceRequirement impossible{.category = ResourceCategory::history,
                                         .physical_bytes = 200,
                                         .roles = resource_role_cpu_resident};
    ResourceReservation rejected =
        refusing.admit(limits(1000, 1000),
                       {.operation = "impossible edit",
                        .fixed_requirements = std::span<const ResourceRequirement>(&impossible, 1),
                        .per_work_item = {},
                        .work_item_count = 0});
    return evicted && expect(!rejected.active() && refusing.report().physical_bytes == 950,
                             "refused admission changed committed cache storage");
}

bool preview_quality_follows_host_policy_without_touching_authored_storage() {
    ResourceLedger ledger;
    ledger.upsert({.allocation_identity = 60,
                   .category = ResourceCategory::document_storage,
                   .physical_bytes = 600,
                   .roles = resource_role_cpu_resident,
                   .device = std::nullopt});
    const ResourceRequirement full_requirement{.category = ResourceCategory::composite,
                                               .physical_bytes = 500,
                                               .roles = resource_role_cpu_resident};
    const ResourceRequirement deferred_requirement{.category = ResourceCategory::composite,
                                                   .physical_bytes = 300,
                                                   .roles = resource_role_cpu_resident};
    const ResourceRequirement reduced_requirement{.category = ResourceCategory::composite,
                                                  .physical_bytes = 200,
                                                  .roles = resource_role_cpu_resident};
    const PreviewQualityOption options[] = {
        {.width = 1024,
         .height = 1024,
         .requirements = std::span<const ResourceRequirement>(&full_requirement, 1)},
        {.width = 1024,
         .height = 1024,
         .requirements = std::span<const ResourceRequirement>(&deferred_requirement, 1),
         .derived_work_deferred = true},
        {.width = 512,
         .height = 512,
         .requirements = std::span<const ResourceRequirement>(&reduced_requirement, 1),
         .derived_work_deferred = true},
    };
    const PreviewQualityRequest request = {
        .operation = "mobile viewport",
        .full_quality_width = 1024,
        .full_quality_height = 1024,
        .options = options,
    };

    PreviewQualityAdmission deferred = ledger.admit_preview_quality(limits(950, 1000), request);
    const bool deferred_ok =
        expect(deferred.reservation.active() &&
                   deferred.report.status == PreviewQualityStatus::deferred_derived &&
                   deferred.report.selected_option == 1 && deferred.report.selected_width == 1024,
               "preview policy did not defer derived work before reducing resolution");
    deferred.reservation.release();

    PreviewQualityAdmission reduced = ledger.admit_preview_quality(limits(850, 1000), request);
    const bool reduced_ok =
        expect(reduced.reservation.active() &&
                   reduced.report.status == PreviewQualityStatus::reduced_and_deferred &&
                   reduced.report.selected_option == 2 && reduced.report.selected_width == 512 &&
                   reduced.report.selected_height == 512 &&
                   reduced.report.projected_usage.cpu_bytes == 800,
               "preview policy did not report the selected reduced quality");
    reduced.reservation.release();

    EvictionTracker tracker;
    ledger.set_cache_eviction_callback(record_eviction, &tracker);
    ledger.upsert({.allocation_identity = 61,
                   .category = ResourceCategory::cache,
                   .physical_bytes = 200,
                   .roles = resource_role_cpu_resident,
                   .device = std::nullopt});
    PreviewQualityAdmission evicted = ledger.admit_preview_quality(limits(850, 1000), request);
    const bool eviction_ok =
        expect(evicted.reservation.active() && evicted.report.selected_option == 2 &&
                   evicted.report.evicted_allocation_identities == std::vector<std::uint64_t>{61} &&
                   tracker.calls == 1 && tracker.identity == 61,
               "preview quality admission did not release cache under pressure");
    evicted.reservation.release();

    PreviewQualityAdmission refused = ledger.admit_preview_quality(limits(700, 1000), request);
    return deferred_ok && reduced_ok && eviction_ok &&
           expect(!refused.reservation.active() &&
                      refused.report.status == PreviewQualityStatus::over_budget &&
                      ledger.report().physical_bytes == 600,
                  "preview over-budget refusal changed authored resource accounting");
}

bool quiesce_blocks_admission_and_tracks_active_work() {
    ResourceLedger ledger;
    const ResourceRequirement requirement{.category = ResourceCategory::temporary,
                                          .physical_bytes = 64,
                                          .roles = resource_role_cpu_resident};
    const ResourceAdmissionRequest request = {
        .operation = "active edit",
        .fixed_requirements = std::span<const ResourceRequirement>(&requirement, 1),
        .per_work_item = {},
        .work_item_count = 0,
    };
    ResourceReservation active = ledger.admit(limits(256, 256), request);
    ledger.begin_quiesce();
    ResourceReservation refused = ledger.admit(limits(256, 256), request);
    const bool stopped =
        expect(active.active() && !refused.active() &&
                   refused.report().status == ResourceAdmissionStatus::quiescing &&
                   !ledger.accepting_admissions() && ledger.active_reservation_count() == 1 &&
                   !ledger.wait_until_quiescent(std::chrono::milliseconds::zero()),
               "quiesce did not stop admission or retain active-work visibility");
    active.release();
    const bool drained = expect(ledger.wait_until_quiescent(std::chrono::milliseconds::zero()) &&
                                    ledger.active_reservation_count() == 0,
                                "released work did not make the ledger quiescent");
    ledger.resume_admission();
    ResourceReservation resumed = ledger.admit(limits(256, 256), request);
    return stopped && drained &&
           expect(resumed.active() && ledger.accepting_admissions(),
                  "resuming the lifecycle gate did not restore admission");
}

}  // namespace

int main() {
    return shared_views_count_physical_storage_once() &&
                   updates_and_release_are_identity_stable() &&
                   invalid_or_reused_identities_are_refused() &&
                   admission_reserves_a_bounded_tile_batch() &&
                   admission_evicts_only_when_required_and_refusal_is_atomic() &&
                   preview_quality_follows_host_policy_without_touching_authored_storage() &&
                   quiesce_blocks_admission_and_tracks_active_work()
               ? 0
               : 1;
}
