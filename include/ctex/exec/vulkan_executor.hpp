#ifndef CTEX_EXEC_VULKAN_EXECUTOR_HPP
#define CTEX_EXEC_VULKAN_EXECUTOR_HPP

#include <ctex/exec/executor.hpp>
#include <memory>
#include <string>

namespace ctex::exec {

struct VulkanExecutorCreation {
    bool compiled{};
    std::shared_ptr<const Executor> executor;
    std::string message;
};

// Returns no executor when the build flag is disabled. An enabled build always
// returns a descriptor, using device_unavailable when no Vulkan device can be
// created on the current machine.
[[nodiscard]] VulkanExecutorCreation create_vulkan_executor();

}  // namespace ctex::exec

#endif
