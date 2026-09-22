#ifndef CTEX_XPORT_RESOURCE_ACCOUNTING_HPP
#define CTEX_XPORT_RESOURCE_ACCOUNTING_HPP

#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

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

struct ResourceBudgetLimits {
    std::size_t cpu_bytes{};
    std::size_t gpu_bytes{};
    std::size_t backing_store_bytes{};
    std::size_t temporary_bytes{};
};

struct ResourceRequirement {
    ResourceCategory category{};
    std::size_t physical_bytes{};
    std::uint32_t roles{};
};

struct ResourceAdmissionRequest {
    std::string operation;
    std::span<const ResourceRequirement> fixed_requirements;
    ResourceRequirement per_work_item;
    std::size_t work_item_count{};
};

enum class ResourceAdmissionStatus : std::uint8_t {
    admitted_whole,
    admitted_tiled,
    over_budget,
    quiescing,
};

struct ResourceAdmissionReport {
    ResourceAdmissionStatus status{ResourceAdmissionStatus::over_budget};
    std::size_t work_item_count{};
    std::size_t admitted_work_items{};
    ResourceBudgetLimits projected_usage{};
    std::vector<std::uint64_t> evicted_allocation_identities;
    std::string detail;
};

struct PreviewQualityOption {
    std::uint32_t width{};
    std::uint32_t height{};
    std::span<const ResourceRequirement> requirements;
    bool derived_work_deferred{};
};

struct PreviewQualityRequest {
    std::string operation;
    std::uint32_t full_quality_width{};
    std::uint32_t full_quality_height{};
    std::span<const PreviewQualityOption> options;
};

enum class PreviewQualityStatus : std::uint8_t {
    full_quality,
    reduced_resolution,
    deferred_derived,
    reduced_and_deferred,
    over_budget,
    quiescing,
};

struct PreviewQualityReport {
    PreviewQualityStatus status{PreviewQualityStatus::over_budget};
    std::size_t selected_option{};
    std::uint32_t full_quality_width{};
    std::uint32_t full_quality_height{};
    std::uint32_t selected_width{};
    std::uint32_t selected_height{};
    bool derived_work_deferred{};
    ResourceBudgetLimits projected_usage{};
    std::vector<std::uint64_t> evicted_allocation_identities;
    std::string detail;
};

class ResourceLedgerState;

class ResourceReservation {
public:
    ResourceReservation() = default;
    ~ResourceReservation();
    ResourceReservation(ResourceReservation&&) noexcept;
    ResourceReservation& operator=(ResourceReservation&&) noexcept;
    ResourceReservation(const ResourceReservation&) = delete;
    ResourceReservation& operator=(const ResourceReservation&) = delete;

    [[nodiscard]] bool active() const noexcept;
    [[nodiscard]] const ResourceAdmissionReport& report() const noexcept { return report_; }
    void release() noexcept;

private:
    friend class ResourceLedger;
    ResourceReservation(std::shared_ptr<ResourceLedgerState> state, ResourceBudgetLimits reserved,
                        ResourceAdmissionReport report);

    std::shared_ptr<ResourceLedgerState> state_;
    ResourceBudgetLimits reserved_{};
    ResourceAdmissionReport report_{};
};

struct PreviewQualityAdmission {
    ResourceReservation reservation;
    PreviewQualityReport report;
};

class ResourceLedger {
public:
    using CacheEvictionCallback = void (*)(std::uint64_t allocation_identity,
                                           void* user_data) noexcept;

    ResourceLedger();
    ~ResourceLedger();
    ResourceLedger(ResourceLedger&&) noexcept;
    ResourceLedger& operator=(ResourceLedger&&) noexcept;
    ResourceLedger(const ResourceLedger&) = delete;
    ResourceLedger& operator=(const ResourceLedger&) = delete;

    void upsert(ResourceAllocationDescriptor descriptor);
    [[nodiscard]] bool remove(std::uint64_t allocation_identity) noexcept;
    [[nodiscard]] ResourceAccountingReport report() const;
    void set_cache_eviction_callback(CacheEvictionCallback callback,
                                     void* user_data = nullptr) noexcept;
    [[nodiscard]] ResourceReservation admit(const ResourceBudgetLimits& limits,
                                            const ResourceAdmissionRequest& request);
    [[nodiscard]] PreviewQualityAdmission admit_preview_quality(
        const ResourceBudgetLimits& limits, const PreviewQualityRequest& request);
    void begin_quiesce();
    void resume_admission();
    [[nodiscard]] bool wait_until_quiescent(std::chrono::milliseconds timeout) const;
    [[nodiscard]] bool accepting_admissions() const;
    [[nodiscard]] std::size_t active_reservation_count() const;

private:
    std::shared_ptr<ResourceLedgerState> state_;
};

[[nodiscard]] std::string_view resource_category_name(ResourceCategory category) noexcept;
[[nodiscard]] std::string_view resource_admission_status_name(
    ResourceAdmissionStatus status) noexcept;
[[nodiscard]] std::string_view preview_quality_status_name(PreviewQualityStatus status) noexcept;

}  // namespace ctex::xport

#endif
