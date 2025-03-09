#pragma once
#include "Image.h"
class DescriptorSet;
class Sampler;
class SwapChain
{
public:
    SwapChain(DContext& context);
    operator vk::raii::SwapchainKHR& () {
        return swapChain;
    }
	vk::raii::SwapchainKHR& Get() {
		return swapChain;
	}

    void create();
    void destroy();

    static SwapChainSupportDetails querySwapChainSupport(vk::raii::PhysicalDevice& device,vk::raii::SurfaceKHR& surface) {
        SwapChainSupportDetails details;
        details.capabilities = device.getSurfaceCapabilitiesKHR(surface);
        details.formats = device.getSurfaceFormatsKHR(surface);
        details.presentModes = device.getSurfacePresentModesKHR(surface);
        return details;
    }



    vk::raii::RenderPass renderPass;
    std::vector<vk::Image> images;
	std::vector<vk::raii::ImageView> imageViews;
    vk::Format imageFormat;
    vk::Extent2D extent;
    std::vector<vk::raii::Framebuffer> framebuffers;
    vk::Format depthFormat;
    DImage depthImage;

private:
    vk::SurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& availableFormats);
    vk::PresentModeKHR chooseSwapPresentMode(const std::vector<vk::PresentModeKHR>& availablePresentModes);
    vk::Extent2D chooseSwapExtent(const vk::SurfaceCapabilitiesKHR& capabilities);


    DContext* context;

    
    vk::raii::SwapchainKHR swapChain;
    
    

};

vk::Format findDepthFormat(DContext& context);
vk::Format findSupportedFormat(DContext& context, const std::vector<vk::Format>& candidates, vk::ImageTiling tiling, vk::FormatFeatureFlags features);