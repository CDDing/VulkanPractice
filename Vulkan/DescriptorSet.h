#pragma once
class DescriptorSet
{
public:
	DescriptorSet();
	DescriptorSet(Device& device, DescriptorSetLayout& descriptorSetLayout);
	
private:
	VkDescriptorSet _descriptorSet;
};

