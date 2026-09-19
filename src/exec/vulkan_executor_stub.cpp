#include <ctex/exec/vulkan_executor.hpp>

namespace ctex::exec {

VulkanExecutorCreation create_vulkan_executor() {
    return {.compiled = false,
            .executor = {},
            .message = "Vulkan executor is not compiled; configure CTEX_ENABLE_VULKAN_EXECUTOR=ON"};
}

}  // namespace ctex::exec
