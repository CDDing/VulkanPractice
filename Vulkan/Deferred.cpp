#include "pch.h"
#include "Deferred.h"

Deferred::Deferred(std::shared_ptr<Device> device, SwapChain& swapChain) : device(device), swapChain(&swapChain)
{
	createImages();
	createRenderPass();
	createFramebuffers();
}

void Deferred::destroy()
{
	for (auto& image : images) {
		image.image.destroy(device);
		image.imageView.destroy(device);
	}
	for (auto& framebuffer : framebuffers) {
		device->logical.destroyFramebuffer(framebuffer);
	}
	device->logical.destroyRenderPass(renderPass);
}

void Deferred::createRenderPass()
{
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

	attachmentDescription[0].format = images[0].image.format;
	attachmentDescription[1].format = images[1].image.format;
	attachmentDescription[2].format = images[2].image.format;
	attachmentDescription[3].format = images[3].image.format;
	attachmentDescription[4].format = images[4].image.format;
	attachmentDescription[5].format = images[5].image.format;
	attachmentDescription[6].format = images[6].image.format;

	std::vector<vk::AttachmentReference> colorReferences;
	colorReferences.push_back({ 0,vk::ImageLayout::eColorAttachmentOptimal });
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
	renderPassInfo.pAttachments = attachmentDescription.data();
	renderPassInfo.attachmentCount = static_cast<uint32_t>(attachmentDescription.size());
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;
	renderPassInfo.dependencyCount = 2;
	renderPassInfo.pDependencies = dependencies.data();

	renderPass = device->logical.createRenderPass(renderPassInfo);
}

void Deferred::createImages()
{
	//디퍼드 렌더링
	//1. 이미지 7개 만들기(위치, 노말, 알베도, 깊이, roughness,metalic,ao)
	vk::Format positionFormat = vk::Format::eR16G16B16A16Sfloat;
	vk::Format normalFormat = vk::Format::eR16G16B16A16Sfloat;
	vk::Format albedoFormat = vk::Format::eR8G8B8A8Unorm;
	vk::Format roughnessFormat = vk::Format::eR8G8B8A8Unorm;
	vk::Format metalnessFormat = vk::Format::eR8G8B8A8Unorm;
	vk::Format aoFormat = vk::Format::eR8G8B8A8Unorm;
	vk::Format depthFormat = findDepthFormat(*device);
	images.resize(7);
	images[0].image = Image(device,
		swapChain->extent.width, swapChain->extent.height, 1, positionFormat,
		vk::ImageTiling::eOptimal,
		vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled,
		vk::MemoryPropertyFlagBits::eDeviceLocal);
	images[0].imageView = ImageView(device,
		images[0].image,
		positionFormat,
		vk::ImageAspectFlagBits::eColor, 1);

	images[1].image = Image(device,
		swapChain->extent.width, swapChain->extent.height,
		1, normalFormat, vk::ImageTiling::eOptimal,
		vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled,
		vk::MemoryPropertyFlagBits::eDeviceLocal);
	images[1].imageView = ImageView(device,
		images[1].image,
		normalFormat,
		vk::ImageAspectFlagBits::eColor, 1);

	images[2].image = Image(device
		, swapChain->extent.width, swapChain->extent.height,
		1, albedoFormat,
		vk::ImageTiling::eOptimal,
		vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled
		, vk::MemoryPropertyFlagBits::eDeviceLocal);
	images[2].imageView = ImageView(device,
		images[2].image,
		albedoFormat,
		vk::ImageAspectFlagBits::eColor, 1);

	images[3].image = Image(device,
		swapChain->extent.width, swapChain->extent.height,
		1, roughnessFormat,
		vk::ImageTiling::eOptimal,
		vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled
		, vk::MemoryPropertyFlagBits::eDeviceLocal);
	images[3].imageView = ImageView(device,
		images[3].image,
		roughnessFormat,
		vk::ImageAspectFlagBits::eColor, 1);

	images[4].image = Image(device,
		swapChain->extent.width, swapChain->extent.height,
		1, metalnessFormat,
		vk::ImageTiling::eOptimal,
		vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled
		, vk::MemoryPropertyFlagBits::eDeviceLocal);
	images[4].imageView = ImageView(device,
		images[4].image,
		metalnessFormat,
		vk::ImageAspectFlagBits::eColor, 1);

	images[5].image = Image(device,
		swapChain->extent.width, swapChain->extent.height,
		1, aoFormat,
		vk::ImageTiling::eOptimal,
		vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled
		, vk::MemoryPropertyFlagBits::eDeviceLocal);
	images[5].imageView = ImageView(device,
		images[5].image,
		aoFormat,
		vk::ImageAspectFlagBits::eColor, 1);

	//이미지 뷰 만들기
	if (depthFormat >= vk::Format::eD16UnormS8Uint) {
		images[6].image = Image(device, swapChain->extent.width, swapChain->extent.height, 1, depthFormat, vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eDepthStencilAttachment, vk::MemoryPropertyFlagBits::eDeviceLocal);
		images[6].imageView = ImageView(device, images[6].image, depthFormat, vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil, 1);
	}
	else {
		images[6].image = Image(device, swapChain->extent.width, swapChain->extent.height, 1, depthFormat, vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eDepthStencilAttachment, vk::MemoryPropertyFlagBits::eDeviceLocal);
		images[6].imageView = ImageView(device, images[6].image, depthFormat, vk::ImageAspectFlagBits::eDepth, 1);
	}
}

void Deferred::createFramebuffers()
{


	framebuffers.resize(swapChain->images.size());
	for (size_t i = 0; i < swapChain->images.size(); i++) {
		std::array<vk::ImageView, 7> attachments = {
			images[0].imageView,
			images[1].imageView,
			images[2].imageView,
			images[3].imageView,
			images[4].imageView,
			images[5].imageView,
			images[6].imageView,
		};
		vk::FramebufferCreateInfo framebufferInfo{};
		framebufferInfo.setAttachments(attachments);
		framebufferInfo.renderPass = renderPass.operator vk::RenderPass & ();
		framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
		framebufferInfo.pAttachments = attachments.data();
		framebufferInfo.width = swapChain->extent.width;
		framebufferInfo.height = swapChain->extent.height;
		framebufferInfo.layers = 1;

		framebuffers[i] = device->logical.createFramebuffer(framebufferInfo);
	}
}

void Deferred::updateDescriptorSets()
{

	for (size_t frame = 0; frame < MAX_FRAMES_IN_FLIGHT; frame++) {
		std::vector<vk::DescriptorImageInfo> imageInfos(6);
		for (int i = 0; i < 6; i++) {
			auto& imageInfo = imageInfos[i];
			imageInfo.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
			imageInfo.imageView = images[i].imageView;
			imageInfo.sampler = Sampler::Get(SamplerMipMapType::Low);

		}
		vk::WriteDescriptorSet descriptorWrite{};
		descriptorWrite.dstSet = descriptorSets[frame];
		descriptorWrite.dstBinding = 0;
		descriptorWrite.dstArrayElement = 0;
		descriptorWrite.descriptorType = vk::DescriptorType::eCombinedImageSampler;
		descriptorWrite.descriptorCount = imageInfos.size();
		descriptorWrite.pImageInfo = imageInfos.data();

		device->logical.updateDescriptorSets(descriptorWrite, nullptr);
	}
}
