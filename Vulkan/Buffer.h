#pragma once
class Buffer
{
public:
	Buffer();
	Buffer(std::shared_ptr<Device> device, vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties);
	~Buffer();
	//복사
	Buffer(const Buffer& other) noexcept :size(other.size), mapped(other.mapped),_buffer(other._buffer), _memory(other._memory), _device(other._device){

	}
	Buffer& operator=(const Buffer& other) {
		_buffer = other._buffer;
		_memory = other._memory;
		size = other.size;
		mapped = other.mapped;
		_device = other._device;
		return *this;
	}

	//이동
	Buffer(Buffer&& other) noexcept : size(other.size),mapped(other.mapped),_buffer(other._buffer), _memory(other._memory), _device(std::move(other._device)) {
		other._memory = VK_NULL_HANDLE;
		other._buffer = VK_NULL_HANDLE;
		other.size = 0;
	}
	Buffer& operator=(Buffer&& other) noexcept {
		if (this != &other)
		{
			if (_memory != VK_NULL_HANDLE)
			{
				_device->logical.freeMemory(_memory);
			}
			if (_buffer != VK_NULL_HANDLE)
			{
				_device->logical.destroyBuffer(_buffer);

			}
			_buffer = other._buffer;
			_memory = other._memory;
			size = other.size;
			mapped = other.mapped;
			_device = std::move(other._device);

			other.mapped = nullptr;
			other._memory = VK_NULL_HANDLE;
			other._buffer = VK_NULL_HANDLE;
		}
		return *this;
	}

	operator vk::Buffer& () {
		return _buffer;
	}
	bool operator==(void* data) {
		return _buffer == data;
	}
	vk::DeviceMemory& GetMemory() { return _memory; }
	void* mapped = nullptr;
	void unmap();
	void map(vk::DeviceSize size = vk::WholeSize, vk::DeviceSize offset = 0);
	void flush(vk::DeviceSize size = vk::WholeSize, vk::DeviceSize offset = 0);
	void fillBuffer(void* data, vk::DeviceSize size);
	vk::DeviceSize size;
	vk::DescriptorBufferInfo GetBufferInfo();
private:
	vk::Buffer _buffer = VK_NULL_HANDLE;
	vk::DeviceMemory _memory = VK_NULL_HANDLE;
	std::shared_ptr<Device> _device;
};

