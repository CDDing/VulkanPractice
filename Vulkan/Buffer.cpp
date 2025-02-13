#include "pch.h"
#include "Buffer.h"

Buffer::Buffer()
{
}

Buffer::Buffer(std::shared_ptr<Device> device, vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties) : _device(device)
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
    this->size = size;
    device->logical.bindBufferMemory(_buffer, _memory,0);
}

void Buffer::unmap()
{

    if (mapped)
    {
        _device->logical.unmapMemory(_memory);
        mapped = nullptr;
    }
}

void Buffer::map(vk::DeviceSize size, vk::DeviceSize offset)
{
    this->size = size;
    mapped = _device->logical.mapMemory(_memory, offset, size);
}

Buffer::~Buffer()
{
    if (_buffer)
    {
        _device->logical.destroyBuffer(_buffer);
        _buffer = VK_NULL_HANDLE;
    }
    if (_memory)
    {
        _device->logical.freeMemory(_memory);
        _memory = VK_NULL_HANDLE;
    }
}

void Buffer::flush(vk::DeviceSize size, vk::DeviceSize offset)
{
    
    vk::MappedMemoryRange mappedRange = { _memory,offset,size };
    (_device->logical.flushMappedMemoryRanges(mappedRange));
}

void Buffer::fillBuffer(void* data, vk::DeviceSize size)
{
    Buffer stagingBuffer;

    stagingBuffer = Buffer(_device, size, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible| vk::MemoryPropertyFlagBits::eHostCoherent);

    void* staging = _device->logical.mapMemory(stagingBuffer.GetMemory(),0,size);
    memcpy(staging, data, static_cast<size_t>(size));
    _device->logical.unmapMemory(stagingBuffer.GetMemory());

    copyBuffer(_device, stagingBuffer, *this, size);

    this->size = size;
}

vk::DescriptorBufferInfo Buffer::GetBufferInfo()
{
    vk::DescriptorBufferInfo descriptor{_buffer, 0, size};
    return descriptor;
}
