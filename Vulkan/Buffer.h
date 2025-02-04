#pragma once
class Buffer
{
public:
	Buffer();
	Buffer(Device& device, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties);
	operator VkBuffer& () {
		return _buffer;
	}
	VkDeviceMemory& GetMemory() { return _memory; }
	void* mapped = nullptr;
	void unmap(Device& device);
	VkResult map(Device& device, VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0);
	void destroy(Device& device);
	VkResult flush(Device& device, VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0);
	void fillBuffer(Device& device, void* data, VkDeviceSize size);
	VkDeviceSize size;
	VkDescriptorBufferInfo GetBufferInfo();
private:
	VkBuffer _buffer = VK_NULL_HANDLE;
	VkDeviceMemory _memory = VK_NULL_HANDLE;
};

