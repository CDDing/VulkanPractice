#include "pch.h"
#include "RayTracing.h"


void RayTracing::init(Device& device, std::vector<Buffer>& uboBuffers, SwapChain& swapChain,Scene& scene)
{
	_swapChain = &swapChain;
	// Get ray tracing pipeline properties, which will be used later on in the sample
	rayTracingPipelineProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR;
	VkPhysicalDeviceProperties2 deviceProperties2{};
	deviceProperties2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
	deviceProperties2.pNext = &rayTracingPipelineProperties;
	vkGetPhysicalDeviceProperties2(device, &deviceProperties2);

	loadFunctions(device);

	createOutputImages(device);
	BLASs.resize(MAX_FRAMES_IN_FLIGHT);
	TLASs.resize(MAX_FRAMES_IN_FLIGHT);
	createBlas(device,scene.models);
	createTlas(device);
	createRTPipeline(device);
	createSBT(device);
	createDescriptorSets(device, uboBuffers,scene);
}

void RayTracing::createTlas(Device& device)
{
	VkTransformMatrixKHR transformMatrix = {
			1.0f, 0.0f, 0.0f, 0.0f,
			0.0f, 1.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 1.0f, 0.0f };

	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		VkAccelerationStructureInstanceKHR instance{};
		instance.transform = transformMatrix;
		instance.instanceCustomIndex = 0;
		instance.mask = 0xFF;
		instance.instanceShaderBindingTableRecordOffset = 0;
		instance.flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;
		instance.accelerationStructureReference = BLASs[i].deviceAddress;

		Buffer instanceBuffer = Buffer(device,
			sizeof(VkAccelerationStructureInstanceKHR),
			VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
		instanceBuffer.map(device, sizeof(VkAccelerationStructureInstanceKHR), 0);
		memcpy(instanceBuffer.mapped, &instance, sizeof(VkAccelerationStructureInstanceKHR));
		instanceBuffer.unmap(device);

		VkDeviceOrHostAddressConstKHR instanceDataDeviceAddress{};
		instanceDataDeviceAddress.deviceAddress = getBufferDeviceAddress(device, instanceBuffer);

		VkAccelerationStructureGeometryKHR accelerationStructureGeometry{};
		accelerationStructureGeometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
		accelerationStructureGeometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
		accelerationStructureGeometry.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
		accelerationStructureGeometry.geometry.instances.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
		accelerationStructureGeometry.geometry.instances.arrayOfPointers = VK_FALSE;
		accelerationStructureGeometry.geometry.instances.data = instanceDataDeviceAddress;


		VkAccelerationStructureBuildGeometryInfoKHR accelerationStructureBuildGeometryInfo{};
		accelerationStructureBuildGeometryInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
		accelerationStructureBuildGeometryInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
		accelerationStructureBuildGeometryInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
		accelerationStructureBuildGeometryInfo.geometryCount = 1;
		accelerationStructureBuildGeometryInfo.pGeometries = &accelerationStructureGeometry;

		uint32_t primitive_count = 1;

		VkAccelerationStructureBuildSizesInfoKHR accelerationStructureBuildSizesInfo{};
		accelerationStructureBuildSizesInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
		vkGetAccelerationStructureBuildSizesKHR(device, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
			&accelerationStructureBuildGeometryInfo,
			&primitive_count,
			&accelerationStructureBuildSizesInfo);

		TLASs[i].buffer = Buffer(device, accelerationStructureBuildSizesInfo.accelerationStructureSize,
			VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

		VkAccelerationStructureCreateInfoKHR accelerationStructureCreateInfo{};
		accelerationStructureCreateInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
		accelerationStructureCreateInfo.buffer = TLASs[i].buffer;
		accelerationStructureCreateInfo.size = accelerationStructureBuildSizesInfo.accelerationStructureSize;
		accelerationStructureCreateInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
		vkCreateAccelerationStructureKHR(device, &accelerationStructureCreateInfo, nullptr, &TLASs[i].handle);

		Buffer scratchBuffer = Buffer(device, accelerationStructureBuildSizesInfo.buildScratchSize,
			VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
		);

		VkAccelerationStructureBuildGeometryInfoKHR accelerationBuildGeometryInfo{};
		accelerationBuildGeometryInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
		accelerationBuildGeometryInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
		accelerationBuildGeometryInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
		accelerationBuildGeometryInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
		accelerationBuildGeometryInfo.dstAccelerationStructure = TLASs[i].handle;
		accelerationBuildGeometryInfo.geometryCount = 1;
		accelerationBuildGeometryInfo.pGeometries = &accelerationStructureGeometry;
		accelerationBuildGeometryInfo.scratchData.deviceAddress = getBufferDeviceAddress(device, scratchBuffer);

		VkAccelerationStructureBuildRangeInfoKHR accelerationStructureBuildRangeInfo{};
		accelerationStructureBuildRangeInfo.primitiveCount = 1;
		accelerationStructureBuildRangeInfo.primitiveOffset = 0;
		accelerationStructureBuildRangeInfo.firstVertex = 0;
		accelerationStructureBuildRangeInfo.transformOffset = 0;
		std::vector<VkAccelerationStructureBuildRangeInfoKHR*> accelerationBuildStructureRangeInfos = { &accelerationStructureBuildRangeInfo };

		VkCommandBuffer commandBuffer = beginSingleTimeCommands(device);
		vkCmdBuildAccelerationStructuresKHR(commandBuffer, 1, &accelerationBuildGeometryInfo, accelerationBuildStructureRangeInfos.data());
		endSingleTimeCommands(device, commandBuffer);

		VkAccelerationStructureDeviceAddressInfoKHR accelerationDeviceAddressInfo{};
		accelerationDeviceAddressInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
		accelerationDeviceAddressInfo.accelerationStructure = TLASs[i].handle;
		TLASs[i].deviceAddress = vkGetAccelerationStructureDeviceAddressKHR(device, &accelerationDeviceAddressInfo);

		scratchBuffer.destroy(device);
		instanceBuffer.destroy(device);
	}
}

void RayTracing::createBlas(Device&device, std::vector<Model>& models)
{
	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		VkAccelerationStructureGeometryKHR accelerationGeometry{};
		accelerationGeometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
		accelerationGeometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
		accelerationGeometry.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
		accelerationGeometry.geometry.triangles.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;

		std::vector<VkAccelerationStructureGeometryKHR> geometries;
		std::vector<Model> modelss;
		std::vector<uint32_t> maxPrimitiveCounts;
		std::vector<VkAccelerationStructureBuildRangeInfoKHR> buildRangeInfos;
		std::vector<VkAccelerationStructureBuildRangeInfoKHR*> pbuildRangeInfos;
		std::vector<GeometryNode> geometryNodes;
		for (int j = 0; j < 3;j++) {
			auto& model = models[j];
			for (const auto& mesh : model.meshes) {
				VkDeviceOrHostAddressConstKHR vertexDeviceAddress{};
				VkDeviceOrHostAddressConstKHR indexDeviceAddress{};
				VkDeviceOrHostAddressConstKHR transformDeviceAddress{};
				vertexDeviceAddress.deviceAddress = getBufferDeviceAddress(device, mesh->vertexBuffer);
				indexDeviceAddress.deviceAddress = getBufferDeviceAddress(device, mesh->indexBuffer);
				transformDeviceAddress.deviceAddress = getBufferDeviceAddress(device, model.uniformBuffers[i]);

				VkAccelerationStructureGeometryTrianglesDataKHR triangles{};
				triangles.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
				triangles.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
				triangles.vertexData = vertexDeviceAddress;
				triangles.maxVertex = mesh->vertices.size();
				triangles.vertexStride = sizeof(Vertex);

				triangles.indexType = VK_INDEX_TYPE_UINT32;
				triangles.indexData = indexDeviceAddress;

				triangles.transformData = transformDeviceAddress;

				accelerationGeometry.geometry.triangles = triangles;

				geometries.push_back(accelerationGeometry);

				auto numTriangles = static_cast<uint32_t>(mesh->indices.size() / 3);
				maxPrimitiveCounts.push_back(numTriangles);

				VkAccelerationStructureBuildRangeInfoKHR accelerationStructureBuildRangeInfo{};
				accelerationStructureBuildRangeInfo.primitiveCount = numTriangles;
				accelerationStructureBuildRangeInfo.primitiveOffset = 0;
				accelerationStructureBuildRangeInfo.firstVertex = 0;
				accelerationStructureBuildRangeInfo.transformOffset = 0;
				buildRangeInfos.push_back(accelerationStructureBuildRangeInfo);

				GeometryNode gn{};
				gn.vertexBufferAddress = vertexDeviceAddress.deviceAddress;
				gn.indexBufferAddress = indexDeviceAddress.deviceAddress;
				geometryNodes.push_back(gn);
			}
		}

		Buffer geometryNodeBuffer = Buffer(device,
			sizeof(GeometryNode) * static_cast<uint32_t>(geometryNodes.size()),
			VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		geometryNodeBuffer.fillBuffer(device, 
			geometryNodes.data(), 
			sizeof(GeometryNode) * static_cast<uint32_t>(geometryNodes.size()));


		geometryNodeBuffers.push_back(geometryNodeBuffer);

		for (auto& buildRangeInfo : buildRangeInfos) {
			pbuildRangeInfos.push_back(&buildRangeInfo);
		}

		VkAccelerationStructureBuildGeometryInfoKHR accelerationStructureBuildGeometryInfo{};
		accelerationStructureBuildGeometryInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
		accelerationStructureBuildGeometryInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
		accelerationStructureBuildGeometryInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
		accelerationStructureBuildGeometryInfo.geometryCount = static_cast<uint32_t>(geometries.size());
		accelerationStructureBuildGeometryInfo.pGeometries = geometries.data();

		VkAccelerationStructureBuildSizesInfoKHR accelerationStructureBuildSizesInfo{};
		accelerationStructureBuildSizesInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR; 
		vkGetAccelerationStructureBuildSizesKHR(device,
			VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
			&accelerationStructureBuildGeometryInfo,
			maxPrimitiveCounts.data(),
			&accelerationStructureBuildSizesInfo);

		BLASs[i].buffer = Buffer(device, accelerationStructureBuildSizesInfo.accelerationStructureSize,
			VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

		VkAccelerationStructureCreateInfoKHR accelerationStructureCreateInfo{};
		accelerationStructureCreateInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
		accelerationStructureCreateInfo.buffer = BLASs[i].buffer;
		accelerationStructureCreateInfo.size = accelerationStructureBuildSizesInfo.accelerationStructureSize;
		accelerationStructureCreateInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
		vkCreateAccelerationStructureKHR(device, &accelerationStructureCreateInfo, nullptr, &BLASs[i].handle);

		Buffer scratchBuffer = Buffer(device,
			accelerationStructureBuildSizesInfo.buildScratchSize,
			VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

		VkAccelerationStructureBuildGeometryInfoKHR accelerationBuildGeometryInfo{};
		accelerationBuildGeometryInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
		accelerationBuildGeometryInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
		accelerationBuildGeometryInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
		accelerationBuildGeometryInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
		accelerationBuildGeometryInfo.dstAccelerationStructure = BLASs[i].handle;
		accelerationBuildGeometryInfo.geometryCount = static_cast<uint32_t>(geometries.size());
		accelerationBuildGeometryInfo.pGeometries = geometries.data();
		accelerationBuildGeometryInfo.scratchData.deviceAddress = getBufferDeviceAddress(device, scratchBuffer);


		

		VkCommandBuffer commandBuffer = beginSingleTimeCommands(device);
		vkCmdBuildAccelerationStructuresKHR(commandBuffer,
			1, 
			&accelerationBuildGeometryInfo, 
			pbuildRangeInfos.data());
		endSingleTimeCommands(device, commandBuffer);

		VkAccelerationStructureDeviceAddressInfoKHR accelerationDeviceAddressInfo{};
		accelerationDeviceAddressInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
		accelerationDeviceAddressInfo.accelerationStructure = BLASs[i].handle;
		BLASs[i].deviceAddress = vkGetAccelerationStructureDeviceAddressKHR(device, &accelerationDeviceAddressInfo);

		scratchBuffer.destroy(device);
	}
}

void RayTracing::createSBT(Device& device)
{
	const uint32_t handleSize = rayTracingPipelineProperties.shaderGroupHandleSize;
	
	const uint32_t handleSizeAligned = SBTalignedSize(rayTracingPipelineProperties.shaderGroupHandleSize, rayTracingPipelineProperties.shaderGroupHandleAlignment);

	const uint32_t groupCount = static_cast<uint32_t>(shaderGroups.size());

	const uint32_t sbtSize = groupCount * handleSizeAligned;

	std::vector<uint8_t> shaderHandleStorage(sbtSize);
	vkGetRayTracingShaderGroupHandlesKHR(device, pipeline, 0, groupCount, sbtSize, shaderHandleStorage.data());

	const VkBufferUsageFlags bufferUsageFlags = VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
	const VkMemoryPropertyFlags memoryUsageFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
	raygenShaderBindingTable = Buffer(device, handleSize, bufferUsageFlags, memoryUsageFlags);
	missShaderBindingTable = Buffer(device, handleSize, bufferUsageFlags, memoryUsageFlags);
	hitShaderBindingTable = Buffer(device, handleSize, bufferUsageFlags, memoryUsageFlags);


	raygenShaderBindingTable.map(device);
	missShaderBindingTable.map(device);
	hitShaderBindingTable.map(device);
	memcpy(raygenShaderBindingTable.mapped, shaderHandleStorage.data(), handleSize);
	memcpy(missShaderBindingTable.mapped, shaderHandleStorage.data() + handleSizeAligned, handleSize);
	memcpy(hitShaderBindingTable.mapped, shaderHandleStorage.data() + handleSizeAligned * 2, handleSize);
}

void RayTracing::createRTPipeline(Device& device)
{
	descriptorSetLayout = DescriptorSetLayout(device, DescriptorType::RayTracing);

	VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{};
	pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutCreateInfo.setLayoutCount = 1;
	pipelineLayoutCreateInfo.pSetLayouts = &descriptorSetLayout;
	vkCreatePipelineLayout(device, &pipelineLayoutCreateInfo, nullptr, &pipelineLayout);

	std::vector<VkPipelineShaderStageCreateInfo> shaderStages;

	Shader rayGenShader = Shader(device, "shaders/raygen.rgen.spv");
	Shader missShader = Shader(device, "shaders/miss.rmiss.spv");
	Shader hitShader = Shader(device, "shaders/hit.rchit.spv");
	
	//RayGen
	VkPipelineShaderStageCreateInfo rayGenShaderStage{};
	rayGenShaderStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	rayGenShaderStage.stage = VK_SHADER_STAGE_RAYGEN_BIT_KHR;
	rayGenShaderStage.module = rayGenShader;
	rayGenShaderStage.pName = "main";
	shaderStages.push_back(rayGenShaderStage);

	VkRayTracingShaderGroupCreateInfoKHR rayGenShaderGroup{};
	rayGenShaderGroup.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
	rayGenShaderGroup.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
	rayGenShaderGroup.generalShader = static_cast<uint32_t>(shaderStages.size()-1);
	rayGenShaderGroup.closestHitShader = VK_SHADER_UNUSED_KHR;
	rayGenShaderGroup.anyHitShader = VK_SHADER_UNUSED_KHR;
	rayGenShaderGroup.intersectionShader = VK_SHADER_UNUSED_KHR;
	shaderGroups.push_back(rayGenShaderGroup);
	//Miss

	VkPipelineShaderStageCreateInfo missShaderStage{};
	missShaderStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	missShaderStage.stage = VK_SHADER_STAGE_MISS_BIT_KHR;
	missShaderStage.module = missShader;
	missShaderStage.pName = "main";
	shaderStages.push_back(missShaderStage);

	VkRayTracingShaderGroupCreateInfoKHR missShaderGroup{};
	missShaderGroup.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
	missShaderGroup.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
	missShaderGroup.generalShader = static_cast<uint32_t>(shaderStages.size() - 1);
	missShaderGroup.closestHitShader = VK_SHADER_UNUSED_KHR;
	missShaderGroup.anyHitShader = VK_SHADER_UNUSED_KHR;
	missShaderGroup.intersectionShader = VK_SHADER_UNUSED_KHR;
	shaderGroups.push_back(missShaderGroup);

	//Hit

	VkPipelineShaderStageCreateInfo hitShaderStage{};
	hitShaderStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	hitShaderStage.stage = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
	hitShaderStage.module = hitShader;
	hitShaderStage.pName = "main";
	shaderStages.push_back(hitShaderStage);

	VkRayTracingShaderGroupCreateInfoKHR hitShaderGroup{};
	hitShaderGroup.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
	hitShaderGroup.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR;
	hitShaderGroup.generalShader = VK_SHADER_UNUSED_KHR;
	hitShaderGroup.closestHitShader = static_cast<uint32_t>(shaderStages.size()-1);
	hitShaderGroup.anyHitShader = VK_SHADER_UNUSED_KHR;
	hitShaderGroup.intersectionShader = VK_SHADER_UNUSED_KHR;
	shaderGroups.push_back(hitShaderGroup);

	VkRayTracingPipelineCreateInfoKHR rayTracingPipelineCreateInfo{};
	rayTracingPipelineCreateInfo.sType = VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR;
	rayTracingPipelineCreateInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
	rayTracingPipelineCreateInfo.pStages = shaderStages.data();
	rayTracingPipelineCreateInfo.groupCount = static_cast<uint32_t>(shaderGroups.size());
	rayTracingPipelineCreateInfo.pGroups = shaderGroups.data();
	rayTracingPipelineCreateInfo.maxPipelineRayRecursionDepth = 1;
	rayTracingPipelineCreateInfo.layout = pipelineLayout;
	vkCreateRayTracingPipelinesKHR(device, VK_NULL_HANDLE, VK_NULL_HANDLE,
		1, &rayTracingPipelineCreateInfo,
		nullptr, &pipeline);

	rayGenShader.destroy(device);
	missShader.destroy(device);
	hitShader.destroy(device);

}

void RayTracing::createOutputImages(Device& device)
{
	outputImages.resize(MAX_FRAMES_IN_FLIGHT);
	for (auto& image : outputImages) {
		image = ImageSet(device, _swapChain->GetExtent().width, _swapChain->GetExtent().height,
			1, _swapChain->GetImageFormat(),
			VK_IMAGE_TILING_OPTIMAL,
			VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_STORAGE_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			VK_IMAGE_ASPECT_COLOR_BIT);

		VkCommandBuffer cmdBuf = beginSingleTimeCommands(device);
		image.image.transitionLayout(device, cmdBuf, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_ASPECT_COLOR_BIT);
		endSingleTimeCommands(device, cmdBuf);
	}
}

void RayTracing::createDescriptorSets(Device& device,std::vector<Buffer>& uboBuffers, Scene& scene)
{
	std::vector<VkDescriptorPoolSize> poolSizes = {
		{VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR,1},
		{VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,1},
		{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,1},
		{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,1},
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,4 },
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,15 }
	};

	VkDescriptorPoolCreateInfo descriptorPoolCreateInfo{};
	descriptorPoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	descriptorPoolCreateInfo.maxSets = 2;
	descriptorPoolCreateInfo.pPoolSizes = poolSizes.data();
	descriptorPoolCreateInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());

	vkCreateDescriptorPool(device, &descriptorPoolCreateInfo, nullptr, &descriptorPool);

	descriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {


		descriptorSets[i] = DescriptorSet(device, descriptorPool, descriptorSetLayout);

		VkWriteDescriptorSetAccelerationStructureKHR descriptorAccelerationStructureInfo{};
		descriptorAccelerationStructureInfo.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR;
		descriptorAccelerationStructureInfo.accelerationStructureCount = 1;
		descriptorAccelerationStructureInfo.pAccelerationStructures = &TLASs[i].handle;

		VkWriteDescriptorSet accelerationStructureWrite{};
		accelerationStructureWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		// The specialized acceleration structure descriptor has to be chained
		accelerationStructureWrite.pNext = &descriptorAccelerationStructureInfo;
		accelerationStructureWrite.dstSet = descriptorSets[i];
		accelerationStructureWrite.dstBinding = 0;
		accelerationStructureWrite.descriptorCount = 1;
		accelerationStructureWrite.descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;

		VkDescriptorImageInfo storageImageDescriptor{};
		storageImageDescriptor.imageView = outputImages[i];
		storageImageDescriptor.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

		VkDescriptorBufferInfo bufferInfo = uboBuffers[i].GetBufferInfo();

		VkWriteDescriptorSet resultImageWrite{};
		resultImageWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		resultImageWrite.dstSet = descriptorSets[i];
		resultImageWrite.dstBinding = 1;
		resultImageWrite.dstArrayElement = 0;
		resultImageWrite.descriptorCount = 1;
		resultImageWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		resultImageWrite.pImageInfo = &storageImageDescriptor;

		
		VkWriteDescriptorSet uniformBufferWrite{};
		uniformBufferWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		uniformBufferWrite.dstSet = descriptorSets[i];
		uniformBufferWrite.dstBinding = 2;
		uniformBufferWrite.dstArrayElement = 0;
		uniformBufferWrite.descriptorCount = 1;
		uniformBufferWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		uniformBufferWrite.pBufferInfo = &bufferInfo;


		std::vector<VkDescriptorImageInfo> imageInfos(4);
		for (int i = 0; i < 3; i++) {
			auto& imageInfo = imageInfos[i];
			imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			imageInfo.imageView = scene.skybox.material.Get(i);
			imageInfo.sampler = Sampler::Get(SamplerMipMapType::High);
		}
		VkWriteDescriptorSet descriptorWriteForMap{};
		descriptorWriteForMap.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWriteForMap.dstSet = descriptorSets[i];
		descriptorWriteForMap.dstBinding = 3;
		descriptorWriteForMap.dstArrayElement = 0;
		descriptorWriteForMap.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		descriptorWriteForMap.descriptorCount = 3;
		descriptorWriteForMap.pImageInfo = imageInfos.data();


		VkDescriptorImageInfo lutImageInfo{};
		lutImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		lutImageInfo.imageView = scene.skybox.material.Get(3);
		lutImageInfo.sampler = Sampler::Get(SamplerMipMapType::High);
		imageInfos[3] = lutImageInfo;
		VkWriteDescriptorSet descriptorWriteForLut{};
		descriptorWriteForLut.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWriteForLut.dstSet = descriptorSets[i];
		descriptorWriteForLut.dstBinding = 4;
		descriptorWriteForLut.dstArrayElement = 0;
		descriptorWriteForLut.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		descriptorWriteForLut.descriptorCount = 1;
		descriptorWriteForLut.pImageInfo = &lutImageInfo;




		VkDescriptorBufferInfo gnInfo = geometryNodeBuffers[i].GetBufferInfo();

		VkWriteDescriptorSet descriptorWriteForNode{};
		descriptorWriteForNode.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWriteForNode.dstSet = descriptorSets[i];
		descriptorWriteForNode.dstBinding = 5;
		descriptorWriteForNode.dstArrayElement = 0;
		descriptorWriteForNode.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		descriptorWriteForNode.descriptorCount = 1;
		descriptorWriteForNode.pBufferInfo = &gnInfo;

		auto materialCnt = static_cast<uint32_t>(MaterialComponent::END);
		std::vector<VkDescriptorImageInfo> materialInfos(scene.models.size() * materialCnt);
		for (int j = 0; j < scene.models.size();j++) {
			auto& model = scene.models[j];
			for (int i = 0; i < materialCnt;i++) {
				VkDescriptorImageInfo imageInfo{};
				imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
				imageInfo.imageView = model.material.Get(i);
				imageInfo.sampler = Sampler::Get(SamplerMipMapType::High);
				if (!model.material.hasComponent(i)) {
					imageInfo.imageView = Material::dummy.imageView;
					imageInfo.sampler = Sampler::Get(SamplerMipMapType::Low);
				}

				materialInfos[j*materialCnt + i] = (imageInfo);
			}
		}


		VkWriteDescriptorSet descriptorWriteForMaterials{};
		descriptorWriteForMaterials.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWriteForMaterials.dstSet = descriptorSets[i];
		descriptorWriteForMaterials.dstBinding = 6;
		descriptorWriteForMaterials.dstArrayElement = 0;
		descriptorWriteForMaterials.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		descriptorWriteForMaterials.descriptorCount = static_cast<uint32_t>(materialInfos.size());
		descriptorWriteForMaterials.pImageInfo = materialInfos.data();








		std::vector<VkWriteDescriptorSet> writeDescriptorSets = {
				accelerationStructureWrite,
				resultImageWrite,
				uniformBufferWrite,
				descriptorWriteForMap,
				descriptorWriteForLut,
				descriptorWriteForNode,
				descriptorWriteForMaterials
		};
		vkUpdateDescriptorSets(device, static_cast<uint32_t>(writeDescriptorSets.size()), writeDescriptorSets.data(), 0, VK_NULL_HANDLE);
	}
}

