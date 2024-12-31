#pragma once
class DescriptorSetLayout
{
public:
	DescriptorSetLayout();
	DescriptorSetLayout(Device& device);
	VkDescriptorSetLayout& Get() { return _descriptorSetLayout; }
private:
	VkDescriptorSetLayout _descriptorSetLayout;
};

