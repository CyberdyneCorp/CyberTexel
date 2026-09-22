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

}  // namespace

int main() {
    return shared_views_count_physical_storage_once() &&
                   updates_and_release_are_identity_stable() &&
                   invalid_or_reused_identities_are_refused() &&
                   admission_reserves_a_bounded_tile_batch() &&
                   admission_evicts_only_when_required_and_refusal_is_atomic()
               ? 0
               : 1;
}
