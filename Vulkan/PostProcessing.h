#pragma once
class PostProcessing
{
public:
    PostProcessing(Device& device, SwapChain& swapChain);
    
    vk::raii::RenderPass renderPass;
    std::vector<vk::raii::Framebuffer> framebuffers;

private:
	void createRenderPass(Device& device);
	void createFramebuffers(Device& device);

	SwapChain* swapChain;
};

