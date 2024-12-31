#pragma once
class Buffer
{
public:
	Buffer();
	Buffer(Device& device, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties);
	VkBuffer& Get() { return _buffer; }
	VkDeviceMemory& GetMemory() { return _memory; }
private:
	VkBuffer _buffer;
	VkDeviceMemory _memory;
};

