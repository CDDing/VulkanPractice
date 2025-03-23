#include "pch.h"
#include "Deferred.h"

Deferred::Deferred(DContext& context, SwapChain& swapChain) : swapChain(&swapChain),
 renderPass(nullptr)
{
	createImages(context);
	createRenderPass(context);
	createFramebuffers(context);


	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		{
			vk::DescriptorSetAllocateInfo allocInfo{ DescriptorPool::Pool ,*DescriptorSetLayout::Get(DescriptorType::GBuffer) };
			descriptorSets.push_back(std::move(vk::raii::DescriptorSets(context.logical, allocInfo).front()));
		}
	}
	updateDescriptorSets(context);
}

void Deferred::createRenderPass(DContext& context)
{
	//렌더 패스 생성
	std::array<vk::AttachmentDescription, 5> attachmentDescription = {};
	for (uint32_t i = 0; i < 5; ++i) {
		attachmentDescription[i].samples = vk::SampleCountFlagBits::e1;
		attachmentDescription[i].loadOp = vk::AttachmentLoadOp::eClear;
		attachmentDescription[i].storeOp = vk::AttachmentStoreOp::eStore;
		attachmentDescription[i].stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
		attachmentDescription[i].stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
		if (i == 4) {
			attachmentDescription[i].initialLayout = vk::ImageLayout::eUndefined;
			attachmentDescription[i].finalLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
		}
		else {
			attachmentDescription[i].initialLayout = vk::ImageLayout::eUndefined;
			attachmentDescription[i].finalLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
		}
	}

	attachmentDescription[0].format = images[0].format;
	attachmentDescription[1].format = images[1].format;
	attachmentDescription[2].format = images[2].format;
	attachmentDescription[3].format = images[3].format;
	attachmentDescription[4].format = images[4].format;

	std::vector<vk::AttachmentReference> colorReferences;
	colorReferences.push_back({ 0,vk::ImageLayout::eColorAttachmentOptimal });
	colorReferences.push_back({ 1,vk::ImageLayout::eColorAttachmentOptimal });
	colorReferences.push_back({ 2,vk::ImageLayout::eColorAttachmentOptimal });
	colorReferences.push_back({ 3,vk::ImageLayout::eColorAttachmentOptimal });

	vk::AttachmentReference depthReference = {};
	depthReference.attachment = 4;
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
	dependencies[0].dstAccessMask = vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite;
	dependencies[0].dependencyFlags = vk::DependencyFlagBits::eByRegion;


	dependencies[1].srcSubpass = 0;
	dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
	dependencies[1].srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
	dependencies[1].dstStageMask = vk::PipelineStageFlagBits::eBottomOfPipe;
	dependencies[1].srcAccessMask = vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite;
	dependencies[1].dstAccessMask = vk::AccessFlagBits::eMemoryRead;
	dependencies[1].dependencyFlags = vk::DependencyFlagBits::eByRegion;


	vk::RenderPassCreateInfo renderPassInfo = {};
	renderPassInfo.setAttachments(attachmentDescription);
	renderPassInfo.setSubpasses(subpass);
	renderPassInfo.setDependencies(dependencies);

	renderPass = context.logical.createRenderPass(renderPassInfo);
}

