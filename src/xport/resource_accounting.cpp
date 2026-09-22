#include <algorithm>
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

std::size_t multiply_bytes(std::size_t bytes, std::size_t count) {
    if (count != 0 && bytes > std::numeric_limits<std::size_t>::max() / count) {
        throw std::overflow_error("resource reservation byte total overflow");
    }
    return bytes * count;
}

void validate_roles(std::uint32_t roles) {
    if ((roles & ~known_roles) != 0U || (roles & residency_roles) == 0U) {
        throw std::invalid_argument("resource allocation roles are invalid or lack residency");
    }
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
    validate_roles(descriptor.roles);
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

void validate_requirement(const ResourceRequirement& requirement) {
    if (requirement.category >= ResourceCategory::count || requirement.physical_bytes == 0) {
        throw std::invalid_argument("resource requirement category and byte size are invalid");
    }
    validate_roles(requirement.roles);
}

ResourceBudgetLimits usage(const ResourceAllocationDescriptor& descriptor) {
    return {
        .cpu_bytes =
            (descriptor.roles & resource_role_cpu_resident) != 0U ? descriptor.physical_bytes : 0,
        .gpu_bytes =
            (descriptor.roles & resource_role_gpu_resident) != 0U ? descriptor.physical_bytes : 0,
        .backing_store_bytes =
            (descriptor.roles & resource_role_backing_store) != 0U ? descriptor.physical_bytes : 0,
        .temporary_bytes =
            descriptor.category == ResourceCategory::temporary ? descriptor.physical_bytes : 0,
    };
}

ResourceBudgetLimits usage(const ResourceRequirement& requirement, std::size_t count = 1) {
    const std::size_t bytes = multiply_bytes(requirement.physical_bytes, count);
    return {
        .cpu_bytes = (requirement.roles & resource_role_cpu_resident) != 0U ? bytes : 0,
        .gpu_bytes = (requirement.roles & resource_role_gpu_resident) != 0U ? bytes : 0,
        .backing_store_bytes = (requirement.roles & resource_role_backing_store) != 0U ? bytes : 0,
        .temporary_bytes = requirement.category == ResourceCategory::temporary ? bytes : 0,
    };
}

ResourceBudgetLimits add_usage(ResourceBudgetLimits left, const ResourceBudgetLimits& right) {
    add_bytes(left.cpu_bytes, right.cpu_bytes);
    add_bytes(left.gpu_bytes, right.gpu_bytes);
    add_bytes(left.backing_store_bytes, right.backing_store_bytes);
    add_bytes(left.temporary_bytes, right.temporary_bytes);
    return left;
}

void subtract_usage(ResourceBudgetLimits& left, const ResourceBudgetLimits& right) noexcept {
    left.cpu_bytes -= right.cpu_bytes;
    left.gpu_bytes -= right.gpu_bytes;
    left.backing_store_bytes -= right.backing_store_bytes;
    left.temporary_bytes -= right.temporary_bytes;
}

bool fits(const ResourceBudgetLimits& usage_value, const ResourceBudgetLimits& limits) noexcept {
    return usage_value.cpu_bytes <= limits.cpu_bytes && usage_value.gpu_bytes <= limits.gpu_bytes &&
           usage_value.backing_store_bytes <= limits.backing_store_bytes &&
           usage_value.temporary_bytes <= limits.temporary_bytes;
}

bool relieves_shortage(const ResourceBudgetLimits& base, const ResourceBudgetLimits& required,
                       const ResourceBudgetLimits& candidate, const ResourceBudgetLimits& limits) {
    const ResourceBudgetLimits projected = add_usage(base, required);
    return (projected.cpu_bytes > limits.cpu_bytes && candidate.cpu_bytes != 0) ||
           (projected.gpu_bytes > limits.gpu_bytes && candidate.gpu_bytes != 0) ||
           (projected.backing_store_bytes > limits.backing_store_bytes &&
            candidate.backing_store_bytes != 0) ||
           (projected.temporary_bytes > limits.temporary_bytes && candidate.temporary_bytes != 0);
}

std::size_t cap_batch(std::size_t current, std::size_t base, std::size_t fixed,
                      std::size_t per_item, std::size_t limit) noexcept {
    if (per_item == 0) {
        return current;
    }
    if (base > limit || fixed > limit - base) {
        return 0;
    }
    return std::min(current, (limit - base - fixed) / per_item);
}

std::size_t maximum_batch(const ResourceBudgetLimits& base, const ResourceBudgetLimits& fixed,
                          const ResourceBudgetLimits& per_item, const ResourceBudgetLimits& limits,
                          std::size_t work_item_count) {
    std::size_t result = work_item_count;
    result =
        cap_batch(result, base.cpu_bytes, fixed.cpu_bytes, per_item.cpu_bytes, limits.cpu_bytes);
    result =
        cap_batch(result, base.gpu_bytes, fixed.gpu_bytes, per_item.gpu_bytes, limits.gpu_bytes);
    result = cap_batch(result, base.backing_store_bytes, fixed.backing_store_bytes,
                       per_item.backing_store_bytes, limits.backing_store_bytes);
    return cap_batch(result, base.temporary_bytes, fixed.temporary_bytes, per_item.temporary_bytes,
                     limits.temporary_bytes);
}

ResourceBudgetLimits requirement_usage(std::span<const ResourceRequirement> requirements) {
    ResourceBudgetLimits result;
    for (const ResourceRequirement& requirement : requirements) {
        validate_requirement(requirement);
        result = add_usage(result, usage(requirement));
    }
    return result;
}

PreviewQualityStatus preview_status(const PreviewQualityRequest& request,
                                    const PreviewQualityOption& option) noexcept {
    const bool reduced =
        option.width < request.full_quality_width || option.height < request.full_quality_height;
    if (reduced && option.derived_work_deferred) {
        return PreviewQualityStatus::reduced_and_deferred;
    }
    if (reduced) {
        return PreviewQualityStatus::reduced_resolution;
    }
    if (option.derived_work_deferred) {
        return PreviewQualityStatus::deferred_derived;
    }
    return PreviewQualityStatus::full_quality;
}

}  // namespace

