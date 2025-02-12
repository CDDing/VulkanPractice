#pragma once

class CommandPool
{
public:
	CommandPool();
	CommandPool(Device& device,QueueFamilyIndices queueFamilyIndices);
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
	void destroy(vk::Device& device) {
		device.destroyCommandPool(_commandPool);
	}
private:

	vk::CommandPool _commandPool;
};

