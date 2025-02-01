#pragma once
class DescriptorPool
{
public:
	DescriptorPool();
	DescriptorPool(Device& device);
	operator VkDescriptorPool& () {
		return _descriptorPool;
	}
	VkDescriptorPool* operator&() {
		return &_descriptorPool;
	}
	void destroy(Device& device) {
		vkDestroyDescriptorPool(device, _descriptorPool, nullptr);
	}
private:
	VkDescriptorPool _descriptorPool;
};

