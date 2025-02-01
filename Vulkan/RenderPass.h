#pragma once
class RenderPass
{
public:

	RenderPass();
	RenderPass(Device& device, VkFormat swapChainImageFormat, VkFormat DepthFormat);
	operator VkRenderPass& () {
		return _renderPass;
	}

	VkRenderPass* operator&() {
		return &_renderPass;
	}

private:
	VkRenderPass _renderPass;
};

