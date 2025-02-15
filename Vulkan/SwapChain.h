#pragma once
#include "Image.h"
#include "ImageView.h"
class DescriptorSet;
class Sampler;
class SwapChain
{
public:
    SwapChain();
    SwapChain(std::shared_ptr<Device> device, Surface& surface);
    operator vk::SwapchainKHR& () {
        return swapChain;
    }

    void create(std::shared_ptr<Device> device);
    void destroy(std::shared_ptr<Device> device);

    static SwapChainSupportDetails querySwapChainSupport(vk::PhysicalDevice device,vk::SurfaceKHR surface) {
        SwapChainSupportDetails details;
        details.capabilities = device.getSurfaceCapabilitiesKHR(surface);
        details.formats = device.getSurfaceFormatsKHR(surface);
        details.presentModes = device.getSurfacePresentModesKHR(surface);
        return details;
    }



    RenderPass renderPass;
    std::vector<ImageSet> images;
    vk::Format imageFormat;
    vk::Extent2D extent;
    std::vector<vk::Framebuffer> framebuffers;
    vk::Format depthFormat;
    ImageSet depthImage;

private:
    vk::SurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& availableFormats);
    vk::PresentModeKHR chooseSwapPresentMode(const std::vector<vk::PresentModeKHR>& availablePresentModes);
    vk::Extent2D chooseSwapExtent(const vk::SurfaceCapabilitiesKHR& capabilities);
    void createImageViews();


    std::shared_ptr<Device> device;
    Surface* surface;
    GLFWwindow* window;

    
    vk::SwapchainKHR swapChain;
    
    

};

vk::Format findDepthFormat(Device& device);
vk::Format findSupportedFormat(Device& device, const std::vector<vk::Format>& candidates, vk::ImageTiling tiling, vk::FormatFeatureFlags features);