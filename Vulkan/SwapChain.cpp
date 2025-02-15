#include "pch.h"
#include "SwapChain.h"

SwapChain::SwapChain()
{
}

SwapChain::SwapChain(std::shared_ptr<Device> device,Surface& surface) : _device(device), _surface(&surface)
{
    create(device);
}


void SwapChain::InitDescriptorSetForGBuffer(std::shared_ptr<Device> device)
{
    for (size_t frame = 0; frame < MAX_FRAMES_IN_FLIGHT; frame++) {
        std::vector<vk::DescriptorImageInfo> imageInfos(6);
        for (int i = 0; i < 6; i++) {
            auto& imageInfo = imageInfos[i];
            imageInfo.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
            imageInfo.imageView = _deferredImages[i].imageView;
            imageInfo.sampler = Sampler::Get(SamplerMipMapType::Low);

        }
        vk::WriteDescriptorSet descriptorWrite{};
        descriptorWrite.dstSet = descriptorSets[frame];
        descriptorWrite.dstBinding = 0;
        descriptorWrite.dstArrayElement = 0;
        descriptorWrite.descriptorType = vk::DescriptorType::eCombinedImageSampler;
        descriptorWrite.descriptorCount = imageInfos.size();
        descriptorWrite.pImageInfo = imageInfos.data();
        
        device->logical.updateDescriptorSets(descriptorWrite,nullptr);
    }
}

