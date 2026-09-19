#include <ctex/version.h>
#include <volk.h>

#include <algorithm>
#include <array>
#include <ctex/exec/vulkan_executor.hpp>
#include <limits>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>
#include <vector>

namespace ctex::exec {
namespace {

struct VulkanState {
    VkInstance instance{VK_NULL_HANDLE};
    VkDevice device{VK_NULL_HANDLE};
    VkQueue queue{VK_NULL_HANDLE};
    VolkDeviceTable table{};

    ~VulkanState() {
        if (device != VK_NULL_HANDLE) {
            if (table.vkDeviceWaitIdle != nullptr) {
                static_cast<void>(table.vkDeviceWaitIdle(device));
            }
            table.vkDestroyDevice(device, nullptr);
        }
        if (instance != VK_NULL_HANDLE) {
            vkDestroyInstance(instance, nullptr);
        }
    }
};

class OwnedVulkanExecutor final : public Executor {
public:
    OwnedVulkanExecutor(ExecutorDescriptor descriptor, std::unique_ptr<VulkanState> state)
        : descriptor_(std::move(descriptor)), state_(std::move(state)) {}

    [[nodiscard]] const ExecutorDescriptor& descriptor() const noexcept override {
        return descriptor_;
    }

private:
    ExecutorDescriptor descriptor_;
    std::unique_ptr<VulkanState> state_;
};

emit::DeviceFeatureSet unavailable_features() {
    return {.binding_budget = 2,
            .maximum_texture_dimension = 4096,
            .supported_texture_formats = {emit::TextureFormat::rgba8_unorm},
            .floating_point_filtering = false,
            .compute_available = false};
}

ExecutorDescriptor descriptor(std::string device_name, ExecutorAvailability availability,
                              emit::DeviceFeatureSet features) {
    return {.identifier = "vulkan",
            .display_name = "Vulkan owned GPU",
            .device_name = std::move(device_name),
            .route = ExecutorRoute::owned_gpu,
            .availability = availability,
            .features = std::move(features)};
}

VulkanExecutorCreation unavailable(std::string message) {
    auto executor = std::make_shared<OwnedVulkanExecutor>(
        descriptor("No Vulkan device", ExecutorAvailability::device_unavailable,
                   unavailable_features()),
        nullptr);
    return {.compiled = true, .executor = std::move(executor), .message = std::move(message)};
}

std::string failure(std::string_view operation, VkResult result) {
    return std::string(operation) + " failed with VkResult " +
           std::to_string(static_cast<std::int32_t>(result));
}

struct Candidate {
    VkPhysicalDevice physical_device{VK_NULL_HANDLE};
    VkPhysicalDeviceProperties properties{};
    std::uint32_t queue_family{};
};

int device_rank(VkPhysicalDeviceType type) {
    switch (type) {
        case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
            return 0;
        case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
            return 1;
        case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
            return 2;
        case VK_PHYSICAL_DEVICE_TYPE_CPU:
            return 3;
        case VK_PHYSICAL_DEVICE_TYPE_OTHER:
        default:
            return 4;
    }
}

std::optional<std::uint32_t> execution_queue_family(VkPhysicalDevice physical_device) {
    std::uint32_t count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &count, nullptr);
    std::vector<VkQueueFamilyProperties> properties(count);
    vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &count, properties.data());
    constexpr VkQueueFlags required = VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT;
    for (std::uint32_t index = 0; index < count; ++index) {
        if (properties[index].queueCount != 0 &&
            (properties[index].queueFlags & required) == required) {
            return index;
        }
    }
    return std::nullopt;
}

