#ifndef CTEX_XPORT_RESOURCE_ACCOUNTING_HPP
#define CTEX_XPORT_RESOURCE_ACCOUNTING_HPP

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <string_view>

namespace ctex::xport {

enum class ResourceCategory : std::uint8_t {
    document_storage,
    history,
    recovery_record,
    mesh_map,
    composite,
    cache,
    temporary,
    count
};

enum ResourceRole : std::uint32_t {
    resource_role_cpu_resident = 1U << 0U,
    resource_role_gpu_resident = 1U << 1U,
    resource_role_backing_store = 1U << 2U,
    resource_role_pinned = 1U << 3U,
    resource_role_in_flight = 1U << 4U,
};

struct DeviceAllocationDescriptor {
    std::string backend;
    std::string device_identifier;
    std::string heap_identifier;

    friend bool operator==(const DeviceAllocationDescriptor&,
                           const DeviceAllocationDescriptor&) = default;
};

struct ResourceAllocationDescriptor {
    std::uint64_t allocation_identity{};
    ResourceCategory category{};
    std::size_t physical_bytes{};
    std::uint32_t roles{};
    std::optional<DeviceAllocationDescriptor> device;
};

struct ResourceCategoryReport {
    ResourceCategory category{};
    std::size_t allocation_count{};
    std::size_t physical_bytes{};
    std::size_t cpu_resident_bytes{};
    std::size_t gpu_resident_bytes{};
    std::size_t backing_store_bytes{};
    std::size_t pinned_bytes{};
    std::size_t in_flight_bytes{};
};

struct ResourceAccountingReport {
    std::array<ResourceCategoryReport, static_cast<std::size_t>(ResourceCategory::count)>
        categories{};
    std::size_t allocation_count{};
    std::size_t physical_bytes{};
    std::size_t cpu_resident_bytes{};
    std::size_t gpu_resident_bytes{};
    std::size_t backing_store_bytes{};
    std::size_t pinned_bytes{};
    std::size_t in_flight_bytes{};
};

class ResourceLedger {
public:
    ResourceLedger();
    ~ResourceLedger();
    ResourceLedger(ResourceLedger&&) noexcept;
    ResourceLedger& operator=(ResourceLedger&&) noexcept;
    ResourceLedger(const ResourceLedger&) = delete;
    ResourceLedger& operator=(const ResourceLedger&) = delete;

    void upsert(ResourceAllocationDescriptor descriptor);
    [[nodiscard]] bool remove(std::uint64_t allocation_identity) noexcept;
    [[nodiscard]] ResourceAccountingReport report() const;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

[[nodiscard]] std::string_view resource_category_name(ResourceCategory category) noexcept;

}  // namespace ctex::xport

#endif
