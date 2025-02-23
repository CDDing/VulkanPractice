#include "pch.h"
#include "Utils.h"
uint32_t findMemoryType(vk::raii::PhysicalDevice& device, uint32_t typeFilter, vk::MemoryPropertyFlags properties) {
    vk::PhysicalDeviceMemoryProperties memProperties = device.getMemoryProperties();

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if (typeFilter & (1 << i) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }

    throw std::runtime_error("failed to find suitable memory type!");
}
std::ostream& operator<<(std::ostream& os, const glm::vec3& vec) {
    os << "(" << vec.x << ", " << vec.y << ", " << vec.z << ")";
    return os;
}
uint32_t SBTalignedSize(uint32_t value, uint32_t alignment)
{
    return (value + alignment - 1) & ~(alignment - 1);
}
vk::raii::CommandBuffer beginSingleTimeCommands(Device& device) {
    vk::CommandBufferAllocateInfo allocInfo{ CommandPool::TransientPool ,vk::CommandBufferLevel::ePrimary,1};

    vk::raii::CommandBuffer commandBuffer = std::move(vk::raii::CommandBuffers(device,allocInfo).front());

    vk::CommandBufferBeginInfo beginInfo{ vk::CommandBufferUsageFlagBits::eOneTimeSubmit };
    commandBuffer.begin(beginInfo);

    return commandBuffer;
}
void endSingleTimeCommands(Device& device, vk::raii::CommandBuffer& commandBuffer) {
    commandBuffer.end();
    
    vk::SubmitInfo submitInfo{};
	submitInfo.setCommandBuffers({ *commandBuffer });
    device.GetQueue(Device::QueueType::GRAPHICS).submit(submitInfo, VK_NULL_HANDLE);
    device.GetQueue(Device::QueueType::GRAPHICS).waitIdle();

}
void copyBuffer(Device& device, DBuffer& srcBuffer, DBuffer& dstBuffer, vk::DeviceSize size) {
    vk::raii::CommandBuffer commandBuffer = beginSingleTimeCommands(device);

    vk::BufferCopy copyRegion{};
    copyRegion.size = size;
    commandBuffer.copyBuffer(srcBuffer.buffer, dstBuffer.buffer, copyRegion);

    endSingleTimeCommands(device, commandBuffer);
    dstBuffer.size = size;

}

