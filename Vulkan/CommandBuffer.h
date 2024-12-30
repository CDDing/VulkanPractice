#pragma once
class CommandBuffer
{
public:
	CommandBuffer();
	CommandBuffer(Device* device, CommandPool* commandPool);
	VkCommandBuffer& Get() { return _commandBuffer; }
private:
	VkCommandBuffer _commandBuffer;
};


VkCommandBuffer beginSingleTimeCommands(VkDevice device, VkCommandPool commandPool);
void endSingleTimeCommands(VkDevice device, VkCommandPool commandPool, VkQueue queue, VkCommandBuffer commandBuffer);