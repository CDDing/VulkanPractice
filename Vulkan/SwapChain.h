#pragma once
class Image;
class ImageView;
class SwapChain
{
public:
    SwapChain();
    SwapChain(Device* device, VkSurfaceKHR* surface, GLFWwindow* window);
    VkSwapchainKHR& Get() { return _swapChain; }
    VkFormat& GetImageFormat() { return _swapChainImageFormat; }
    VkExtent2D& GetExtent() { return _swapChainExtent; }
    std::vector<Image>& GetImages() { return _swapChainImages; }
    std::vector<ImageView>& GetImageViews() { return _swapChainImageViews; }
    std::vector<VkFramebuffer>& GetFrameBuffers() { return _swapChainFramebuffers; }
    void recreate();
    void create();
    void cleanup();
    void createImageViews();

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


    //For Recreate, TODO : ÃßÈÄ surface¶û window ÅëÇÕ.

    Device* _device;
    VkSurfaceKHR* _surface;
    GLFWwindow* _window;

    //Members

    std::vector<ImageView> _swapChainImageViews;
    std::vector<Image> _swapChainImages;
    VkFormat _swapChainImageFormat;
    VkExtent2D _swapChainExtent;
    std::vector<VkFramebuffer> _swapChainFramebuffers;
    VkSwapchainKHR _swapChain;
};

