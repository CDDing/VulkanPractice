#pragma once
class DescriptorPool
{
public:
	DescriptorPool();
	DescriptorPool(Device& device);
	operator vk::DescriptorPool& () {
		return _descriptorPool;
	}
	vk::DescriptorPool* operator&() {
		return &_descriptorPool;
	}
	operator VkDescriptorPool() {
		return (VkDescriptorPool)_descriptorPool;
	}
	void destroy(vk::Device& device) {
		device.destroyDescriptorPool(_descriptorPool);
	}
	void operator=(vk::DescriptorPool pool) {
		_descriptorPool = pool;
	}
private:
	vk::DescriptorPool _descriptorPool;
};

