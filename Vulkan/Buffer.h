#pragma once
class Buffer
{
public:
	Buffer();
	Buffer(Device& device, vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties);
	operator vk::Buffer& () {
		return _buffer;
	}
	bool operator==(void* data) {
		return _buffer == data;
	}
	vk::DeviceMemory& GetMemory() { return _memory; }
	void* mapped = nullptr;
	void unmap(vk::Device& device);
	void map(vk::Device & device, vk::DeviceSize size = vk::WholeSize, vk::DeviceSize offset = 0);
	void destroy(vk::Device& device);
	void flush(vk::Device& device, vk::DeviceSize size = vk::WholeSize, vk::DeviceSize offset = 0);
	void fillBuffer(Device& device, void* data, vk::DeviceSize size);
	vk::DeviceSize size;
	vk::DescriptorBufferInfo GetBufferInfo();
private:
	vk::Buffer _buffer = VK_NULL_HANDLE;
	vk::DeviceMemory _memory = VK_NULL_HANDLE;
};

