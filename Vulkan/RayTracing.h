#pragma once
class Scene;
class RayTracing
{

public:
	RayTracing(std::nullptr_t) : 
		descriptorPool(nullptr), 
		pipeline(nullptr), 
		pipelineLayout(nullptr),
		descriptorSetLayout(nullptr),
		raygenShaderBindingTable(nullptr),
		missShaderBindingTable(nullptr),
		hitShaderBindingTable(nullptr)
	{};
	RayTracing(Device& device, SwapChain& swapChain, Scene& scene, std::vector<DBuffer>& uboBuffers, std::vector<DBuffer>& guiBuffers);
	void recordCommandBuffer(Device& device, vk::raii::CommandBuffer& commandBuffer, int currentFrame,uint32_t imageIndex);
	struct AccelerationStructure {
		vk::raii::AccelerationStructureKHR handle;
		uint64_t deviceAddress;
		DBuffer buffer;
	};
	struct GeometryNode {
		uint64_t vertexBufferAddress;
		uint64_t indexBufferAddress;
	};

	std::vector<std::vector<AccelerationStructure>> BLASs{};
	std::vector<AccelerationStructure> TLASs{};
	std::vector<DBuffer> geometryNodeBuffers;
	vk::raii::DescriptorPool descriptorPool;
	std::vector<vk::raii::DescriptorSet> descriptorSets;
	vk::raii::DescriptorSetLayout descriptorSetLayout;
	vk::raii::PipelineLayout pipelineLayout;

private:
	void createTlas(Device& device, std::vector<Model>& models);
	void createBlas(Device& device, std::vector<Model>& models);
	void createSBT(Device& device);
	void createRTPipeline(Device& device);
	void createOutputImages(Device& device);
	void createDescriptorSets(Device& device, Scene& scene, std::vector<DBuffer>& uboBuffers,std::vector<DBuffer>& guiBuffers);
	void loadFunctions(Device& device);
	
	uint64_t getBufferDeviceAddress(Device& device, vk::raii::Buffer& buffer);
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



	vk::PhysicalDeviceRayTracingPipelinePropertiesKHR  rayTracingPipelineProperties{};
	vk::PhysicalDeviceAccelerationStructureFeaturesKHR accelerationStructureFeatures{};
	std::vector<vk::RayTracingShaderGroupCreateInfoKHR> shaderGroups{};
	DBuffer raygenShaderBindingTable;
	DBuffer missShaderBindingTable;
	DBuffer hitShaderBindingTable;
	SwapChain* swapChain;
	vk::raii::Pipeline pipeline;
	std::vector<DImage> outputImages;
};

