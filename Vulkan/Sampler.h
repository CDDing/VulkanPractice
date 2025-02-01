#pragma once
class Sampler
{
public:
	Sampler();
	Sampler(Device& device, uint32_t mipLevels);
	Sampler(VkSampler sampler) : _sampler(sampler) {};
	operator VkSampler& () {
		return _sampler;
	}
private:
	VkSampler _sampler;
};

