#pragma once
class Texture;
class Material
{
public:
	Material() {};
	void createDescriptorSet(Device& device, VkDescriptorPool descriptorPool, VkDescriptorSetLayout descriptorSetLayout, uint32_t descriptorBindingFlags);	
private:
	Texture* baseColor;
	Texture* normalMap;
	DescriptorSet descriptorSet;
};

