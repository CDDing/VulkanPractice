#include "pch.h"
#include "CommandPool.h"
CommandPool CommandPool::TransientPool = CommandPool();
CommandPool::CommandPool()
{
}

CommandPool::CommandPool(Device& device,Surface& surface)
{
    QueueFamilyIndices queueFamilyIndices = findQueueFamilies(device, surface);
    if (TransientPool == NULL) {
        VkCommandPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
        poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();

        if (vkCreateCommandPool(device, &poolInfo, nullptr, &TransientPool) != VK_SUCCESS) {
            throw std::runtime_error("failed to create transient pool!");
        }

    }

    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();

    if (vkCreateCommandPool(device, &poolInfo, nullptr, &_commandPool) != VK_SUCCESS) {
        throw std::runtime_error("failed to create command pool!");
    }
}
