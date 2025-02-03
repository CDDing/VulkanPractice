#pragma once
class DescriptorSet
{
public:
	DescriptorSet();
	DescriptorSet(Device& device, DescriptorPool& descriptorPool, DescriptorSetLayout& descriptorSetLayout);
	operator VkDescriptorSet& () {
		return _descriptorSet;
	}
	VkDescriptorSet* operator&() {
		return &_descriptorSet;
	}
private:
	VkDescriptorSet _descriptorSet;
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