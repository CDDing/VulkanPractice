#pragma once
class Deferred
{
public:
	Deferred(Device& device, SwapChain& swapChain);
    std::vector<vk::raii::DescriptorSet> descriptorSets;
    std::vector<vk::raii::Framebuffer> framebuffers;
    void updateDescriptorSets(Device& device);


    vk::raii::RenderPass renderPass;
private:

    void createRenderPass(Device& device);
    void createImages(Device& device);
	void createFramebuffers(Device& device);

    std::vector<DImage> images;
    SwapChain* swapChain;
    std::shared_ptr<Device> device;

};

