#pragma once
class DescriptorPool
{
public:
	DescriptorPool();
	DescriptorPool(Device& device);
	VkDescriptorPool& Get() { return _descriptorPool; }
private:
	VkDescriptorPool _descriptorPool;
};

