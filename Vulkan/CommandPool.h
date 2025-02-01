#pragma once

class CommandPool
{
public:
	CommandPool();
	CommandPool(Device& device, Surface& surface);
	static VkCommandPool TransientPool;
	operator VkCommandPool& () {
		return _commandPool;
	}
private:

	VkCommandPool _commandPool;
};