std::vector<Candidate> candidates(VkInstance instance, std::string& error) {
    std::uint32_t count = 0;
    VkResult result = vkEnumeratePhysicalDevices(instance, &count, nullptr);
    if (result != VK_SUCCESS) {
        error = failure("vkEnumeratePhysicalDevices", result);
        return {};
    }
    std::vector<VkPhysicalDevice> devices(count);
    result = vkEnumeratePhysicalDevices(instance, &count, devices.data());
    if (result != VK_SUCCESS && result != VK_INCOMPLETE) {
        error = failure("vkEnumeratePhysicalDevices", result);
        return {};
    }
    devices.resize(count);
    std::vector<Candidate> result_candidates;
    for (VkPhysicalDevice physical_device : devices) {
        const auto queue_family = execution_queue_family(physical_device);
        if (!queue_family) {
            continue;
        }
        Candidate candidate{.physical_device = physical_device, .queue_family = *queue_family};
        vkGetPhysicalDeviceProperties(physical_device, &candidate.properties);
        result_candidates.push_back(candidate);
    }
    std::ranges::sort(result_candidates, [](const Candidate& left, const Candidate& right) {
        const auto left_key = std::tuple{device_rank(left.properties.deviceType),
                                         std::string_view(left.properties.deviceName),
                                         left.properties.vendorID, left.properties.deviceID};
        const auto right_key = std::tuple{device_rank(right.properties.deviceType),
                                          std::string_view(right.properties.deviceName),
                                          right.properties.vendorID, right.properties.deviceID};
        return left_key < right_key;
    });
    return result_candidates;
}

bool supports(VkPhysicalDevice physical_device, VkFormat format, VkFormatFeatureFlags required) {
    VkFormatProperties properties{};
    vkGetPhysicalDeviceFormatProperties(physical_device, format, &properties);
    return (properties.optimalTilingFeatures & required) == required;
}

emit::DeviceFeatureSet device_features(const Candidate& candidate) {
    struct FormatMapping {
        emit::TextureFormat cybertexel;
        VkFormat vulkan;
        VkFormatFeatureFlags required;
    };
    constexpr VkFormatFeatureFlags colour_required =
        VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT | VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT;
    constexpr std::array mappings{
        FormatMapping{emit::TextureFormat::r8_unorm, VK_FORMAT_R8_UNORM, colour_required},
        FormatMapping{emit::TextureFormat::rg8_unorm, VK_FORMAT_R8G8_UNORM, colour_required},
        FormatMapping{emit::TextureFormat::rgba8_unorm, VK_FORMAT_R8G8B8A8_UNORM, colour_required},
        FormatMapping{emit::TextureFormat::r16_unorm, VK_FORMAT_R16_UNORM, colour_required},
        FormatMapping{emit::TextureFormat::rg16_unorm, VK_FORMAT_R16G16_UNORM, colour_required},
        FormatMapping{emit::TextureFormat::rgba16_unorm, VK_FORMAT_R16G16B16A16_UNORM,
                      colour_required},
        FormatMapping{emit::TextureFormat::r16_float, VK_FORMAT_R16_SFLOAT, colour_required},
        FormatMapping{emit::TextureFormat::rg16_float, VK_FORMAT_R16G16_SFLOAT, colour_required},
        FormatMapping{emit::TextureFormat::rgba16_float, VK_FORMAT_R16G16B16A16_SFLOAT,
                      colour_required},
        FormatMapping{emit::TextureFormat::r32_float, VK_FORMAT_R32_SFLOAT, colour_required},
        FormatMapping{emit::TextureFormat::rg32_float, VK_FORMAT_R32G32_SFLOAT, colour_required},
        FormatMapping{emit::TextureFormat::rgba32_float, VK_FORMAT_R32G32B32A32_SFLOAT,
                      colour_required},
        FormatMapping{
            emit::TextureFormat::depth32_float, VK_FORMAT_D32_SFLOAT,
            VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT | VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT},
    };
    std::vector<emit::TextureFormat> formats;
    for (const FormatMapping& mapping : mappings) {
        if (supports(candidate.physical_device, mapping.vulkan, mapping.required)) {
            formats.push_back(mapping.cybertexel);
        }
    }
    const bool float_filtering = supports(candidate.physical_device, VK_FORMAT_R16G16B16A16_SFLOAT,
                                          VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT) &&
                                 supports(candidate.physical_device, VK_FORMAT_R32G32B32A32_SFLOAT,
                                          VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT);
    const auto& limits = candidate.properties.limits;
    return {.binding_budget =
                std::min(limits.maxPerStageDescriptorSampledImages, limits.maxPerStageResources),
            .maximum_texture_dimension = limits.maxImageDimension2D,
            .supported_texture_formats = std::move(formats),
            .floating_point_filtering = float_filtering,
            .compute_available = true};
}

