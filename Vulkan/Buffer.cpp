#include "pch.h"
#include "Buffer.h"

void DBuffer::unmap(Device& device)
{
    if (mapped)
    {
        memory.unmapMemory();
        mapped = nullptr;
    }
}

void DBuffer::map(Device& device, vk::DeviceSize size, vk::DeviceSize offset)
{
    mapped = memory.mapMemory(offset, size);
}

void DBuffer::flush(Device& device, vk::DeviceSize size, vk::DeviceSize offset)
{

    vk::MappedMemoryRange mappedRange = { memory,offset,size };
    device.logical.flushMappedMemoryRanges(mappedRange);
}

void DBuffer::fillBuffer(Device& device, void* data, vk::DeviceSize size)
{
    DBuffer stagingBuffer(device, size, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);

    stagingBuffer.map(device, size);
    memcpy(stagingBuffer.mapped, data, static_cast<size_t>(size));
    stagingBuffer.unmap(device);

    copyBuffer(device, stagingBuffer, *this, size);

    this->size = size;
}

vk::DescriptorBufferInfo DBuffer::GetBufferInfo()
{
    vk::DescriptorBufferInfo descriptor{ buffer, 0, size };
    return descriptor;
}
