#pragma once

class CommandPool
{
public:
	CommandPool();
	CommandPool(Device& device, Surface& surface);
	static CommandPool TransientPool;
	operator VkCommandPool& () {
		return _commandPool;
	}
	VkCommandPool* operator&() {
		return &_commandPool;
	}
	void destroy(Device& device) {
		vkDestroyCommandPool(device, _commandPool, nullptr);
	}
private:

	VkCommandPool _commandPool;
};

