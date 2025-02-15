#include "pch.h"
#include "RenderPass.h"

RenderPass::RenderPass()
{
}

RenderPass::RenderPass(std::shared_ptr<Device> device, vk::ArrayProxyNoTemporaries<vk::AttachmentDescription>  attachments, vk::SubpassDescription subpass, vk::SubpassDependency dependency) : device(device)
{
    vk::RenderPassCreateInfo renderPassInfo{
        {},
        attachments,
    subpass,dependency};
    renderPass = device->logical.createRenderPass(renderPassInfo);
}
