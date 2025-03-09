#pragma once
class Deferred
{
public:
	Deferred(DContext& context, SwapChain& swapChain);
    std::vector<vk::raii::DescriptorSet> descriptorSets;
    std::vector<vk::raii::Framebuffer> framebuffers;
    void updateDescriptorSets(DContext& context);


    vk::raii::RenderPass renderPass;
private:

    void createRenderPass(DContext& context);
    void createImages(DContext& context);
	void createFramebuffers(DContext& context);

    std::vector<DImage> images;
    SwapChain* swapChain;
    std::shared_ptr<Device> device;

};