std::string api_version_name(std::uint32_t version) {
    return std::to_string(VK_VERSION_MAJOR(version)) + "." +
           std::to_string(VK_VERSION_MINOR(version)) + "." +
           std::to_string(VK_VERSION_PATCH(version));
}

}  // namespace

VulkanExecutorCreation create_vulkan_executor() {
    static std::mutex initialization_mutex;
    std::lock_guard initialization_lock(initialization_mutex);
    const VkResult initialization = volkInitialize();
    if (initialization != VK_SUCCESS) {
        return unavailable(failure("volkInitialize", initialization));
    }

    VkApplicationInfo application{};
    application.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    application.pApplicationName = "CyberTexel";
    application.applicationVersion =
        VK_MAKE_VERSION(CTEX_VERSION_MAJOR, CTEX_VERSION_MINOR, CTEX_VERSION_PATCH);
    application.pEngineName = "CyberTexel";
    application.engineVersion = application.applicationVersion;
    application.apiVersion = VK_API_VERSION_1_0;
    VkInstanceCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    create_info.pApplicationInfo = &application;
    VkInstance instance = VK_NULL_HANDLE;
    const VkResult instance_result = vkCreateInstance(&create_info, nullptr, &instance);
    if (instance_result != VK_SUCCESS) {
        return unavailable(failure("vkCreateInstance", instance_result));
    }
    volkLoadInstanceOnly(instance);
    auto state = std::make_unique<VulkanState>();
    state->instance = instance;

    std::string discovery_error;
    std::vector<Candidate> devices = candidates(instance, discovery_error);
    if (devices.empty()) {
        state.reset();
        return unavailable(discovery_error.empty()
                               ? "no Vulkan physical device exposes graphics and compute queues"
                               : std::move(discovery_error));
    }
    const Candidate& selected = devices.front();
    emit::DeviceFeatureSet features = device_features(selected);
    if (features.binding_budget < 2 || features.maximum_texture_dimension == 0 ||
        features.supported_texture_formats.empty()) {
        const std::string name(selected.properties.deviceName);
        state.reset();
        return unavailable("Vulkan device '" + name +
                           "' does not expose the required texture capabilities");
    }

    constexpr float queue_priority = 1.0F;
    VkDeviceQueueCreateInfo queue_create_info{};
    queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queue_create_info.queueFamilyIndex = selected.queue_family;
    queue_create_info.queueCount = 1;
    queue_create_info.pQueuePriorities = &queue_priority;
    VkDeviceCreateInfo device_create_info{};
    device_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    device_create_info.queueCreateInfoCount = 1;
    device_create_info.pQueueCreateInfos = &queue_create_info;
    const VkResult device_result =
        vkCreateDevice(selected.physical_device, &device_create_info, nullptr, &state->device);
    if (device_result != VK_SUCCESS) {
        state.reset();
        return unavailable(failure("vkCreateDevice", device_result));
    }
    volkLoadDeviceTable(&state->table, state->device);
    state->table.vkGetDeviceQueue(state->device, selected.queue_family, 0, &state->queue);
    if (state->queue == VK_NULL_HANDLE) {
        state.reset();
        return unavailable("vkGetDeviceQueue returned no execution queue");
    }

    const std::string device_name(selected.properties.deviceName);
    const std::string message = "initialized Vulkan " +
                                api_version_name(selected.properties.apiVersion) + " device '" +
                                device_name + "'";
    auto executor = std::make_shared<OwnedVulkanExecutor>(
        descriptor(device_name, ExecutorAvailability::available, std::move(features)),
        std::move(state));
    return {.compiled = true, .executor = std::move(executor), .message = message};
}

}  // namespace ctex::exec