void SwapChain::create(std::shared_ptr<Device> device)
{
    SwapChainSupportDetails swapChainSupport = SwapChain::querySwapChainSupport(*_device, *_surface);

    vk::SurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
    vk::PresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
    vk::Extent2D extent = chooseSwapExtent(swapChainSupport.capabilities);

    uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;

    if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {
        imageCount = swapChainSupport.capabilities.maxImageCount;
    }

    vk::SwapchainCreateInfoKHR createInfo{};
    createInfo.surface = *_surface;
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eStorage| vk::ImageUsageFlagBits::eTransferDst;

    QueueFamilyIndices indices = findQueueFamilies(*_device, *_surface);
    uint32_t queueFamilyIndices[] = { indices.graphicsFamily.value(), indices.presentFamily.value() };

    if (indices.graphicsFamily != indices.presentFamily) {
        createInfo.imageSharingMode = vk::SharingMode::eConcurrent;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    }
    else {
        createInfo.imageSharingMode = vk::SharingMode::eExclusive;
        createInfo.queueFamilyIndexCount = 0; // Optional
        createInfo.pQueueFamilyIndices = nullptr; // Optional
    }

    createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
    createInfo.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;
    createInfo.oldSwapchain = VK_NULL_HANDLE;

    _swapChain = _device->logical.createSwapchainKHR(createInfo);
    
    std::vector<vk::Image> temp = _device->logical.getSwapchainImagesKHR(_swapChain);
    _swapChainImages.resize(imageCount);
    for (int i = 0; i < imageCount;i++) {
        _swapChainImages[i].image = temp[i];
    }


    _swapChainImageFormat = surfaceFormat.format;
    _swapChainExtent = extent;

    createImageViews();


    //Depth
    vk::Format depthFormat = findDepthFormat(*device);
    _depthImage.image = Image(device, _swapChainExtent.width, _swapChainExtent.height, 1, depthFormat, vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eDepthStencilAttachment, vk::MemoryPropertyFlagBits::eDeviceLocal);
    _depthImage.imageView = ImageView(device, _depthImage.image, depthFormat, vk::ImageAspectFlagBits::eDepth, 1);

    vk::CommandBuffer cmdBuf = beginSingleTimeCommands(device);
    _depthImage.image.transitionLayout(device,cmdBuf, vk::ImageLayout::eDepthStencilAttachmentOptimal, vk::ImageAspectFlagBits::eDepth);
    endSingleTimeCommands(device, cmdBuf);
    _renderPass = RenderPass(*device, _swapChainImageFormat, findDepthFormat(*device));
    
    _swapChainFramebuffers.resize(_swapChainImages.size());
    for (size_t i = 0; i < _swapChainImages.size(); i++) {
        std::array<vk::ImageView, 2> attachments = {
            _swapChainImages[i].imageView,
            _depthImage.imageView
        };
        vk::FramebufferCreateInfo framebufferInfo{};
        framebufferInfo.renderPass = _renderPass.operator vk::RenderPass &();
        framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        framebufferInfo.pAttachments = attachments.data();
        framebufferInfo.width = _swapChainExtent.width;
        framebufferInfo.height = _swapChainExtent.height;
        framebufferInfo.layers = 1;

        _swapChainFramebuffers[i] = device->logical.createFramebuffer(framebufferInfo);
    }

    //Imgui
    vk::AttachmentDescription colorAttachment{};
    colorAttachment.format = _swapChainImageFormat;
    colorAttachment.samples = vk::SampleCountFlagBits::e1;
    colorAttachment.loadOp = vk::AttachmentLoadOp::eLoad;
    colorAttachment.storeOp = vk::AttachmentStoreOp::eStore;
    colorAttachment.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
    colorAttachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
    colorAttachment.initialLayout = vk::ImageLayout::ePresentSrcKHR;
    colorAttachment.finalLayout = vk::ImageLayout::ePresentSrcKHR;

    vk::AttachmentDescription depthAttachment{};
    depthAttachment.format = findDepthFormat(*device);
    depthAttachment.samples = vk::SampleCountFlagBits::e1;
    depthAttachment.loadOp = vk::AttachmentLoadOp::eClear;
    depthAttachment.storeOp = vk::AttachmentStoreOp::eDontCare;
    depthAttachment.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
    depthAttachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
    depthAttachment.initialLayout = vk::ImageLayout::eUndefined;
    depthAttachment.finalLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;

    vk::AttachmentReference colorAttachmentRef{  };
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = vk::ImageLayout::eColorAttachmentOptimal;

    vk::AttachmentReference depthAttachmentRef{};
    depthAttachmentRef.attachment = 1;
    depthAttachmentRef.layout = vk::ImageLayout::eDepthStencilAttachmentOptimal;

    vk::SubpassDescription postsubpass{};
    postsubpass.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
    postsubpass.colorAttachmentCount = 1;
    postsubpass.pColorAttachments = &colorAttachmentRef;
    postsubpass.pDepthStencilAttachment = &depthAttachmentRef;

    vk::SubpassDependency postdependency{};
    postdependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    postdependency.dstSubpass = 0;
    postdependency.srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput| vk::PipelineStageFlagBits::eEarlyFragmentTests;
    postdependency.srcAccessMask = (vk::AccessFlagBits)0;
    postdependency.dstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eEarlyFragmentTests;
    postdependency.dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite| vk::AccessFlagBits::eDepthStencilAttachmentWrite;

    std::array<vk::AttachmentDescription, 2> attachments = { colorAttachment,depthAttachment };
    vk::RenderPassCreateInfo postrenderPassInfo{
        {},attachments,{postsubpass},{postdependency}
    };

    _postRenderPass = device->logical.createRenderPass(postrenderPassInfo);

    _postFramebuffers.resize(_swapChainImages.size());
    for (size_t i = 0; i < _swapChainImages.size(); i++) {
        std::array<vk::ImageView, 2> attachments = {
            _swapChainImages[i].imageView,
            _depthImage.imageView
        };
        vk::FramebufferCreateInfo framebufferInfo{};
        framebufferInfo.renderPass = _postRenderPass.operator vk::RenderPass &();
        framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        framebufferInfo.pAttachments = attachments.data();
        framebufferInfo.width = _swapChainExtent.width;
        framebufferInfo.height = _swapChainExtent.height;
        framebufferInfo.layers = 1;


        _postFramebuffers[i] = device->logical.createFramebuffer(framebufferInfo);
    }
    //디퍼드 렌더링
    //1. 이미지 7개 만들기(위치, 노말, 알베도, 깊이, roughness,metalic,ao)
    vk::Format positionFormat = vk::Format::eR16G16B16A16Sfloat;
    vk::Format normalFormat = vk::Format::eR16G16B16A16Sfloat;
    vk::Format albedoFormat = vk::Format::eR8G8B8A8Unorm;
    vk::Format roughnessFormat = vk::Format::eR8G8B8A8Unorm;
    vk::Format metalnessFormat = vk::Format::eR8G8B8A8Unorm;
    vk::Format aoFormat = vk::Format::eR8G8B8A8Unorm;
    _deferredImages.resize(7);
	_deferredImages[0].image = Image(device, 
        _swapChainExtent.width, _swapChainExtent.height, 1, positionFormat, 
        vk::ImageTiling::eOptimal, 
        vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled, 
        vk::MemoryPropertyFlagBits::eDeviceLocal);
	_deferredImages[0].imageView = ImageView(device, 
        _deferredImages[0].image, 
        positionFormat, 
        vk::ImageAspectFlagBits::eColor, 1);
    
	_deferredImages[1].image = Image(device, 
        _swapChainExtent.width, _swapChainExtent.height, 
        1, normalFormat, vk::ImageTiling::eOptimal, 
        vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled, 
        vk::MemoryPropertyFlagBits::eDeviceLocal);
	_deferredImages[1].imageView = ImageView(device,
		_deferredImages[1].image,
		normalFormat,
		vk::ImageAspectFlagBits::eColor, 1);

	_deferredImages[2].image = Image(device
        , _swapChainExtent.width, _swapChainExtent.height,
		1, albedoFormat,
		vk::ImageTiling::eOptimal,
		vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled
		, vk::MemoryPropertyFlagBits::eDeviceLocal);
	_deferredImages[2].imageView = ImageView(device, 
        _deferredImages[2].image, 
        albedoFormat, 
        vk::ImageAspectFlagBits::eColor, 1);

	_deferredImages[3].image = Image(device,
		_swapChainExtent.width, _swapChainExtent.height,
		1, roughnessFormat,
		vk::ImageTiling::eOptimal,
		vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled
		, vk::MemoryPropertyFlagBits::eDeviceLocal);
	_deferredImages[3].imageView = ImageView(device,
		_deferredImages[3].image,
		roughnessFormat,
		vk::ImageAspectFlagBits::eColor, 1);

	_deferredImages[4].image = Image(device,
		_swapChainExtent.width, _swapChainExtent.height,
		1, metalnessFormat,
		vk::ImageTiling::eOptimal,
		vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled
		, vk::MemoryPropertyFlagBits::eDeviceLocal);
	_deferredImages[4].imageView = ImageView(device,
		_deferredImages[4].image,
		metalnessFormat,
		vk::ImageAspectFlagBits::eColor, 1);

	_deferredImages[5].image = Image(device,
		_swapChainExtent.width, _swapChainExtent.height,
		1, aoFormat,
		vk::ImageTiling::eOptimal,
		vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled
		, vk::MemoryPropertyFlagBits::eDeviceLocal);
	_deferredImages[5].imageView = ImageView(device,
		_deferredImages[5].image,
		aoFormat,
		vk::ImageAspectFlagBits::eColor, 1);

    //이미지 뷰 만들기
    if (depthFormat >= vk::Format::eD16UnormS8Uint) {
		_deferredImages[6].image = Image(device, _swapChainExtent.width, _swapChainExtent.height, 1, depthFormat, vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eDepthStencilAttachment, vk::MemoryPropertyFlagBits::eDeviceLocal);
		_deferredImages[6].imageView = ImageView(device, _deferredImages[6].image, depthFormat, vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil, 1);
    }
    else {
		_deferredImages[6].image = Image(device, _swapChainExtent.width, _swapChainExtent.height, 1, depthFormat, vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eDepthStencilAttachment, vk::MemoryPropertyFlagBits::eDeviceLocal);
		_deferredImages[6].imageView = ImageView(device, _deferredImages[6].image, depthFormat, vk::ImageAspectFlagBits::eDepth, 1);
    }

    //렌더 패스 생성
    std::array<vk::AttachmentDescription, 7> attachmentDescription = {};
    for (uint32_t i = 0; i < 7; ++i) {
        attachmentDescription[i].samples = vk::SampleCountFlagBits::e1;
        attachmentDescription[i].loadOp = vk::AttachmentLoadOp::eClear;
        attachmentDescription[i].storeOp = vk::AttachmentStoreOp::eStore;
        attachmentDescription[i].stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
        attachmentDescription[i].stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
        if (i == 6) {
            attachmentDescription[i].initialLayout = vk::ImageLayout::eUndefined;
            attachmentDescription[i].finalLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
        }
        else {
            attachmentDescription[i].initialLayout = vk::ImageLayout::eUndefined;
            attachmentDescription[i].finalLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
        }
    }

    attachmentDescription[0].format = positionFormat;
    attachmentDescription[1].format = normalFormat;
    attachmentDescription[2].format = albedoFormat;
    attachmentDescription[3].format = roughnessFormat;
    attachmentDescription[4].format = metalnessFormat;
    attachmentDescription[5].format = aoFormat;
    attachmentDescription[6].format = depthFormat;

    std::vector<vk::AttachmentReference> colorReferences;
    colorReferences.push_back({ 0,vk::ImageLayout::eColorAttachmentOptimal});
    colorReferences.push_back({ 1,vk::ImageLayout::eColorAttachmentOptimal });
    colorReferences.push_back({ 2,vk::ImageLayout::eColorAttachmentOptimal });
    colorReferences.push_back({ 3,vk::ImageLayout::eColorAttachmentOptimal });
    colorReferences.push_back({ 4,vk::ImageLayout::eColorAttachmentOptimal });
    colorReferences.push_back({ 5,vk::ImageLayout::eColorAttachmentOptimal });
    
    vk::AttachmentReference depthReference = {};
    depthReference.attachment = 6;
    depthReference.layout = vk::ImageLayout::eDepthStencilAttachmentOptimal;

    vk::SubpassDescription subpass = {};
    subpass.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
    subpass.pColorAttachments = colorReferences.data();
    subpass.colorAttachmentCount = static_cast<uint32_t>(colorReferences.size());
    subpass.pDepthStencilAttachment = &depthReference;

    std::array<vk::SubpassDependency, 2> dependencies;
    dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
    dependencies[0].dstSubpass = 0;
    dependencies[0].srcStageMask = vk::PipelineStageFlagBits::eBottomOfPipe;
    dependencies[0].dstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
    dependencies[0].srcAccessMask = vk::AccessFlagBits::eMemoryRead;
    dependencies[0].dstAccessMask = vk::AccessFlagBits::eColorAttachmentRead| vk::AccessFlagBits::eColorAttachmentWrite;
    dependencies[0].dependencyFlags = vk::DependencyFlagBits::eByRegion;


    dependencies[1].srcSubpass = 0;
    dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
    dependencies[1].srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
    dependencies[1].dstStageMask = vk::PipelineStageFlagBits::eBottomOfPipe;
    dependencies[1].srcAccessMask = vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite;
    dependencies[1].dstAccessMask = vk::AccessFlagBits::eMemoryRead;
    dependencies[1].dependencyFlags = vk::DependencyFlagBits::eByRegion;

    vk::RenderPassCreateInfo renderPassInfo = {};
    renderPassInfo.pAttachments = attachmentDescription.data();
    renderPassInfo.attachmentCount = static_cast<uint32_t>(attachmentDescription.size());
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 2;
    renderPassInfo.pDependencies = dependencies.data();

    _deferredRenderPass = device->logical.createRenderPass(renderPassInfo);
    
    _deferredFramebuffers.resize(_swapChainImages.size());
    for (size_t i = 0; i < _swapChainImages.size(); i++) {
        std::array<vk::ImageView, 7> attachments = {
            _deferredImages[0].imageView,
            _deferredImages[1].imageView,
            _deferredImages[2].imageView,
            _deferredImages[3].imageView,
            _deferredImages[4].imageView,
            _deferredImages[5].imageView,
            _deferredImages[6].imageView,
        };
        vk::FramebufferCreateInfo framebufferInfo{};
        framebufferInfo.renderPass = _deferredRenderPass.operator vk::RenderPass &();
        framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        framebufferInfo.pAttachments = attachments.data();
        framebufferInfo.width = _swapChainExtent.width;
        framebufferInfo.height = _swapChainExtent.height;
        framebufferInfo.layers = 1;

        _deferredFramebuffers[i] = device->logical.createFramebuffer(framebufferInfo);
    }

}

