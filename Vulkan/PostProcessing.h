#pragma once
class PostProcessing
{
public:
    PostProcessing(DContext& context, SwapChain& swapChain);
    
    vk::raii::RenderPass renderPass;
    std::vector<vk::raii::Framebuffer> framebuffers;

private:
	void createRenderPass(DContext& context);
	void createFramebuffers(DContext& context);

	SwapChain* swapChain;
};

