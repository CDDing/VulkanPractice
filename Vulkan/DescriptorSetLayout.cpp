#include "pch.h"
#include "DescriptorSetLayout.h"

DescriptorSetLayout::DescriptorSetLayout()
{
}

DescriptorSetLayout::DescriptorSetLayout(Device& device, DescriptorType type)
{
	std::vector<VkDescriptorSetLayoutBinding> bindings;
	auto components = DescriptorSetLayout::GetComponents(type);
	bindings = DescriptorSetLayout::inputAttributeDescriptions(components);
	VkDescriptorSetLayoutCreateInfo layoutInfo{};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
	layoutInfo.pBindings = bindings.data();

	if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &_descriptorSetLayout) != VK_SUCCESS) {
		throw std::runtime_error("failed to create descriptor set layout!");
	}
}
