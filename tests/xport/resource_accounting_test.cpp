#include <ctex/xport/resource_accounting.hpp>
#include <iostream>
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

}  // namespace

int main() {
    return shared_views_count_physical_storage_once() &&
                   updates_and_release_are_identity_stable() &&
                   invalid_or_reused_identities_are_refused()
               ? 0
               : 1;
}