class ResourceLedgerState {
public:
    mutable std::mutex mutex;
    std::unordered_map<std::uint64_t, ResourceAllocationDescriptor> allocations;
    ResourceBudgetLimits reserved;
    ResourceLedger::CacheEvictionCallback cache_eviction_callback{};
    void* cache_eviction_user_data{};
};

ResourceLedger::ResourceLedger() : state_(std::make_shared<ResourceLedgerState>()) {}
ResourceLedger::~ResourceLedger() = default;
ResourceLedger::ResourceLedger(ResourceLedger&&) noexcept = default;
ResourceLedger& ResourceLedger::operator=(ResourceLedger&&) noexcept = default;

void ResourceLedger::upsert(ResourceAllocationDescriptor descriptor) {
    validate(descriptor);
    std::lock_guard lock(state_->mutex);
    const auto existing = state_->allocations.find(descriptor.allocation_identity);
    if (existing != state_->allocations.end() &&
        (existing->second.category != descriptor.category ||
         existing->second.physical_bytes != descriptor.physical_bytes ||
         existing->second.device != descriptor.device)) {
        throw std::invalid_argument(
            "an allocation identity cannot change storage, category, or device");
    }
    state_->allocations.insert_or_assign(descriptor.allocation_identity, std::move(descriptor));
}

bool ResourceLedger::remove(std::uint64_t allocation_identity) noexcept {
    std::lock_guard lock(state_->mutex);
    return state_->allocations.erase(allocation_identity) != 0;
}

