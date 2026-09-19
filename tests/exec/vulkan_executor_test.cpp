#include <ctex/exec/cpu_reference.hpp>
#include <ctex/exec/vulkan_executor.hpp>
#include <iostream>
#include <memory>
#include <string_view>

namespace {

bool expect(bool condition, std::string_view message) {
    if (!condition) {
        std::cerr << message << '\n';
    }
    return condition;
}

bool disabled_build_has_no_vulkan_route(const ctex::exec::VulkanExecutorCreation& creation) {
    return expect(!creation.compiled && creation.executor == nullptr,
                  "disabled build exposed a Vulkan executor") &&
           expect(creation.message.find("CTEX_ENABLE_VULKAN_EXECUTOR=ON") != std::string::npos,
                  "disabled Vulkan diagnostic did not name the build flag");
}

bool compiled_build_has_a_registerable_route(const ctex::exec::VulkanExecutorCreation& creation,
                                             bool require_device) {
    if (!expect(creation.compiled && creation.executor != nullptr && !creation.message.empty(),
                "enabled build did not expose a Vulkan executor and diagnostic")) {
        return false;
    }
    const auto& descriptor = creation.executor->descriptor();
    const bool descriptor_valid =
        descriptor.identifier == "vulkan" && descriptor.display_name == "Vulkan owned GPU" &&
        descriptor.route == ctex::exec::ExecutorRoute::owned_gpu &&
        descriptor.availability != ctex::exec::ExecutorAvailability::host_not_attached &&
        !descriptor.device_name.empty() && descriptor.features.binding_budget >= 2 &&
        descriptor.features.maximum_texture_dimension != 0 &&
        !descriptor.features.supported_texture_formats.empty();
    if (!expect(descriptor_valid, "Vulkan executor published an incomplete descriptor")) {
        return false;
    }
    if (require_device &&
        !expect(descriptor.availability == ctex::exec::ExecutorAvailability::available,
                creation.message)) {
        return false;
    }

    ctex::exec::ExecutorRegistry registry;
    registry.add(std::make_shared<ctex::exec::CpuReferenceExecutor>());
    registry.add(creation.executor);
    const auto enumerated = registry.enumerate();
    if (!expect(enumerated.size() == 2 && enumerated[1].identifier == "vulkan",
                "compiled Vulkan route did not register with stable identity")) {
        return false;
    }
    const auto automatic = registry.select_automatic();
    const std::string_view expected =
        descriptor.availability == ctex::exec::ExecutorAvailability::available ? "vulkan" : "cpu";
    return expect(automatic.executor->descriptor().identifier == expected,
                  "automatic selection ignored Vulkan runtime availability");
}

}  // namespace

int main(int argc, char** argv) {
    const bool require_device = argc == 2 && std::string_view(argv[1]) == "--require-device";
    const auto creation = ctex::exec::create_vulkan_executor();
    if (!creation.compiled) {
        return !require_device && disabled_build_has_no_vulkan_route(creation) ? 0 : 1;
    }
    if (require_device) {
        std::cout << creation.message << '\n';
    }
    return compiled_build_has_a_registerable_route(creation, require_device) ? 0 : 1;
}