void SwapChain::destroy(std::shared_ptr<Device> device)
{
    for (auto& image : _deferredImages) {
        image.image.destroy(device);
		image.imageView.destroy(device);
    }
    for (auto& framebuffer : _deferredFramebuffers) {
        device->logical.destroyFramebuffer(framebuffer);
    }

    
    _depthImage.image.destroy(device);
    _depthImage.imageView.destroy(device);
    for (size_t i = 0; i < _swapChainFramebuffers.size(); i++) {
        device->logical.destroyFramebuffer(_swapChainFramebuffers[i]);
    }

    for (auto& image : _swapChainImages) {
        image.imageView.destroy(device);
    }
    device->logical.destroySwapchainKHR(_swapChain);

    device->logical.destroyRenderPass(_renderPass);
    device->logical.destroyRenderPass(_deferredRenderPass);
    device->logical.destroyRenderPass(_postRenderPass);
    for (auto& framebuffer : _postFramebuffers) {
        device->logical.destroyFramebuffer(framebuffer);
    }
}


vk::SurfaceFormatKHR SwapChain::chooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& availableFormats) {
    for (const auto& availableFormat : availableFormats) {
        if (availableFormat.format == vk::Format::eB8G8R8A8Unorm&& availableFormat.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear) {
            return availableFormat;
        }
    }
    return availableFormats[0];
}

