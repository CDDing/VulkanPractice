#include "pch.h"
#include "SwapChain.h"

SwapChain::SwapChain()
{
}

SwapChain::SwapChain(Device& device,Surface& surface) : _device(&device), _surface(&surface)
{
    create(device);        

}


void SwapChain::InitDescriptorSetForGBuffer(Device& device)
{
    for (size_t frame = 0; frame < MAX_FRAMES_IN_FLIGHT; frame++) {
        std::vector<VkDescriptorImageInfo> imageInfos(6);
        for (int i = 0; i < 6; i++) {
            auto& imageInfo = imageInfos[i];
            imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            imageInfo.imageView = _deferredImages[i].imageView;
            imageInfo.sampler = Sampler::Get(SamplerMipMapType::Low);

        }
        VkWriteDescriptorSet descriptorWrite{};
        descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrite.dstSet = descriptorSets[frame];
        descriptorWrite.dstBinding = 0;
        descriptorWrite.dstArrayElement = 0;
        descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorWrite.descriptorCount = imageInfos.size();
        descriptorWrite.pImageInfo = imageInfos.data();

        vkUpdateDescriptorSets(device, 1, &descriptorWrite, 0, nullptr);
    }
}

void SwapChain::create(Device& device)
{
    SwapChainSupportDetails swapChainSupport = SwapChain::querySwapChainSupport(*_device, *_surface);

    VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
    VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
    VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities);

    uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;

    if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {
        imageCount = swapChainSupport.capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = *_surface;
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;

    QueueFamilyIndices indices = findQueueFamilies(*_device, *_surface);
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

    if (vkCreateSwapchainKHR(*_device, &createInfo, nullptr, &_swapChain) != VK_SUCCESS) {
        throw std::runtime_error("failed to create swap chain!");
    }

    vkGetSwapchainImagesKHR(*_device, _swapChain, &imageCount, nullptr);
    _swapChainImages.resize(imageCount);
    std::vector<VkImage> temp(imageCount,0);
    vkGetSwapchainImagesKHR(*_device, _swapChain, &imageCount, temp.data());
    for (int i = 0; i < imageCount;i++) {
        _swapChainImages[i].image = temp[i];
    }


    _swapChainImageFormat = surfaceFormat.format;
    _swapChainExtent = extent;

    createImageViews();


    //Depth
    VkFormat depthFormat = findDepthFormat(device);
    _depthImage.image = Image(device, _swapChainExtent.width, _swapChainExtent.height, 1, depthFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    _depthImage.imageView = ImageView(device, _depthImage.image, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT, 1);

    VkCommandBuffer cmdBuf = beginSingleTimeCommands(device);
    _depthImage.image.transitionLayout(device,cmdBuf, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_IMAGE_ASPECT_DEPTH_BIT);
    endSingleTimeCommands(device, cmdBuf);
    _renderPass = RenderPass(device, _swapChainImageFormat, findDepthFormat(device));
    
    _swapChainFramebuffers.resize(_swapChainImages.size());
    for (size_t i = 0; i < _swapChainImages.size(); i++) {
        std::array<VkImageView, 2> attachments = {
            _swapChainImages[i].imageView,
            _depthImage.imageView
        };
        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = _renderPass;
        framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        framebufferInfo.pAttachments = attachments.data();
        framebufferInfo.width = _swapChainExtent.width;
        framebufferInfo.height = _swapChainExtent.height;
        framebufferInfo.layers = 1;

        if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &_swapChainFramebuffers[i]) != VK_SUCCESS) {
            throw std::runtime_error("failed to create framebuffer!");
        }
    }

    //Imgui
    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = _swapChainImageFormat;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentDescription depthAttachment{};
    depthAttachment.format = findDepthFormat(device);
    depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depthAttachmentRef{};
    depthAttachmentRef.attachment = 1;
    depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkSubpassDescription postsubpass{};
    postsubpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    postsubpass.colorAttachmentCount = 1;
    postsubpass.pColorAttachments = &colorAttachmentRef;
    postsubpass.pDepthStencilAttachment = &depthAttachmentRef;

    VkSubpassDependency postdependency{};
    postdependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    postdependency.dstSubpass = 0;
    postdependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    postdependency.srcAccessMask = 0;
    postdependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    postdependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    std::array<VkAttachmentDescription, 2> attachments = { colorAttachment,depthAttachment };
    VkRenderPassCreateInfo postrenderPassInfo{};
    postrenderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    postrenderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    postrenderPassInfo.pAttachments = attachments.data();
    postrenderPassInfo.subpassCount = 1;
    postrenderPassInfo.pSubpasses = &postsubpass;
    postrenderPassInfo.dependencyCount = 1;
    postrenderPassInfo.pDependencies = &postdependency;

    if (vkCreateRenderPass(device, &postrenderPassInfo, nullptr, &_postRenderPass) != VK_SUCCESS) {
        throw std::runtime_error("failed to create render pass!");
    }

    _postFramebuffers.resize(_swapChainImages.size());
    for (size_t i = 0; i < _swapChainImages.size(); i++) {
        std::array<VkImageView, 2> attachments = {
            _swapChainImages[i].imageView,
            _depthImage.imageView
        };
        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = _postRenderPass;
        framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        framebufferInfo.pAttachments = attachments.data();
        framebufferInfo.width = _swapChainExtent.width;
        framebufferInfo.height = _swapChainExtent.height;
        framebufferInfo.layers = 1;

        if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &_postFramebuffers[i]) != VK_SUCCESS) {
            throw std::runtime_error("failed to create framebuffer!");
        }
    }
    //디퍼드 렌더링
    //1. 이미지 7개 만들기(위치, 노말, 알베도, 깊이, roughness,metalic,ao)
    VkFormat positionFormat = VK_FORMAT_R16G16B16A16_SFLOAT;
    VkFormat normalFormat = VK_FORMAT_R16G16B16A16_SFLOAT;
    VkFormat albedoFormat = VK_FORMAT_R8G8B8A8_UNORM;
    VkFormat roughnessFormat = VK_FORMAT_R8G8B8A8_UNORM;
    VkFormat metalnessFormat = VK_FORMAT_R8G8B8A8_UNORM;
    VkFormat aoFormat = VK_FORMAT_R8G8B8A8_UNORM;
    _deferredImages.resize(7);
    _deferredImages[0] = ImageSet(device, _swapChainExtent.width, _swapChainExtent.height,
        1, positionFormat,
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT
        , VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_IMAGE_ASPECT_COLOR_BIT);
    _deferredImages[1] = ImageSet(device, _swapChainExtent.width, _swapChainExtent.height,
        1, normalFormat,
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT
        , VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,VK_IMAGE_ASPECT_COLOR_BIT);
    _deferredImages[2]= ImageSet(device, _swapChainExtent.width, _swapChainExtent.height,
        1, albedoFormat,
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT
        , VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_IMAGE_ASPECT_COLOR_BIT);
    _deferredImages[3]= ImageSet(device, _swapChainExtent.width, _swapChainExtent.height,
        1, roughnessFormat,
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT
        , VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_IMAGE_ASPECT_COLOR_BIT);
    _deferredImages[4]= ImageSet(device, _swapChainExtent.width, _swapChainExtent.height,
        1, metalnessFormat,
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT
        , VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_IMAGE_ASPECT_COLOR_BIT);
    _deferredImages[5]= ImageSet(device, _swapChainExtent.width, _swapChainExtent.height,
        1, aoFormat,
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT
        , VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_IMAGE_ASPECT_COLOR_BIT);

    //이미지 뷰 만들기
    if (depthFormat >= VK_FORMAT_D16_UNORM_S8_UINT) {
        _deferredImages[6] = ImageSet(device, _swapChainExtent.width, _swapChainExtent.height,
            1, depthFormat,
            VK_IMAGE_TILING_OPTIMAL,
            VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT
            , VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT);
        
    }
    else {
        _deferredImages[6] = ImageSet(device, _swapChainExtent.width, _swapChainExtent.height,
            1, depthFormat,
            VK_IMAGE_TILING_OPTIMAL,
            VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT
            , VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_IMAGE_ASPECT_DEPTH_BIT);

    }

    //렌더 패스 생성
    std::array<VkAttachmentDescription, 7> attachmentDescription = {};
    for (uint32_t i = 0; i < 7; ++i) {
        attachmentDescription[i].samples = VK_SAMPLE_COUNT_1_BIT;
        attachmentDescription[i].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachmentDescription[i].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachmentDescription[i].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachmentDescription[i].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        if (i == 6) {
            attachmentDescription[i].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            attachmentDescription[i].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        }
        else {
            attachmentDescription[i].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            attachmentDescription[i].finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        }
    }

    attachmentDescription[0].format = positionFormat;
    attachmentDescription[1].format = normalFormat;
    attachmentDescription[2].format = albedoFormat;
    attachmentDescription[3].format = roughnessFormat;
    attachmentDescription[4].format = metalnessFormat;
    attachmentDescription[5].format = aoFormat;
    attachmentDescription[6].format = depthFormat;

    std::vector<VkAttachmentReference> colorReferences;
    colorReferences.push_back({ 0,VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL });
    colorReferences.push_back({ 1,VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL });
    colorReferences.push_back({ 2,VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL });
    colorReferences.push_back({ 3,VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL });
    colorReferences.push_back({ 4,VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL });
    colorReferences.push_back({ 5,VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL });
    
    VkAttachmentReference depthReference = {};
    depthReference.attachment = 6;
    depthReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.pColorAttachments = colorReferences.data();
    subpass.colorAttachmentCount = static_cast<uint32_t>(colorReferences.size());
    subpass.pDepthStencilAttachment = &depthReference;

    std::array<VkSubpassDependency, 2> dependencies;
    dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
    dependencies[0].dstSubpass = 0;
    dependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;


    dependencies[1].srcSubpass = 0;
    dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
    dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependencies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dependencies[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    VkRenderPassCreateInfo renderPassInfo = {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.pAttachments = attachmentDescription.data();
    renderPassInfo.attachmentCount = static_cast<uint32_t>(attachmentDescription.size());
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 2;
    renderPassInfo.pDependencies = dependencies.data();

    vkCreateRenderPass(device, &renderPassInfo, nullptr, &_deferredRenderPass);

    _deferredFramebuffers.resize(_swapChainImages.size());
    for (size_t i = 0; i < _swapChainImages.size(); i++) {
        std::array<VkImageView, 7> attachments = {
            _deferredImages[0],
            _deferredImages[1],
            _deferredImages[2],
            _deferredImages[3],
            _deferredImages[4],
            _deferredImages[5],
            _deferredImages[6],
        };
        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = _deferredRenderPass;
        framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        framebufferInfo.pAttachments = attachments.data();
        framebufferInfo.width = _swapChainExtent.width;
        framebufferInfo.height = _swapChainExtent.height;
        framebufferInfo.layers = 1;

        if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &_deferredFramebuffers[i]) != VK_SUCCESS) {
            throw std::runtime_error("failed to create framebuffer!");
        }
    }

}

