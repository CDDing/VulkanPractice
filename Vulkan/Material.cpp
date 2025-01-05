#include "pch.h"
#include "Material.h"

void Material::createDescriptorSet(Device& device, VkDescriptorPool descriptorPool, VkDescriptorSetLayout descriptorSetLayout, uint32_t descriptorBindingFlags)
{
	//VkDescriptorSetAllocateInfo descriptorSetAllocInfo{};
	//descriptorSetAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	//descriptorSetAllocInfo.descriptorPool = descriptorPool;
	//descriptorSetAllocInfo.pSetLayouts = &descriptorSetLayout;
	//descriptorSetAllocInfo.descriptorSetCount = 1;
	//vkAllocateDescriptorSets(device.Get(), &descriptorSetAllocInfo, &descriptorSet.Get());
	//std::vector<VkDescriptorImageInfo> imageDescriptors{};
	//std::vector<VkWriteDescriptorSet> writeDescriptorSets{};
	//if (descriptorBindingFlags & 0x00000001) {//BaseColor
	//	imageDescriptors.push_back(baseColor->descriptor);
	//	VkWriteDescriptorSet writeDescriptorSet{};
	//	writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	//	writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	//	writeDescriptorSet.descriptorCount = 1;
	//	writeDescriptorSet.dstSet = descriptorSet.Get();
	//	writeDescriptorSet.dstBinding = static_cast<uint32_t>(writeDescriptorSets.size());
	//	writeDescriptorSet.pImageInfo = &baseColor->descriptor;
	//	writeDescriptorSets.push_back(writeDescriptorSet);
	//}
	//if (normalMap && descriptorBindingFlags & 0x00000002) {
	//	imageDescriptors.push_back(normalMap->descriptor);
	//	VkWriteDescriptorSet writeDescriptorSet{};
	//	writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	//	writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	//	writeDescriptorSet.descriptorCount = 1;
	//	writeDescriptorSet.dstSet = descriptorSet.Get();
	//	writeDescriptorSet.dstBinding = static_cast<uint32_t>(writeDescriptorSets.size());
	//	writeDescriptorSet.pImageInfo = &normalMap->descriptor;
	//	writeDescriptorSets.push_back(writeDescriptorSet);
	//}
	//vkUpdateDescriptorSets(device.Get(), static_cast<uint32_t>(writeDescriptorSets.size()), writeDescriptorSets.data(), 0, nullptr);

}
