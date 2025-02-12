#include "pch.h"
#include "CommandPool.h"
CommandPool CommandPool::TransientPool = CommandPool();
CommandPool::CommandPool()
{
}

CommandPool::CommandPool(Device& device, QueueFamilyIndices queueFamilyIndices)
{
    //QueueFamilyIndices queueFamilyIndices = findQueueFamilies(device, surface);
    if (TransientPool._commandPool == VK_NULL_HANDLE) {
        vk::CommandPoolCreateInfo poolInfo{ vk::CommandPoolCreateFlagBits::eTransient,
    queueFamilyIndices.graphicsFamily.value() };

        TransientPool = device.logical.createCommandPool(poolInfo);

    }

    vk::CommandPoolCreateInfo poolInfo{vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
    queueFamilyIndices.graphicsFamily.value() };

    _commandPool = device.logical.createCommandPool(poolInfo);
}