void RayTracing::destroy(Device& device)
{
	for (auto& image : outputImages) {
		image.destroy(device);
	}
	vkDestroyPipeline(device, pipeline, nullptr);
	vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
	vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
	vkDestroyDescriptorPool(device, descriptorPool, nullptr);
	for (auto& blas : BLASs) {
		vkDestroyBuffer(device, blas.buffer, nullptr);
		vkFreeMemory(device, blas.buffer.GetMemory(), nullptr);
		vkDestroyAccelerationStructureKHR(device, blas.handle, nullptr);
	}

	for (auto& tlas : TLASs) {
		vkDestroyBuffer(device, tlas.buffer, nullptr);
		vkFreeMemory(device, tlas.buffer.GetMemory(), nullptr);
		vkDestroyAccelerationStructureKHR(device, tlas.handle, nullptr);
	}

	for (auto& buffer : geometryNodeBuffers) {
		buffer.destroy(device);
	}

	raygenShaderBindingTable.destroy(device);
	missShaderBindingTable.destroy(device);
	hitShaderBindingTable.destroy(device);
}

void RayTracing::recordCommandBuffer(Device& device, VkCommandBuffer commandBuffer,int currentFrame,uint32_t imageIndex)
{


	const uint32_t handleSizeAligned = SBTalignedSize(rayTracingPipelineProperties.shaderGroupHandleSize, rayTracingPipelineProperties.shaderGroupHandleAlignment);

	VkStridedDeviceAddressRegionKHR raygenShaderSbtEntry{};
	raygenShaderSbtEntry.deviceAddress = getBufferDeviceAddress(device,raygenShaderBindingTable);
	raygenShaderSbtEntry.stride = handleSizeAligned;
	raygenShaderSbtEntry.size = handleSizeAligned;

	VkStridedDeviceAddressRegionKHR missShaderSbtEntry{};
	missShaderSbtEntry.deviceAddress = getBufferDeviceAddress(device,missShaderBindingTable);
	missShaderSbtEntry.stride = handleSizeAligned;
	missShaderSbtEntry.size = handleSizeAligned;

	VkStridedDeviceAddressRegionKHR hitShaderSbtEntry{};
	hitShaderSbtEntry.deviceAddress = getBufferDeviceAddress(device, hitShaderBindingTable);
	hitShaderSbtEntry.stride = handleSizeAligned;
	hitShaderSbtEntry.size = handleSizeAligned;

	VkStridedDeviceAddressRegionKHR callableShaderSbtEntry{};
	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, pipeline);
	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, pipelineLayout, 0, 1, &descriptorSets[currentFrame], 0, 0);

	vkCmdTraceRaysKHR(
		commandBuffer,
		&raygenShaderSbtEntry,
		&missShaderSbtEntry,
		&hitShaderSbtEntry,
		&callableShaderSbtEntry,
		_swapChain->GetExtent().width,
		_swapChain->GetExtent().height,
		1);

	auto& outputSwapChain = _swapChain->GetImages()[imageIndex].image;
	auto& outputImage = outputImages[currentFrame].image;
	outputSwapChain.transitionLayout(device, commandBuffer,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
	outputImage.transitionLayout(device, commandBuffer, 
		VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);


	VkImageCopy copyRegion{};
	copyRegion.srcSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
	copyRegion.srcOffset = { 0, 0, 0 };
	copyRegion.dstSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
	copyRegion.dstOffset = { 0, 0, 0 };
	copyRegion.extent = { _swapChain->GetExtent().width, _swapChain->GetExtent().height, 1};
	vkCmdCopyImage(commandBuffer, 
		outputImage, 
		VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, 
		outputSwapChain,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 
		1, &copyRegion);


	outputSwapChain.transitionLayout(device, commandBuffer,
		VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
	outputImage.transitionLayout(device, commandBuffer,
		VK_IMAGE_LAYOUT_GENERAL);

}

void RayTracing::loadFunctions(Device& device)
{
	vkGetBufferDeviceAddressKHR = reinterpret_cast<PFN_vkGetBufferDeviceAddressKHR>(vkGetDeviceProcAddr(device, "vkGetBufferDeviceAddressKHR"));
	vkCmdBuildAccelerationStructuresKHR = reinterpret_cast<PFN_vkCmdBuildAccelerationStructuresKHR>(vkGetDeviceProcAddr(device, "vkCmdBuildAccelerationStructuresKHR"));
	vkBuildAccelerationStructuresKHR = reinterpret_cast<PFN_vkBuildAccelerationStructuresKHR>(vkGetDeviceProcAddr(device, "vkBuildAccelerationStructuresKHR"));
	vkCreateAccelerationStructureKHR = reinterpret_cast<PFN_vkCreateAccelerationStructureKHR>(vkGetDeviceProcAddr(device, "vkCreateAccelerationStructureKHR"));
	vkDestroyAccelerationStructureKHR = reinterpret_cast<PFN_vkDestroyAccelerationStructureKHR>(vkGetDeviceProcAddr(device, "vkDestroyAccelerationStructureKHR"));
	vkGetAccelerationStructureBuildSizesKHR = reinterpret_cast<PFN_vkGetAccelerationStructureBuildSizesKHR>(vkGetDeviceProcAddr(device, "vkGetAccelerationStructureBuildSizesKHR"));
	vkGetAccelerationStructureDeviceAddressKHR = reinterpret_cast<PFN_vkGetAccelerationStructureDeviceAddressKHR>(vkGetDeviceProcAddr(device, "vkGetAccelerationStructureDeviceAddressKHR"));
	vkCmdTraceRaysKHR = reinterpret_cast<PFN_vkCmdTraceRaysKHR>(vkGetDeviceProcAddr(device, "vkCmdTraceRaysKHR"));
	vkGetRayTracingShaderGroupHandlesKHR = reinterpret_cast<PFN_vkGetRayTracingShaderGroupHandlesKHR>(vkGetDeviceProcAddr(device, "vkGetRayTracingShaderGroupHandlesKHR"));
	vkCreateRayTracingPipelinesKHR = reinterpret_cast<PFN_vkCreateRayTracingPipelinesKHR>(vkGetDeviceProcAddr(device, "vkCreateRayTracingPipelinesKHR"));

}

uint64_t RayTracing::getBufferDeviceAddress(Device& device, Buffer& buffer)
{
	VkBufferDeviceAddressInfoKHR bufferDeviceAI{};
	bufferDeviceAI.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
	bufferDeviceAI.buffer = buffer;
	return vkGetBufferDeviceAddressKHR(device, &bufferDeviceAI);
}
