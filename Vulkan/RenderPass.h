#pragma once
class RenderPass
{
public:

	RenderPass();
	RenderPass(Device& device, vk::Format swapChainImageFormat, vk::Format DepthFormat);
	operator vk::RenderPass& () {
		return _renderPass;
	}
	operator VkRenderPass() {
		return static_cast<VkRenderPass>(_renderPass);
	}
	void operator=(vk::RenderPass renderPass) {
		_renderPass = renderPass;
	}
	vk::RenderPass* operator&() {
		return &_renderPass;
	}

private:
	vk::RenderPass _renderPass;
};

