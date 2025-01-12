#pragma once
class DescriptorSet
{
public:
	DescriptorSet();
	DescriptorSet(Device& device, DescriptorPool& descriptorPool, DescriptorSetLayout& descriptorSetLayout);
	VkDescriptorSet& Get() { return _descriptorSet; }
private:
	VkDescriptorSet _descriptorSet;
};

enum class DescriptorInfoType{
	Buffer,
	Image
};
enum class DescriptorType {
	VP,
	Skybox,
	Material,
	Model,
	END
};