void SwapChain::destroy(Device& device)
{
    for (auto& image : _deferredImages) {
        image.destroy(device);
    }
    for (auto& framebuffer : _deferredFramebuffers) {
        vkDestroyFramebuffer(device, framebuffer, nullptr);
    }

    
    _depthImage.destroy(device);
    for (size_t i = 0; i < _swapChainFramebuffers.size(); i++) {
        vkDestroyFramebuffer(device, _swapChainFramebuffers[i], nullptr);
    }

    for (auto& image : _swapChainImages) {
        image.imageView.destroy(device);
    }
    vkDestroySwapchainKHR(device, _swapChain, nullptr);

    vkDestroyRenderPass(device, _renderPass, nullptr);
    vkDestroyRenderPass(device, _deferredRenderPass, nullptr);

    vkDestroyRenderPass(device, _postRenderPass, nullptr);
    for (auto& framebuffer : _postFramebuffers) {
        vkDestroyFramebuffer(device, framebuffer, nullptr);
    }
}


VkSurfaceFormatKHR SwapChain::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
    for (const auto& availableFormat : availableFormats) {
        if (availableFormat.format == VK_FORMAT_B8G8R8A8_UNORM && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
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
    for (size_t i = 0; i < _swapChainImages.size(); i++) {
        _swapChainImages[i].imageView = ImageView(*_device, _swapChainImages[i].image, _swapChainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1);
    }
}

VkFormat findDepthFormat(Device& device) {
    return findSupportedFormat(device, { VK_FORMAT_D32_SFLOAT,VK_FORMAT_D32_SFLOAT_S8_UINT,VK_FORMAT_D24_UNORM_S8_UINT }, VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
}

VkFormat findSupportedFormat(Device& device, const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features) {
    for (VkFormat format : candidates) {
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(device, format, &props);

        if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
            return format;
        }
        else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
            return format;
        }
    }

    throw std::runtime_error("failed to find supported format!");
}