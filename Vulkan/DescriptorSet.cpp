#include "pch.h"
#include "DescriptorSet.h"

DescriptorSet::DescriptorSet()
{
}

DescriptorSet::DescriptorSet(Device& device, DescriptorPool& descriptorPool, DescriptorSetLayout& descriptorSetLayout)
{
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = descriptorPool.Get();
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &descriptorSetLayout.Get();

    if (vkAllocateDescriptorSets(device, &allocInfo, &_descriptorSet) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate descriptor sets!");
    }
}
