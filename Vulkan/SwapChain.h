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
    operator VkSwapchainKHR& () {
        return _swapChain;
    }
    VkFormat& GetImageFormat() { return _swapChainImageFormat; }
    VkExtent2D& GetExtent() { return _swapChainExtent; }
    std::vector<ImageSet>& GetImages() { return _swapChainImages; }
    std::vector<VkFramebuffer>& GetFrameBuffers() { return _swapChainFramebuffers; }
    RenderPass& GetRenderPass() { return _renderPass; }
    RenderPass& GetDeferredRenderPass() { return _deferredRenderPass; }
    std::vector<VkFramebuffer>& GetDeferredFrameBuffers() { return _deferredFramebuffers; }

    void InitDescriptorSetForGBuffer(Device& device);
    std::vector<DescriptorSet> descriptorSets;
    void create(Device& device);
    void destroy(Device& device);

    static SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device,VkSurfaceKHR surface) {
        SwapChainSupportDetails details;
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

        uint32_t formatCount;
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);

        if (formatCount != 0) {
            details.formats.resize(formatCount);
            vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
        }

        uint32_t presentModeCount;
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);

        if (presentModeCount != 0) {
            details.presentModes.resize(presentModeCount);
            vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());
        }

        return details;
    }

private:
    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
    VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);
    void createImageViews();


    RenderPass _renderPass;
    Device* _device;
    Surface* _surface;
    GLFWwindow* _window;

    //Members

    std::vector<ImageSet> _swapChainImages;
    VkFormat _swapChainImageFormat;
    VkExtent2D _swapChainExtent;
    std::vector<VkFramebuffer> _swapChainFramebuffers;
    VkSwapchainKHR _swapChain;
    
    //Depth
    VkFormat _depthFormat;
    ImageSet _depthImage;
    
    //Deferred
    std::vector<ImageSet> _deferredImages;
    std::vector<VkFramebuffer> _deferredFramebuffers;
    RenderPass _deferredRenderPass;

};

VkFormat findDepthFormat(Device& device);
VkFormat findSupportedFormat(Device& device, const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);