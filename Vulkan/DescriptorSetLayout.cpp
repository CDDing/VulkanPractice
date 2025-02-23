#include "pch.h"
#include "DescriptorSetLayout.h"
vk::raii::DescriptorPool DescriptorPool::Pool(nullptr);
std::vector<vk::raii::DescriptorSetLayout> DescriptorSetLayout::descriptorSetLayouts = {};