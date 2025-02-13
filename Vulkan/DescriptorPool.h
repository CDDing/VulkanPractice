#pragma once
class DescriptorPool
{
	friend class GUI;
	friend class RayTracing;
public:
	DescriptorPool();
	DescriptorPool(std::shared_ptr<Device> device);
	operator vk::DescriptorPool& () {
		return _descriptorPool;
	}
	vk::DescriptorPool* operator&() {
		return &_descriptorPool;
	}
	operator VkDescriptorPool() {
		return (VkDescriptorPool)_descriptorPool;
	}
	~DescriptorPool() {

		if(_descriptorPool) _device->logical.destroyDescriptorPool(_descriptorPool);
	}
	void operator=(vk::DescriptorPool pool) {
		_descriptorPool = pool;
	}
private:
	vk::DescriptorPool _descriptorPool;
	std::shared_ptr<Device> _device;
};

