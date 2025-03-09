#include "pch.h"
#include "Buffer.h"

void DBuffer::unmap()
{
    if (mapped)
    {
        memory.unmapMemory();
        mapped = nullptr;
    }
}

void DBuffer::map(vk::DeviceSize size, vk::DeviceSize offset)
{
    mapped = memory.mapMemory(offset, size);
}

void DBuffer::flush(DContext& context, vk::DeviceSize size, vk::DeviceSize offset)
{

    vk::MappedMemoryRange mappedRange = { memory,offset,size };
    context.logical.flushMappedMemoryRanges(mappedRange);
}

void DBuffer::fillBuffer(DContext& context, void* data, vk::DeviceSize size)
{
    DBuffer stagingBuffer(context, size, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);

    stagingBuffer.map(size);
    memcpy(stagingBuffer.mapped, data, static_cast<size_t>(size));
    stagingBuffer.unmap();

    copyBuffer(context, stagingBuffer, *this, size);

    this->size = size;
}

vk::DescriptorBufferInfo DBuffer::GetBufferInfo()
{
    vk::DescriptorBufferInfo descriptor{ buffer, 0, size };
    return descriptor;
}
