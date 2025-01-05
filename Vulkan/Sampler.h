#pragma once
class Sampler
{
public:
	Sampler();
	Sampler(Device& device, uint32_t mipLevels);
	Sampler(VkSampler sampler) : _sampler(sampler) {};
	VkSampler& Get() { return _sampler; }
private:
	VkSampler _sampler;
};

