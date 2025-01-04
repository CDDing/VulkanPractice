#include "pch.h"
#include "SwapChain.h"

SwapChain::SwapChain()
{
}

SwapChain::SwapChain(Device& device,Surface& surface) : _device(&device), _surface(&surface)
{
    create(device);        

}


void SwapChain::create(Device& device)
{
    SwapChainSupportDetails swapChainSupport = SwapChain::querySwapChainSupport(_device->GetPhysical(), _surface->Get());

    VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
    VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
    VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities);

    uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;

    if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {
        imageCount = swapChainSupport.capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = _surface->Get();
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    QueueFamilyIndices indices = findQueueFamilies(_device->GetPhysical(), _surface->Get());
    uint32_t queueFamilyIndices[] = { indices.graphicsFamily.value(), indices.presentFamily.value() };

    if (indices.graphicsFamily != indices.presentFamily) {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    }
    else {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        createInfo.queueFamilyIndexCount = 0; // Optional
        createInfo.pQueueFamilyIndices = nullptr; // Optional
    }

    createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;
    createInfo.oldSwapchain = VK_NULL_HANDLE;

    if (vkCreateSwapchainKHR(_device->Get(), &createInfo, nullptr, &_swapChain) != VK_SUCCESS) {
        throw std::runtime_error("failed to create swap chain!");
    }

    vkGetSwapchainImagesKHR(_device->Get(), _swapChain, &imageCount, nullptr);
    _swapChainImages.resize(imageCount);
    std::vector<VkImage> temp(imageCount,0);
    vkGetSwapchainImagesKHR(_device->Get(), _swapChain, &imageCount, temp.data());
    for (int i = 0; i < imageCount;i++) {
        _swapChainImages[i].Get() = temp[i];
    }


    _swapChainImageFormat = surfaceFormat.format;
    _swapChainExtent = extent;

    createImageViews();


    //Depth
    VkFormat depthFormat = findDepthFormat(device);
    _depthImage = Image(device, _swapChainExtent.width, _swapChainExtent.height, 1, depthFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    _depthImageView = ImageView(device, _depthImage.Get(), depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT, 1);

    transitionImageLayout(device, _depthImage.Get(), depthFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_IMAGE_ASPECT_DEPTH_BIT, 1);

    _renderPass = RenderPass(device, _swapChainImageFormat, findDepthFormat(device));
    
    _swapChainFramebuffers.resize(_swapChainImageViews.size());
    for (size_t i = 0; i < _swapChainImageViews.size(); i++) {
        std::array<VkImageView, 2> attachments = {
            _swapChainImageViews[i].Get(),
            _depthImageView.Get()
        };
        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = _renderPass.Get();
        framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        framebufferInfo.pAttachments = attachments.data();
        framebufferInfo.width = _swapChainExtent.width;
        framebufferInfo.height = _swapChainExtent.height;
        framebufferInfo.layers = 1;

        if (vkCreateFramebuffer(device.Get(), &framebufferInfo, nullptr, &_swapChainFramebuffers[i]) != VK_SUCCESS) {
            throw std::runtime_error("failed to create framebuffer!");
        }
    }


}

void SwapChain::destroy(Device& device)
{
    vkDestroyImageView(device.Get(), _depthImageView.Get(), nullptr);
    vkDestroyImage(device.Get(), _depthImage.Get(), nullptr);
    vkFreeMemory(device.Get(), _depthImage.GetMemory(), nullptr);
    for (size_t i = 0; i < _swapChainFramebuffers.size(); i++) {
        vkDestroyFramebuffer(device.Get(), _swapChainFramebuffers[i], nullptr);
    }

    for (size_t i = 0; i < _swapChainImageViews.size(); i++) {
        vkDestroyImageView(device.Get(), _swapChainImageViews[i].Get(), nullptr);
    }
    vkDestroySwapchainKHR(device.Get(), _swapChain, nullptr);
}


VkSurfaceFormatKHR SwapChain::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
    for (const auto& availableFormat : availableFormats) {
        if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return availableFormat;
        }
    }
    return availableFormats[0];
}

VkPresentModeKHR SwapChain::chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) {
    for (const auto& availablePresentMode : availablePresentModes) {
        if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
            return availablePresentMode;
        }
    }
    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D SwapChain::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) {
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
        return capabilities.currentExtent;
    }
    else {
        int width, height;
        glfwGetFramebufferSize(_window, &width, &height);

        VkExtent2D actualExtent = {
            static_cast<uint32_t>(width),
            static_cast<uint32_t>(height)
        };

        actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
        actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

        return actualExtent;
    }
}

void SwapChain::createImageViews()
{
    _swapChainImageViews.resize(_swapChainImages.size());
    for (size_t i = 0; i < _swapChainImages.size(); i++) {
        _swapChainImageViews[i] = ImageView(*_device, _swapChainImages[i].Get(), _swapChainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1);
    }
}

VkFormat findDepthFormat(Device& device) {
    return findSupportedFormat(device, { VK_FORMAT_D32_SFLOAT,VK_FORMAT_D32_SFLOAT_S8_UINT,VK_FORMAT_D24_UNORM_S8_UINT }, VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
}

VkFormat findSupportedFormat(Device& device, const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features) {
    for (VkFormat format : candidates) {
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(device.GetPhysical(), format, &props);

        if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
            return format;
        }
        else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
            return format;
        }
    }

    throw std::runtime_error("failed to find supported format!");
}