vk::PresentModeKHR SwapChain::chooseSwapPresentMode(const std::vector<vk::PresentModeKHR>& availablePresentModes) {
    for (const auto& availablePresentMode : availablePresentModes) {
        if (availablePresentMode == vk::PresentModeKHR::eMailbox) {
            return availablePresentMode;
        }
    }
    return vk::PresentModeKHR::eFifo;
}

vk::Extent2D SwapChain::chooseSwapExtent(const vk::SurfaceCapabilitiesKHR& capabilities) {
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
        _swapChainImages[i].imageView = ImageView(_device, _swapChainImages[i].image, _swapChainImageFormat, vk::ImageAspectFlagBits::eColor, 1);
    }
}

vk::Format findDepthFormat(Device& device) {
    return findSupportedFormat(device, { vk::Format::eD32Sfloat,vk::Format::eD32SfloatS8Uint,vk::Format::eD24UnormS8Uint}, vk::ImageTiling::eOptimal, vk::FormatFeatureFlagBits::eDepthStencilAttachment);
}

vk::Format findSupportedFormat(Device& device, const std::vector<vk::Format>& candidates, vk::ImageTiling tiling, vk::FormatFeatureFlags features) {
    for (vk::Format format : candidates) {
        vk::FormatProperties props = device.physical.getFormatProperties(format);

        if (tiling == vk::ImageTiling::eLinear&& (props.linearTilingFeatures & features) == features) {
            return format;
        }
        else if (tiling == vk::ImageTiling::eOptimal&& (props.optimalTilingFeatures & features) == features) {
            return format;
        }
    }

    throw std::runtime_error("failed to find supported format!");
}