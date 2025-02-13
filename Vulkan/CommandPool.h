#pragma once

class CommandPool
{
public:
	CommandPool();
	CommandPool(std::shared_ptr<Device> device,QueueFamilyIndices queueFamilyIndices);
	~CommandPool() {
		if(_commandPool != VK_NULL_HANDLE) _device->logical.destroyCommandPool(_commandPool);
		_commandPool = VK_NULL_HANDLE;
	}
	static CommandPool TransientPool;
	operator vk::CommandPool& () {
		return _commandPool;
	}
	CommandPool& operator=(vk::CommandPool commandPool) {
		_commandPool = commandPool;
		return *this;
	}
	vk::CommandPool* operator&() {
		return &_commandPool;
	}
private:

	vk::CommandPool _commandPool = VK_NULL_HANDLE;
	std::shared_ptr<Device> _device;
};

