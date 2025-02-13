#pragma once
class Semaphore
{
public:
	Semaphore(std::shared_ptr<Device> device) : _device(device){

		vk::SemaphoreCreateInfo semaphoreInfo{};

		_semaphore = device->logical.createSemaphore(semaphoreInfo);
	}
	~Semaphore() {
		_device->logical.destroySemaphore(_semaphore);
	}
	operator vk::Semaphore() {
		return _semaphore;
	}
private:
	vk::Semaphore _semaphore;
	std::shared_ptr<Device> _device;
};

