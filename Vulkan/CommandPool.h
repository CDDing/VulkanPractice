#pragma once

class CommandPool
{
public:
	CommandPool();
	CommandPool(Device& device, Surface& surface);
	VkCommandPool& Get() { return _commandPool; }
	static VkCommandPool TransientPool;
private:

	VkCommandPool _commandPool;
};

