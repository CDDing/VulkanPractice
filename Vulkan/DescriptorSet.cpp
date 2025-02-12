#include "pch.h"
#include "DescriptorSet.h"

DescriptorSet::DescriptorSet()
{
}

DescriptorSet::DescriptorSet(Device& device, DescriptorPool& descriptorPool, DescriptorSetLayout& descriptorSetLayout)
{
    vk::DescriptorSetAllocateInfo allocInfo{ descriptorPool,descriptorSetLayout };

    _descriptorSet = device.logical.allocateDescriptorSets(allocInfo).front();
}
