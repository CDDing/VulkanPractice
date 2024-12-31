#pragma once
class RenderPass
{
public:

	RenderPass();
	RenderPass(Device& device, VkFormat swapChainImageFormat, VkFormat DepthFormat);
	VkRenderPass& Get() { return _renderPass; }

private:
	VkRenderPass _renderPass;
};

