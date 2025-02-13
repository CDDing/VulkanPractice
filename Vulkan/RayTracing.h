#pragma once
class Scene;
class RayTracing
{

public:
	void init(std::shared_ptr<Device> device, std::vector<std::shared_ptr<Buffer>>& uboBuffers, SwapChain& swapChain, Scene& scene,std::vector<std::shared_ptr<Buffer>>& guiBuffers);
	void destroy();
	void recordCommandBuffer(vk::CommandBuffer commandBuffer, int currentFrame,uint32_t imageIndex);
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
	std::shared_ptr<DescriptorPool> descriptorPool;
	vk::PipelineLayout pipelineLayout;

	std::vector<Buffer> geometryNodeBuffers;
private:
	std::shared_ptr<Device> _device;
	void createTlas(std::vector<Model>& models);
	void createBlas(std::vector<Model>& models);
	void createSBT();
	void createRTPipeline();
	void createOutputImages();
	void createDescriptorSets(std::vector<std::shared_ptr<Buffer>>& uboBuffers,std::vector<std::shared_ptr<Buffer>>& guiBuffers, Scene& scene);
	void loadFunctions();
	
	vk::PhysicalDeviceRayTracingPipelinePropertiesKHR  rayTracingPipelineProperties{};
	vk::PhysicalDeviceAccelerationStructureFeaturesKHR accelerationStructureFeatures{};
	std::vector<vk::RayTracingShaderGroupCreateInfoKHR> shaderGroups{};
	vk::Pipeline pipeline;
	Buffer raygenShaderBindingTable;
	Buffer missShaderBindingTable;
	std::vector<ImageSet> outputImages;
	Buffer hitShaderBindingTable;
	SwapChain* _swapChain;
	
	uint64_t getBufferDeviceAddress(Buffer& buffer);
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

