#include "pch.h"
#include "Buffer.h"

Buffer::Buffer()
{
}

Buffer::Buffer(std::shared_ptr<Device> device, vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties)
{
    vk::BufferCreateInfo bufferInfo{ {},size, usage,vk::SharingMode::eExclusive};

    _buffer = device->logical.createBuffer(bufferInfo);

    vk::MemoryRequirements memRequirements = device->logical.getBufferMemoryRequirements(_buffer);

    vk::MemoryAllocateInfo allocInfo{ memRequirements.size,findMemoryType(*device,
        memRequirements.memoryTypeBits,properties)};
    vk::MemoryAllocateFlagsInfo memoryAllocateFlagsInfo{};
    if (usage & vk::BufferUsageFlagBits::eShaderDeviceAddress) {
        memoryAllocateFlagsInfo.flags = vk::MemoryAllocateFlagBits::eDeviceAddress;

        allocInfo.pNext = &memoryAllocateFlagsInfo;
    }

    _memory = device->logical.allocateMemory(allocInfo);

    device->logical.bindBufferMemory(_buffer, _memory,0);
}

void Buffer::unmap(std::shared_ptr<Device> device)
{

    if (mapped)
    {
        device->logical.unmapMemory(_memory);
        mapped = nullptr;
    }
}

void Buffer::map(std::shared_ptr<Device> device,vk::DeviceSize size, vk::DeviceSize offset)
{
    this->size = size;
    mapped = device->logical.mapMemory(_memory, offset, size);
}

void Buffer::destroy(std::shared_ptr<Device> device)
{
    if (_buffer)
    {
        device->logical.destroyBuffer(_buffer);
    }
    if (_memory)
    {
        device->logical.freeMemory(_memory);
    }
}

void Buffer::flush(std::shared_ptr<Device> device, vk::DeviceSize size, vk::DeviceSize offset)
{
    
    vk::MappedMemoryRange mappedRange = { _memory,offset,size };
    device->logical.flushMappedMemoryRanges(mappedRange);
}

void Buffer::fillBuffer(std::shared_ptr<Device> device, void* data, vk::DeviceSize size)
{
    Buffer stagingBuffer;

    stagingBuffer = Buffer(device, size, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible| vk::MemoryPropertyFlagBits::eHostCoherent);

    stagingBuffer.map(device, size);
    memcpy(stagingBuffer.mapped, data, static_cast<size_t>(size));
    stagingBuffer.unmap(device);

    copyBuffer(device, stagingBuffer, *this, size);

    stagingBuffer.destroy(device);
    this->size = size;
}

vk::DescriptorBufferInfo Buffer::GetBufferInfo()
{
    vk::DescriptorBufferInfo descriptor{_buffer, 0, size};
    return descriptor;
}
