#include "pch.h"
#include "DescriptorSetLayout.h"

DescriptorSetLayout::DescriptorSetLayout()
{
}

DescriptorSetLayout::DescriptorSetLayout(Device& device, DescriptorType type)
{
	std::vector<vk::DescriptorSetLayoutBinding> bindings;
	auto components = DescriptorSetLayout::GetComponents(type);
	bindings = DescriptorSetLayout::inputAttributeDescriptions(components);
	vk::DescriptorSetLayoutCreateInfo layoutInfo{ {},bindings };

	_descriptorSetLayout = device.logical.createDescriptorSetLayout(layoutInfo);
}
