#pragma once
class CommandPool
{
public:
	CommandPool();
	CommandPool(Device device, VkSurfaceKHR* surface);
	VkCommandPool& Get() { return _commandPool; }
private:

	VkCommandPool _commandPool;
};

