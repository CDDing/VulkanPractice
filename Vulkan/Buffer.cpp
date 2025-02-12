#include "pch.h"
#include "Buffer.h"

Buffer::Buffer()
{
}

Buffer::Buffer(Device& device, vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties)
{
    vk::BufferCreateInfo bufferInfo{ {},size, usage,vk::SharingMode::eExclusive};

    _buffer = device.logical.createBuffer(bufferInfo);

    vk::MemoryRequirements memRequirements = device.logical.getBufferMemoryRequirements(_buffer);

    vk::MemoryAllocateInfo allocInfo{ memRequirements.size,findMemoryType(device,
        memRequirements.memoryTypeBits,properties)};
    vk::MemoryAllocateFlagsInfo memoryAllocateFlagsInfo{};
    if (usage & vk::BufferUsageFlagBits::eShaderDeviceAddress) {
        memoryAllocateFlagsInfo.flags = vk::MemoryAllocateFlagBits::eDeviceAddress;

        allocInfo.pNext = &memoryAllocateFlagsInfo;
    }

    _memory = device.logical.allocateMemory(allocInfo);

    device.logical.bindBufferMemory(_buffer, _memory,0);
}

void Buffer::unmap(vk::Device& device)
{

    if (mapped)
    {
        device.unmapMemory(_memory);
        mapped = nullptr;
    }
}

void Buffer::map(vk::Device& device,vk::DeviceSize size, vk::DeviceSize offset)
{
    this->size = size;
    mapped = device.mapMemory(_memory, offset, size);
}

void Buffer::destroy(vk::Device& device)
{
    if (_buffer)
    {
        device.destroyBuffer(_buffer);
    }
    if (_memory)
    {
        device.freeMemory(_memory);
    }
}

void Buffer::flush(vk::Device& device, vk::DeviceSize size, vk::DeviceSize offset)
{
    
    vk::MappedMemoryRange mappedRange = { _memory,offset,size };
    (device.flushMappedMemoryRanges(mappedRange));
}

void Buffer::fillBuffer(Device& device, void* data, vk::DeviceSize size)
{
    Buffer stagingBuffer;

    stagingBuffer = Buffer(device, size, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible| vk::MemoryPropertyFlagBits::eHostCoherent);

    void* staging = device.logical.mapMemory(stagingBuffer.GetMemory(),0,size);
    memcpy(staging, data, static_cast<size_t>(size));
    device.logical.unmapMemory(stagingBuffer.GetMemory());

    copyBuffer(device, stagingBuffer, *this, size);

    stagingBuffer.destroy(device);
    this->size = size;
}

vk::DescriptorBufferInfo Buffer::GetBufferInfo()
{
    vk::DescriptorBufferInfo descriptor{_buffer, 0, size};
    return descriptor;
}
