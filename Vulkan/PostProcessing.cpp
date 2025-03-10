#include "pch.h"
#include "PostProcessing.h"

PostProcessing::PostProcessing(DContext& context, SwapChain& swapChain) : swapChain(&swapChain),
renderPass(nullptr)
{
	createRenderPass(context);
	createFramebuffers(context);
}

void PostProcessing::createRenderPass(DContext& context)
{

    vk::AttachmentDescription colorAttachment{};
    colorAttachment.format = swapChain->imageFormat;
    colorAttachment.samples = vk::SampleCountFlagBits::e1;
    colorAttachment.loadOp = vk::AttachmentLoadOp::eLoad;
    colorAttachment.storeOp = vk::AttachmentStoreOp::eStore;
    colorAttachment.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
    colorAttachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
    colorAttachment.initialLayout = vk::ImageLayout::ePresentSrcKHR;
    colorAttachment.finalLayout = vk::ImageLayout::ePresentSrcKHR;

    vk::AttachmentDescription depthAttachment{};
    depthAttachment.format = findDepthFormat(context);
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
    postdependency.srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eEarlyFragmentTests;
    postdependency.srcAccessMask = (vk::AccessFlagBits)0;
    postdependency.dstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eEarlyFragmentTests;
    postdependency.dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite | vk::AccessFlagBits::eDepthStencilAttachmentWrite;

    std::array<vk::AttachmentDescription, 2> attachments = { colorAttachment,depthAttachment };
    vk::RenderPassCreateInfo postrenderPassInfo{
        {},attachments,{postsubpass},{postdependency}
    };

    renderPass = context.logical.createRenderPass(postrenderPassInfo);
}

void PostProcessing::createFramebuffers(DContext& context)
{
	framebuffers.reserve(swapChain->images.size());
    for (size_t i = 0; i < swapChain->images.size(); i++) {
        std::array<vk::ImageView, 2> attachments = {
            swapChain->imageViews[i],
            swapChain->depthImage.view
        };
        vk::FramebufferCreateInfo framebufferInfo{ };
        framebufferInfo.setRenderPass(renderPass);
        framebufferInfo.setAttachments(attachments);
        framebufferInfo.width = swapChain->extent.width;
        framebufferInfo.height = swapChain->extent.height;
        framebufferInfo.layers = 1;


        framebuffers.emplace_back(context.logical,framebufferInfo,nullptr);
    }
}
