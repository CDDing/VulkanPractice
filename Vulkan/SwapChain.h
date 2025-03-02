#pragma once
#include "Image.h"
class DescriptorSet;
class Sampler;
class SwapChain
{
public:
    SwapChain(Device& device, vk::raii::SurfaceKHR& surface);
    operator vk::raii::SwapchainKHR& () {
        return swapChain;
    }
	vk::raii::SwapchainKHR& Get() {
		return swapChain;
	}

    void create(Device& device);
    void destroy(std::shared_ptr<Device> device);

    static SwapChainSupportDetails querySwapChainSupport(vk::raii::PhysicalDevice& device,vk::raii::SurfaceKHR& surface) {
        SwapChainSupportDetails details;
        details.capabilities = device.getSurfaceCapabilitiesKHR(surface);
        details.formats = device.getSurfaceFormatsKHR(surface);
        details.presentModes = device.getSurfacePresentModesKHR(surface);
        return details;
    }



    vk::raii::RenderPass renderPass;
    std::vector<vk::raii::Image> images;
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


    vk::raii::SurfaceKHR* surface;
    GLFWwindow* window;

    
    vk::raii::SwapchainKHR swapChain;
    
    

};

vk::Format findDepthFormat(Device& device);
vk::Format findSupportedFormat(Device& device, const std::vector<vk::Format>& candidates, vk::ImageTiling tiling, vk::FormatFeatureFlags features);