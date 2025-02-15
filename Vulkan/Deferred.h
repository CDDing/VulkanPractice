#pragma once
class Deferred
{
public:
	Deferred() {};
	Deferred(std::shared_ptr<Device> device, SwapChain& swapChain);
    std::vector<DescriptorSet> descriptorSets;
    RenderPass renderPass;
    std::vector<vk::Framebuffer> framebuffers;
    void destroy();
    void updateDescriptorSets();
private:

    void createRenderPass();
    void createImages();
	void createFramebuffers();

    std::vector<ImageSet> images;
    SwapChain* swapChain;
    std::shared_ptr<Device> device;

};

