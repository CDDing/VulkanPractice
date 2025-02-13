#include "pch.h"
#include "CommandPool.h"
CommandPool CommandPool::TransientPool;
CommandPool::CommandPool()
{
}

CommandPool::CommandPool(std::shared_ptr<Device> device, QueueFamilyIndices queueFamilyIndices) : _device(device)
{
    //QueueFamilyIndices queueFamilyIndices = findQueueFamilies(device, surface);
    if (TransientPool._commandPool == VK_NULL_HANDLE) {
        vk::CommandPoolCreateInfo poolInfo{ vk::CommandPoolCreateFlagBits::eTransient,
    queueFamilyIndices.graphicsFamily.value() };
        TransientPool._commandPool = device->logical.createCommandPool(poolInfo);
        TransientPool._device = _device;

    }

    vk::CommandPoolCreateInfo poolInfo{vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
    queueFamilyIndices.graphicsFamily.value() };

    _commandPool = device->logical.createCommandPool(poolInfo);
}
