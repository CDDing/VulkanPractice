#pragma once
class Scene;
class RayTracing
{

public:
	void init(Device& device, std::vector<Buffer>& uboBuffers, SwapChain& swapChain, Scene& scene,std::vector<Buffer>& guiBuffers);
	void destroy(Device& device);
	void recordCommandBuffer(Device& device, vk::CommandBuffer commandBuffer, int currentFrame,uint32_t imageIndex);
	struct AccelerationStructure {
		VkAccelerationStructureKHR handle;
		uint64_t deviceAddress;
		Buffer buffer;
	};
	struct GeometryNode {
		uint64_t vertexBufferAddress;
		uint64_t indexBufferAddress;
	};

	std::vector<std::vector<AccelerationStructure>> BLASs{};
	std::vector<AccelerationStructure> TLASs{};
	DescriptorSetLayout descriptorSetLayout;
	std::vector<DescriptorSet> descriptorSets;
	DescriptorPool descriptorPool;
	vk::PipelineLayout pipelineLayout;

	std::vector<Buffer> geometryNodeBuffers;
private:
	void createTlas(Device& device, std::vector<Model>& models);
	void createBlas(Device& device, std::vector<Model>& models);
	void createSBT(Device& device);
	void createRTPipeline(Device& device);
	void createOutputImages(Device& device);
	void createDescriptorSets(Device& device, std::vector<Buffer>& uboBuffers,std::vector<Buffer>& guiBuffers, Scene& scene);
	void loadFunctions(Device& device);
	
	vk::PhysicalDeviceRayTracingPipelinePropertiesKHR  rayTracingPipelineProperties{};
	vk::PhysicalDeviceAccelerationStructureFeaturesKHR accelerationStructureFeatures{};
	std::vector<vk::RayTracingShaderGroupCreateInfoKHR> shaderGroups{};
	vk::Pipeline pipeline;
	Buffer raygenShaderBindingTable;
	Buffer missShaderBindingTable;
	std::vector<ImageSet> outputImages;
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

