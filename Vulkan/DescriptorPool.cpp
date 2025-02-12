#include "pch.h"
#include "DescriptorPool.h"

DescriptorPool::DescriptorPool()
{
}

DescriptorPool::DescriptorPool(Device& device)
{
	std::array<vk::DescriptorPoolSize, 2> poolSizes{};
	poolSizes[0].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
	poolSizes[1].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

	vk::DescriptorPoolCreateInfo poolInfo{ {/*Flags*/},
		static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT) * 100 ,poolSizes};
	
	_descriptorPool = device.logical.createDescriptorPool(poolInfo);
}
