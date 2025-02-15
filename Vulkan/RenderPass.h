#pragma once
class RenderPass
{
public:

	RenderPass();
	RenderPass(std::shared_ptr<Device> device, vk::ArrayProxyNoTemporaries<vk::AttachmentDescription> attachments,vk::SubpassDescription subpass,vk::SubpassDependency dependency);
	operator vk::RenderPass& () {
		return renderPass;
	}
	operator VkRenderPass() {
		return static_cast<VkRenderPass>(renderPass);
	}
	void operator=(vk::RenderPass renderPass) {
		this->renderPass = renderPass;
	}
	vk::RenderPass* operator&() {
		return &renderPass;
	}
	void destroy() {
	}

private:
	vk::RenderPass renderPass;
	std::shared_ptr<Device> device;
};

