#pragma once
class PostProcessing
{
public:
    PostProcessing() {};
	PostProcessing(std::shared_ptr<Device> device, SwapChain& swapChain);
    void destroy();

    RenderPass renderPass;
    std::vector<vk::Framebuffer> framebuffers;

private:
	void createRenderPass();
	void createFramebuffers();

	SwapChain* swapChain;
    std::shared_ptr<Device> device;
};

