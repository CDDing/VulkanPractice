#pragma once
#include "Image.h"
#include "ImageView.h"
#include "ImageSet.h"
class DescriptorSet;
class Sampler;
class SwapChain
{
public:
    SwapChain();
    SwapChain(Device& device, Surface& surface);
    operator vk::SwapchainKHR& () {
        return _swapChain;
    }
    vk::Format& GetImageFormat() { return _swapChainImageFormat; }
    vk::Extent2D& GetExtent() { return _swapChainExtent; }
    std::vector<ImageSet>& GetImages() { return _swapChainImages; }
    std::vector<vk::Framebuffer>& GetFrameBuffers() { return _swapChainFramebuffers; }
    RenderPass& GetRenderPass() { return _renderPass; }
    RenderPass& GetPostRenderPass() { return _postRenderPass; }
    RenderPass& GetDeferredRenderPass() { return _deferredRenderPass; }
    std::vector<vk::Framebuffer>& GetDeferredFrameBuffers() { return _deferredFramebuffers; }
    std::vector<vk::Framebuffer>& GetPostFrameBuffers() { return _postFramebuffers; }

    void InitDescriptorSetForGBuffer(Device& device);
    std::vector<DescriptorSet> descriptorSets;
    void create(Device& device);
    void destroy(Device& device);

    static SwapChainSupportDetails querySwapChainSupport(vk::PhysicalDevice device,vk::SurfaceKHR surface) {
        SwapChainSupportDetails details;
        details.capabilities = device.getSurfaceCapabilitiesKHR(surface);
        details.formats = device.getSurfaceFormatsKHR(surface);
        details.presentModes = device.getSurfacePresentModesKHR(surface);
        return details;
    }

private:
    vk::SurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& availableFormats);
    vk::PresentModeKHR chooseSwapPresentMode(const std::vector<vk::PresentModeKHR>& availablePresentModes);
    vk::Extent2D chooseSwapExtent(const vk::SurfaceCapabilitiesKHR& capabilities);
    void createImageViews();


    RenderPass _renderPass;
    RenderPass _postRenderPass;
    Device* _device;
    Surface* _surface;
    GLFWwindow* _window;

    //Members

    std::vector<ImageSet> _swapChainImages;
    vk::Format _swapChainImageFormat;
    vk::Extent2D _swapChainExtent;
    std::vector<vk::Framebuffer> _swapChainFramebuffers;
    std::vector<vk::Framebuffer> _postFramebuffers;
    vk::SwapchainKHR _swapChain;
    
    //Depth
    vk::Format _depthFormat;
    ImageSet _depthImage;
    
    //Deferred
    std::vector<ImageSet> _deferredImages;
    std::vector<vk::Framebuffer> _deferredFramebuffers;
    RenderPass _deferredRenderPass;

};

vk::Format findDepthFormat(Device& device);
vk::Format findSupportedFormat(Device& device, const std::vector<vk::Format>& candidates, vk::ImageTiling tiling, vk::FormatFeatureFlags features);