void Deferred::createImages(DContext& context)
{
	//디퍼드 렌더링
	//1. 이미지 5개 만들기(위치, 노말, 알베도, 깊이, (roughness,metalic,ao))
	vk::Format positionFormat = vk::Format::eR16G16B16A16Sfloat;
	vk::Format normalFormat = vk::Format::eR16G16B16A16Sfloat;
	vk::Format albedoFormat = vk::Format::eR8G8B8A8Unorm;
	vk::Format pbrFormat = vk::Format::eR8G8B8A8Unorm;
	vk::Format depthFormat = findDepthFormat(context);
	images.push_back(DImage(context, 1,
		positionFormat,
		swapChain->extent,
		vk::ImageTiling::eOptimal,
		vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled,
		vk::ImageLayout::eUndefined,
		vk::MemoryPropertyFlagBits::eDeviceLocal,
		vk::ImageAspectFlagBits::eColor));


	images.push_back(DImage(context, 1,
		normalFormat,
		swapChain->extent,
		vk::ImageTiling::eOptimal,
		vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled,
		vk::ImageLayout::eUndefined,
		vk::MemoryPropertyFlagBits::eDeviceLocal,
		vk::ImageAspectFlagBits::eColor));


	images.push_back(DImage(context, 1,
		albedoFormat,
		swapChain->extent,
		vk::ImageTiling::eOptimal,
		vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled,
		vk::ImageLayout::eUndefined,
		vk::MemoryPropertyFlagBits::eDeviceLocal,
		vk::ImageAspectFlagBits::eColor));


	images.push_back(DImage(context, 1,
		pbrFormat,
		swapChain->extent,
		vk::ImageTiling::eOptimal,
		vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled,
		vk::ImageLayout::eUndefined,
		vk::MemoryPropertyFlagBits::eDeviceLocal,
		vk::ImageAspectFlagBits::eColor));



	if (depthFormat >= vk::Format::eD16UnormS8Uint) {
		images.push_back(DImage(context, 1,
			depthFormat,
			swapChain->extent,
			vk::ImageTiling::eOptimal,
			vk::ImageUsageFlagBits::eDepthStencilAttachment,
			vk::ImageLayout::eUndefined,
			vk::MemoryPropertyFlagBits::eDeviceLocal,
			vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil));
	}
	else {

		images.push_back(DImage(context, 1,
			depthFormat,
			swapChain->extent,
			vk::ImageTiling::eOptimal,
			vk::ImageUsageFlagBits::eDepthStencilAttachment,
			vk::ImageLayout::eUndefined,
			vk::MemoryPropertyFlagBits::eDeviceLocal,
			vk::ImageAspectFlagBits::eDepth));
	}
}

void Deferred::createFramebuffers(DContext& context)
{
	framebuffers.reserve(swapChain->images.size());

	for (size_t i = 0; i < swapChain->images.size(); i++) {
		std::array<vk::ImageView, 5> attachments = {
			images[0].view,
			images[1].view,
			images[2].view,
			images[3].view,
			images[4].view
		};
		vk::FramebufferCreateInfo framebufferInfo{};
		framebufferInfo.setAttachments(attachments);
		framebufferInfo.renderPass = renderPass;
		framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
		framebufferInfo.pAttachments = attachments.data();
		framebufferInfo.width = swapChain->extent.width;
		framebufferInfo.height = swapChain->extent.height;
		framebufferInfo.layers = 1;

		framebuffers.emplace_back(context.logical,framebufferInfo,nullptr);
	}
}

void Deferred::updateDescriptorSets(DContext& context)
{

	for (size_t frame = 0; frame < MAX_FRAMES_IN_FLIGHT; frame++) {
		std::vector<vk::DescriptorImageInfo> imageInfos(4);
		for (int i = 0; i < 4; i++) {
			auto& imageInfo = imageInfos[i];
			imageInfo.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
			imageInfo.imageView = images[i].view;
			imageInfo.sampler = Sampler::Get(SamplerMipMapType::Low);

		}
		vk::WriteDescriptorSet descriptorWrite{};
		descriptorWrite.dstSet = descriptorSets[frame];
		descriptorWrite.dstBinding = 0;
		descriptorWrite.dstArrayElement = 0;
		descriptorWrite.descriptorType = vk::DescriptorType::eCombinedImageSampler;
		descriptorWrite.descriptorCount = imageInfos.size();
		descriptorWrite.pImageInfo = imageInfos.data();

		context.logical.updateDescriptorSets(descriptorWrite, nullptr);
	}
}