ResourceAccountingReport ResourceLedger::report() const {
    std::lock_guard lock(state_->mutex);
    ResourceAccountingReport result;
    for (std::size_t index = 0; index < result.categories.size(); ++index) {
        result.categories[index].category = static_cast<ResourceCategory>(index);
    }
    for (const auto& [identity, descriptor] : state_->allocations) {
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

void ResourceLedger::set_cache_eviction_callback(CacheEvictionCallback callback,
                                                 void* user_data) noexcept {
    std::lock_guard lock(state_->mutex);
    state_->cache_eviction_callback = callback;
    state_->cache_eviction_user_data = user_data;
}

ResourceReservation ResourceLedger::admit(const ResourceBudgetLimits& limits,
                                          const ResourceAdmissionRequest& request) {
    if (request.operation.empty()) {
        throw std::invalid_argument("resource admission operation must not be empty");
    }
    ResourceBudgetLimits fixed;
    for (const ResourceRequirement& requirement : request.fixed_requirements) {
        validate_requirement(requirement);
        fixed = add_usage(fixed, usage(requirement));
    }
    ResourceBudgetLimits per_item;
    if (request.work_item_count != 0) {
        validate_requirement(request.per_work_item);
        if (request.per_work_item.category != ResourceCategory::temporary) {
            throw std::invalid_argument("per-work-item storage must be temporary");
        }
        per_item = usage(request.per_work_item);
    }

    std::lock_guard lock(state_->mutex);
    ResourceBudgetLimits base = state_->reserved;
    for (const auto& [identity, descriptor] : state_->allocations) {
        static_cast<void>(identity);
        base = add_usage(base, usage(descriptor));
    }
    const ResourceBudgetLimits original_base = base;

    const ResourceBudgetLimits complete =
        add_usage(fixed, usage(request.per_work_item, request.work_item_count));
    if (fits(add_usage(base, complete), limits)) {
        state_->reserved = add_usage(state_->reserved, complete);
        return ResourceReservation(
            state_, complete,
            {.status = ResourceAdmissionStatus::admitted_whole,
             .work_item_count = request.work_item_count,
             .admitted_work_items = request.work_item_count,
             .projected_usage = add_usage(base, complete),
             .evicted_allocation_identities = {},
             .detail = request.operation + " admitted as one bounded batch"});
    }

    const ResourceBudgetLimits minimum =
        add_usage(fixed, request.work_item_count == 0 ? ResourceBudgetLimits{} : per_item);
    std::vector<std::uint64_t> candidates;
    candidates.reserve(state_->allocations.size());
    for (const auto& [identity, descriptor] : state_->allocations) {
        if (state_->cache_eviction_callback != nullptr &&
            descriptor.category == ResourceCategory::cache &&
            (descriptor.roles & (resource_role_pinned | resource_role_in_flight)) == 0U) {
            candidates.push_back(identity);
        }
    }
    std::ranges::sort(candidates);

    std::vector<std::uint64_t> evicted;
    for (const std::uint64_t identity : candidates) {
        if (fits(add_usage(base, minimum), limits)) {
            break;
        }
        const ResourceBudgetLimits candidate = usage(state_->allocations.at(identity));
        if (!relieves_shortage(base, minimum, candidate, limits)) {
            continue;
        }
        subtract_usage(base, candidate);
        evicted.push_back(identity);
    }
    if (!fits(add_usage(base, minimum), limits)) {
        return ResourceReservation(
            {}, {},
            {.status = ResourceAdmissionStatus::over_budget,
             .work_item_count = request.work_item_count,
             .admitted_work_items = 0,
             .projected_usage = add_usage(original_base, minimum),
             .evicted_allocation_identities = {},
             .detail = request.operation + " cannot fit one bounded work item"});
    }

    const std::size_t batch = maximum_batch(base, fixed, per_item, limits, request.work_item_count);
    const ResourceBudgetLimits reserved = add_usage(fixed, usage(request.per_work_item, batch));
    for (const std::uint64_t identity : evicted) {
        state_->cache_eviction_callback(identity, state_->cache_eviction_user_data);
        state_->allocations.erase(identity);
    }
    state_->reserved = add_usage(state_->reserved, reserved);
    const ResourceAdmissionStatus status = batch == request.work_item_count
                                               ? ResourceAdmissionStatus::admitted_whole
                                               : ResourceAdmissionStatus::admitted_tiled;
    return ResourceReservation(
        state_, reserved,
        {.status = status,
         .work_item_count = request.work_item_count,
         .admitted_work_items = batch,
         .projected_usage = add_usage(base, reserved),
         .evicted_allocation_identities = std::move(evicted),
         .detail = status == ResourceAdmissionStatus::admitted_whole
                       ? request.operation + " admitted after cache eviction"
                       : request.operation + " admitted as bounded tiled work"});
}

PreviewQualityAdmission ResourceLedger::admit_preview_quality(
    const ResourceBudgetLimits& limits, const PreviewQualityRequest& request) {
    if (request.operation.empty() || request.full_quality_width == 0 ||
        request.full_quality_height == 0 || request.options.empty()) {
        throw std::invalid_argument(
            "preview quality requires an operation, full resolution, and ordered options");
    }
    std::vector<ResourceBudgetLimits> option_usage;
    option_usage.reserve(request.options.size());
    for (const PreviewQualityOption& option : request.options) {
        if (option.width == 0 || option.height == 0 || option.width > request.full_quality_width ||
            option.height > request.full_quality_height || option.requirements.empty()) {
            throw std::invalid_argument(
                "preview quality options require bounded dimensions and resources");
        }
        option_usage.push_back(requirement_usage(option.requirements));
    }

    std::lock_guard lock(state_->mutex);
    ResourceBudgetLimits base = state_->reserved;
    for (const auto& [identity, descriptor] : state_->allocations) {
        static_cast<void>(identity);
        base = add_usage(base, usage(descriptor));
    }

    std::size_t selected = request.options.size();
    ResourceBudgetLimits selected_base = base;
    std::vector<std::uint64_t> selected_evictions;
    for (std::size_t index = 0; index < request.options.size(); ++index) {
        if (fits(add_usage(base, option_usage[index]), limits)) {
            selected = index;
            break;
        }
    }

    if (selected == request.options.size() && state_->cache_eviction_callback != nullptr) {
        std::vector<std::uint64_t> candidates;
        candidates.reserve(state_->allocations.size());
        for (const auto& [identity, descriptor] : state_->allocations) {
            if (descriptor.category == ResourceCategory::cache &&
                (descriptor.roles & (resource_role_pinned | resource_role_in_flight)) == 0U) {
                candidates.push_back(identity);
            }
        }
        std::ranges::sort(candidates);
        for (std::size_t index = 0; index < request.options.size(); ++index) {
            ResourceBudgetLimits candidate_base = base;
            std::vector<std::uint64_t> evictions;
            for (const std::uint64_t identity : candidates) {
                if (fits(add_usage(candidate_base, option_usage[index]), limits)) {
                    break;
                }
                const ResourceBudgetLimits candidate = usage(state_->allocations.at(identity));
                if (relieves_shortage(candidate_base, option_usage[index], candidate, limits)) {
                    subtract_usage(candidate_base, candidate);
                    evictions.push_back(identity);
                }
            }
            if (fits(add_usage(candidate_base, option_usage[index]), limits)) {
                selected = index;
                selected_base = candidate_base;
                selected_evictions = std::move(evictions);
                break;
            }
        }
    }

    if (selected == request.options.size()) {
        return {
            .reservation = {},
            .report = {.status = PreviewQualityStatus::over_budget,
                       .selected_option = request.options.size(),
                       .full_quality_width = request.full_quality_width,
                       .full_quality_height = request.full_quality_height,
                       .selected_width = 0,
                       .selected_height = 0,
                       .derived_work_deferred = false,
                       .projected_usage = base,
                       .evicted_allocation_identities = {},
                       .detail = request.operation + " has no allowed preview quality that fits"}};
    }

    for (const std::uint64_t identity : selected_evictions) {
        state_->cache_eviction_callback(identity, state_->cache_eviction_user_data);
        state_->allocations.erase(identity);
    }
    state_->reserved = add_usage(state_->reserved, option_usage[selected]);
    const PreviewQualityOption& option = request.options[selected];
    const PreviewQualityStatus status = preview_status(request, option);
    const ResourceBudgetLimits projected = add_usage(selected_base, option_usage[selected]);
    ResourceAdmissionReport reservation_report = {
        .status = ResourceAdmissionStatus::admitted_whole,
        .projected_usage = projected,
        .evicted_allocation_identities = selected_evictions,
        .detail = request.operation + " preview reservation",
    };
    return {.reservation =
                ResourceReservation(state_, option_usage[selected], std::move(reservation_report)),
            .report = {.status = status,
                       .selected_option = selected,
                       .full_quality_width = request.full_quality_width,
                       .full_quality_height = request.full_quality_height,
                       .selected_width = option.width,
                       .selected_height = option.height,
                       .derived_work_deferred = option.derived_work_deferred,
                       .projected_usage = projected,
                       .evicted_allocation_identities = std::move(selected_evictions),
                       .detail = request.operation + " admitted with host preview option " +
                                 std::to_string(selected)}};
}

ResourceReservation::ResourceReservation(std::shared_ptr<ResourceLedgerState> state,
                                         ResourceBudgetLimits reserved,
                                         ResourceAdmissionReport report)
    : state_(std::move(state)), reserved_(reserved), report_(std::move(report)) {}

ResourceReservation::~ResourceReservation() { release(); }
ResourceReservation::ResourceReservation(ResourceReservation&& other) noexcept = default;

ResourceReservation& ResourceReservation::operator=(ResourceReservation&& other) noexcept {
    if (this != &other) {
        release();
        state_ = std::move(other.state_);
        reserved_ = other.reserved_;
        report_ = std::move(other.report_);
    }
    return *this;
}

bool ResourceReservation::active() const noexcept { return state_ != nullptr; }

void ResourceReservation::release() noexcept {
    if (!state_) {
        return;
    }
    std::lock_guard lock(state_->mutex);
    subtract_usage(state_->reserved, reserved_);
    state_.reset();
    reserved_ = {};
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

std::string_view resource_admission_status_name(ResourceAdmissionStatus status) noexcept {
    switch (status) {
        case ResourceAdmissionStatus::admitted_whole:
            return "admitted-whole";
        case ResourceAdmissionStatus::admitted_tiled:
            return "admitted-tiled";
        case ResourceAdmissionStatus::over_budget:
            return "over-budget";
    }
    return "unknown";
}

std::string_view preview_quality_status_name(PreviewQualityStatus status) noexcept {
    switch (status) {
        case PreviewQualityStatus::full_quality:
            return "full-quality";
        case PreviewQualityStatus::reduced_resolution:
            return "reduced-resolution";
        case PreviewQualityStatus::deferred_derived:
            return "deferred-derived";
        case PreviewQualityStatus::reduced_and_deferred:
            return "reduced-and-deferred";
        case PreviewQualityStatus::over_budget:
            return "over-budget";
    }
    return "unknown";
}

}  // namespace ctex::xport
