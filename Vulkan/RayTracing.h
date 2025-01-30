#pragma once
class RayTracing
{

public:
	void init(Device& device, int currentFrame);
	void destroy(Device& device);

	struct AccelerationStructure {
		VkAccelerationStructureKHR handle;
		uint64_t deviceAddress;
		Buffer buffer;
	};

	AccelerationStructure blas{};
	AccelerationStructure tlas{};
	DescriptorSetLayout descriptorSetLayout;
	DescriptorSet descriptorSet;
	VkPipelineLayout pipelineLayout;
private:
	void createTlas(Device& device);
	void createBlas(Device& device, int currentFrame);
	void createSBT(Device& device);
	void createRTPipeline(Device& device,int currentFrame);
	void loadFunctions(Device& device);

	VkPhysicalDeviceRayTracingPipelinePropertiesKHR  rayTracingPipelineProperties{};
	VkPhysicalDeviceAccelerationStructureFeaturesKHR accelerationStructureFeatures{};
	std::vector<VkRayTracingShaderGroupCreateInfoKHR> shaderGroups{};
	VkPipeline pipeline;

	uint64_t getBufferDeviceAddress(Device& device, Buffer& buffer);
	PFN_vkGetBufferDeviceAddressKHR vkGetBufferDeviceAddressKHR;
	PFN_vkCreateAccelerationStructureKHR vkCreateAccelerationStructureKHR;
	PFN_vkDestroyAccelerationStructureKHR vkDestroyAccelerationStructureKHR;
	PFN_vkGetAccelerationStructureBuildSizesKHR vkGetAccelerationStructureBuildSizesKHR;
	PFN_vkGetAccelerationStructureDeviceAddressKHR vkGetAccelerationStructureDeviceAddressKHR;
	PFN_vkCmdBuildAccelerationStructuresKHR vkCmdBuildAccelerationStructuresKHR;
	PFN_vkBuildAccelerationStructuresKHR vkBuildAccelerationStructuresKHR;
	PFN_vkCmdTraceRaysKHR vkCmdTraceRaysKHR;
	PFN_vkGetRayTracingShaderGroupHandlesKHR vkGetRayTracingShaderGroupHandlesKHR;
	PFN_vkCreateRayTracingPipelinesKHR vkCreateRayTracingPipelinesKHR;
};

