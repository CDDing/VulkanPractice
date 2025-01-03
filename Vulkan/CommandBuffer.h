#pragma once
class CommandBuffer
{
public:
	CommandBuffer();
	CommandBuffer(Device& device, CommandPool& commandPool);
	VkCommandBuffer& Get() { return _commandBuffer; }
private:
	VkCommandBuffer _commandBuffer;
};


VkCommandBuffer beginSingleTimeCommands(Device& device);
void endSingleTimeCommands(Device& device, VkCommandBuffer commandBuffer);