#include <ctex/xport/resource_accounting.hpp>
#include <limits>
#include <mutex>
#include <stdexcept>
#include <unordered_map>
#include <utility>

namespace ctex::xport {
namespace {

constexpr std::uint32_t residency_roles =
    resource_role_cpu_resident | resource_role_gpu_resident | resource_role_backing_store;
constexpr std::uint32_t known_roles =
    residency_roles | resource_role_pinned | resource_role_in_flight;

void add_bytes(std::size_t& total, std::size_t value) {
    if (value > std::numeric_limits<std::size_t>::max() - total) {
        throw std::overflow_error("resource accounting byte total overflow");
    }
    total += value;
}

void add_descriptor(ResourceCategoryReport& report,
                    const ResourceAllocationDescriptor& descriptor) {
    ++report.allocation_count;
    add_bytes(report.physical_bytes, descriptor.physical_bytes);
    if ((descriptor.roles & resource_role_cpu_resident) != 0U) {
        add_bytes(report.cpu_resident_bytes, descriptor.physical_bytes);
    }
    if ((descriptor.roles & resource_role_gpu_resident) != 0U) {
        add_bytes(report.gpu_resident_bytes, descriptor.physical_bytes);
    }
    if ((descriptor.roles & resource_role_backing_store) != 0U) {
        add_bytes(report.backing_store_bytes, descriptor.physical_bytes);
    }
    if ((descriptor.roles & resource_role_pinned) != 0U) {
        add_bytes(report.pinned_bytes, descriptor.physical_bytes);
    }
    if ((descriptor.roles & resource_role_in_flight) != 0U) {
        add_bytes(report.in_flight_bytes, descriptor.physical_bytes);
    }
}

void validate(const ResourceAllocationDescriptor& descriptor) {
    if (descriptor.allocation_identity == 0 || descriptor.physical_bytes == 0) {
        throw std::invalid_argument("resource allocation identity and byte size must be non-zero");
    }
    if (descriptor.category >= ResourceCategory::count) {
        throw std::invalid_argument("resource allocation category is invalid");
    }
    if ((descriptor.roles & ~known_roles) != 0U || (descriptor.roles & residency_roles) == 0U) {
        throw std::invalid_argument("resource allocation roles are invalid or lack residency");
    }
    const bool gpu_resident = (descriptor.roles & resource_role_gpu_resident) != 0U;
    if (gpu_resident != descriptor.device.has_value()) {
        throw std::invalid_argument("GPU residency requires exactly one device descriptor");
    }
    if (descriptor.device &&
        (descriptor.device->backend.empty() || descriptor.device->device_identifier.empty() ||
         descriptor.device->heap_identifier.empty())) {
        throw std::invalid_argument("device allocation descriptor fields must be non-empty");
    }
}

}  // namespace

class ResourceLedger::Impl {
public:
    mutable std::mutex mutex;
    std::unordered_map<std::uint64_t, ResourceAllocationDescriptor> allocations;
};

ResourceLedger::ResourceLedger() : impl_(std::make_unique<Impl>()) {}
ResourceLedger::~ResourceLedger() = default;
ResourceLedger::ResourceLedger(ResourceLedger&&) noexcept = default;
ResourceLedger& ResourceLedger::operator=(ResourceLedger&&) noexcept = default;

void ResourceLedger::upsert(ResourceAllocationDescriptor descriptor) {
    validate(descriptor);
    std::lock_guard lock(impl_->mutex);
    const auto existing = impl_->allocations.find(descriptor.allocation_identity);
    if (existing != impl_->allocations.end() &&
        (existing->second.category != descriptor.category ||
         existing->second.physical_bytes != descriptor.physical_bytes ||
         existing->second.device != descriptor.device)) {
        throw std::invalid_argument(
            "an allocation identity cannot change storage, category, or device");
    }
    impl_->allocations.insert_or_assign(descriptor.allocation_identity, std::move(descriptor));
}

bool ResourceLedger::remove(std::uint64_t allocation_identity) noexcept {
    std::lock_guard lock(impl_->mutex);
    return impl_->allocations.erase(allocation_identity) != 0;
}

ResourceAccountingReport ResourceLedger::report() const {
    std::lock_guard lock(impl_->mutex);
    ResourceAccountingReport result;
    for (std::size_t index = 0; index < result.categories.size(); ++index) {
        result.categories[index].category = static_cast<ResourceCategory>(index);
    }
    for (const auto& [identity, descriptor] : impl_->allocations) {
        static_cast<void>(identity);
        add_descriptor(result.categories[static_cast<std::size_t>(descriptor.category)],
                       descriptor);
    }
    for (const ResourceCategoryReport& category : result.categories) {
        result.allocation_count += category.allocation_count;
        add_bytes(result.physical_bytes, category.physical_bytes);
        add_bytes(result.cpu_resident_bytes, category.cpu_resident_bytes);
        add_bytes(result.gpu_resident_bytes, category.gpu_resident_bytes);
        add_bytes(result.backing_store_bytes, category.backing_store_bytes);
        add_bytes(result.pinned_bytes, category.pinned_bytes);
        add_bytes(result.in_flight_bytes, category.in_flight_bytes);
    }
    return result;
}

std::string_view resource_category_name(ResourceCategory category) noexcept {
    switch (category) {
        case ResourceCategory::document_storage:
            return "document-storage";
        case ResourceCategory::history:
            return "history";
        case ResourceCategory::recovery_record:
            return "recovery-record";
        case ResourceCategory::mesh_map:
            return "mesh-map";
        case ResourceCategory::composite:
            return "composite";
        case ResourceCategory::cache:
            return "cache";
        case ResourceCategory::temporary:
            return "temporary";
        case ResourceCategory::count:
            break;
    }
    return "unknown";
}

}  // namespace ctex::xport
