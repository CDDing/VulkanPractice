#include "pch.h"
#include "Buffer.h"

Buffer::Buffer()
{
}

Buffer::Buffer(Device& device, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties)
{
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(device.Get(), &bufferInfo, nullptr, &_buffer) != VK_SUCCESS) {
        throw std::runtime_error("failed to create buffer!");
    }

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(device.Get(), _buffer, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(device, memRequirements.memoryTypeBits, properties);
    VkMemoryAllocateFlagsInfo memoryAllocateFlagsInfo{};
    if (usage & VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT) {
        memoryAllocateFlagsInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO;
        memoryAllocateFlagsInfo.flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT_KHR;

        allocInfo.pNext = &memoryAllocateFlagsInfo;
    }
    if (vkAllocateMemory(device.Get(), &allocInfo, nullptr, &_memory) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate buffer memory!");
    }

    vkBindBufferMemory(device.Get(), _buffer, _memory, 0);
}

void Buffer::unmap(Device& device)
{

    if (mapped)
    {
        vkUnmapMemory(device.Get(), _memory);
        mapped = nullptr;
    }
}

VkResult Buffer::map(Device& device,VkDeviceSize size, VkDeviceSize offset)
{
    return vkMapMemory(device.Get(), _memory, offset, size, 0, &mapped);
}

void Buffer::destroy(Device& device)
{
    if (_buffer)
    {
        vkDestroyBuffer(device.Get(), _buffer, nullptr);
    }
    if (_memory)
    {
        vkFreeMemory(device.Get(), _memory, nullptr);
    }
}

VkResult Buffer::flush(Device& device, VkDeviceSize size, VkDeviceSize offset)
{
    VkMappedMemoryRange mappedRange = {};
    mappedRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
    mappedRange.memory = _memory;
    mappedRange.offset = offset;
    mappedRange.size = size;
    return vkFlushMappedMemoryRanges(device.Get(), 1, &mappedRange);
}

void copyBuffer(Device& device, VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) {
    VkCommandBuffer commandBuffer = beginSingleTimeCommands(device);

    VkBufferCopy copyRegion{};
    copyRegion.size = size;
    vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

    endSingleTimeCommands(device, commandBuffer);

}