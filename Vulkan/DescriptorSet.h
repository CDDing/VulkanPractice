#pragma once
class DescriptorSet
{
public:
	DescriptorSet();
	DescriptorSet(Device& device, DescriptorPool& descriptorPool, DescriptorSetLayout& descriptorSetLayout);
	operator vk::DescriptorSet& () {
		return _descriptorSet;
	}
	vk::DescriptorSet* operator&() {
		return &_descriptorSet;
	}
private:
	vk::DescriptorSet _descriptorSet;
};
enum class DescriptorType {
	VP,
	Skybox,
	Material,
	Model,
	GBuffer,
	ImGUI,
	RayTracing,
	END
};