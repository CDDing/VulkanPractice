#pragma once
class RayTracing
{

public:
	void init(Device& device, std::vector<Buffer>& uboBuffers, SwapChain swapChain);
	void destroy(Device& device);

	struct AccelerationStructure {
		VkAccelerationStructureKHR handle;
		uint64_t deviceAddress;
		Buffer buffer;
	};

	std::vector<AccelerationStructure> BLASs{};
	std::vector<AccelerationStructure> TLASs{};
	std::vector<Image> outputImages;
	std::vector<ImageView> outputImageViews;
	DescriptorSetLayout descriptorSetLayout;
	std::vector<DescriptorSet> descriptorSets;
	DescriptorPool descriptorPool;
	VkPipelineLayout pipelineLayout;
private:
	void createTlas(Device& device);
	void createBlas(Device& device);
	void createSBT(Device& device);
	void createRTPipeline(Device& device);
	void createOutputImage(Device& device,SwapChain swapChain);
	void createDescriptorSets(Device& device, std::vector<Buffer>& uboBuffers);
	void loadFunctions(Device& device);

	VkPhysicalDeviceRayTracingPipelinePropertiesKHR  rayTracingPipelineProperties{};
	VkPhysicalDeviceAccelerationStructureFeaturesKHR accelerationStructureFeatures{};
	std::vector<VkRayTracingShaderGroupCreateInfoKHR> shaderGroups{};
	VkPipeline pipeline;
	Buffer raygenShaderBindingTable;
	Buffer missShaderBindingTable;
	Buffer hitShaderBindingTable;

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

