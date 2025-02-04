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

    if (vkCreateBuffer(device, &bufferInfo, nullptr, &_buffer) != VK_SUCCESS) {
        throw std::runtime_error("failed to create buffer!");
    }

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(device, _buffer, &memRequirements);

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
    if (vkAllocateMemory(device, &allocInfo, nullptr, &_memory) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate buffer memory!");
    }

    vkBindBufferMemory(device, _buffer, _memory, 0);
}

void Buffer::unmap(Device& device)
{

    if (mapped)
    {
        vkUnmapMemory(device, _memory);
        mapped = nullptr;
    }
}

VkResult Buffer::map(Device& device,VkDeviceSize size, VkDeviceSize offset)
{
    this->size = size;
    return vkMapMemory(device, _memory, offset, size, 0, &mapped);
}

void Buffer::destroy(Device& device)
{
    if (_buffer)
    {
        vkDestroyBuffer(device, _buffer, nullptr);
    }
    if (_memory)
    {
        vkFreeMemory(device, _memory, nullptr);
    }
}

VkResult Buffer::flush(Device& device, VkDeviceSize size, VkDeviceSize offset)
{
    VkMappedMemoryRange mappedRange = {};
    mappedRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
    mappedRange.memory = _memory;
    mappedRange.offset = offset;
    mappedRange.size = size;
    return vkFlushMappedMemoryRanges(device, 1, &mappedRange);
}

void Buffer::fillBuffer(Device& device, void* data, VkDeviceSize size)
{
    Buffer stagingBuffer;

    stagingBuffer = Buffer(device, size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    void* staging;
    vkMapMemory(device, stagingBuffer.GetMemory(), 0, size, 0, &staging);
    memcpy(staging, data, static_cast<size_t>(size));
    vkUnmapMemory(device, stagingBuffer.GetMemory());

    copyBuffer(device, stagingBuffer, *this, size);

    stagingBuffer.destroy(device);
    this->size = size;
}

VkDescriptorBufferInfo Buffer::GetBufferInfo()
{
    VkDescriptorBufferInfo descriptor{};
    descriptor.buffer = _buffer;
    descriptor.offset = 0;
    descriptor.range = size;
    return descriptor;
}
