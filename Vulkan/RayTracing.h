#pragma once
class RayTracing
{

public:
	void init(Device& device, std::vector<Buffer>& uboBuffers, SwapChain& swapChain, std::vector<Model>& models);
	void destroy(Device& device);
	void recordCommandBuffer(Device& device, VkCommandBuffer commandBuffer, int currentFrame,uint32_t imageIndex);
	struct AccelerationStructure {
		VkAccelerationStructureKHR handle;
		uint64_t deviceAddress;
		Buffer buffer;
	};

	std::vector<AccelerationStructure> BLASs{};
	std::vector<AccelerationStructure> TLASs{};
	DescriptorSetLayout descriptorSetLayout;
	std::vector<DescriptorSet> descriptorSets;
	DescriptorPool descriptorPool;
	VkPipelineLayout pipelineLayout;
private:
	void createTlas(Device& device);
	void createBlas(Device& device, std::vector<Model>& models);
	void createSBT(Device& device);
	void createRTPipeline(Device& device);
	void createDescriptorSets(Device& device, std::vector<Buffer>& uboBuffers,std::vector<ImageView>& swapChainImageViews);
	void loadFunctions(Device& device);
	
	VkPhysicalDeviceRayTracingPipelinePropertiesKHR  rayTracingPipelineProperties{};
	VkPhysicalDeviceAccelerationStructureFeaturesKHR accelerationStructureFeatures{};
	std::vector<VkRayTracingShaderGroupCreateInfoKHR> shaderGroups{};
	VkPipeline pipeline;
	Buffer raygenShaderBindingTable;
	Buffer missShaderBindingTable;
	Buffer hitShaderBindingTable;
	SwapChain* _swapChain;

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

