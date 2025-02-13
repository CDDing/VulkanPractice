#pragma once
class Fence
{
public:
	Fence(std::shared_ptr<Device> device) : _device(device) {
		vk::FenceCreateInfo fenceInfo{ vk::FenceCreateFlagBits::eSignaled };
		_fence = device->logical.createFence(fenceInfo);
	}
	operator vk::Fence() {
		return _fence;
	}
	~Fence() {
		_device->logical.destroyFence(_fence);
	}
private:
	vk::Fence _fence;
	std::shared_ptr<Device> _device;
};
