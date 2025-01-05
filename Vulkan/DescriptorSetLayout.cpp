#include "pch.h"
#include "DescriptorSetLayout.h"

DescriptorSetLayout::DescriptorSetLayout()
{
}

DescriptorSetLayout::DescriptorSetLayout(Device& device, ShaderType type)
{
	std::vector<VkDescriptorSetLayoutBinding> bindings;
	std::vector<ShaderComponent> components;
	switch (type) {
	case ShaderType::DEFAULT:
		components = { ShaderComponent::UNIFORM,ShaderComponent::SAMPLER ,ShaderComponent::SAMPLER };
		bindings = DescriptorSetLayout::inputAttributeDescriptions(components);
		break;
	case ShaderType::SKYBOX:
		components = { ShaderComponent::UNIFORM,ShaderComponent::SAMPLER };
		bindings = DescriptorSetLayout::inputAttributeDescriptions(components);
		break;
	}
	VkDescriptorSetLayoutCreateInfo layoutInfo{};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
	layoutInfo.pBindings = bindings.data();

	if (vkCreateDescriptorSetLayout(device.Get(), &layoutInfo, nullptr, &_descriptorSetLayout) != VK_SUCCESS) {
		throw std::runtime_error("failed to create descriptor set layout!");
	}
}
