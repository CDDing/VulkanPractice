#include "pch.h"
#include "RayTracing.h"


void RayTracing::init(std::shared_ptr<Device> device, std::vector<Buffer>& uboBuffers, SwapChain& swapChain,Scene& scene, std::vector<Buffer>& guiBuffers)
{
	_device = device;
	_swapChain = &swapChain;
	auto deviceProperties2 = device->physical.getProperties2<vk::PhysicalDeviceProperties2, vk::PhysicalDeviceRayTracingPipelinePropertiesKHR>();
	rayTracingPipelineProperties = deviceProperties2.get<vk::PhysicalDeviceRayTracingPipelinePropertiesKHR>();
	loadFunctions();

	createOutputImages();
	BLASs.resize(MAX_FRAMES_IN_FLIGHT);
	TLASs.resize(MAX_FRAMES_IN_FLIGHT);
	createBlas(scene.models);
	createTlas(scene.models);
	createRTPipeline();
	createSBT();
	createDescriptorSets(uboBuffers,guiBuffers,scene);
}

void RayTracing::createTlas(std::vector<Model>& models)
{

	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		std::vector<VkAccelerationStructureInstanceKHR> instances;
		for (int j = 0; j < models.size(); j++) {
			VkTransformMatrixKHR transform;
			memcpy(&transform, &models[j].transform, sizeof(VkTransformMatrixKHR));
			VkAccelerationStructureInstanceKHR instance{};
			instance.transform = transform;
			instance.instanceCustomIndex = j;
			instance.mask = 0xFF;
			instance.instanceShaderBindingTableRecordOffset = 0;
			instance.flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;
			instance.accelerationStructureReference = BLASs[i][j].deviceAddress;
			instances.push_back(instance);
		}
		Buffer instanceBuffer = Buffer(_device,
			sizeof(VkAccelerationStructureInstanceKHR) * instances.size(),
			vk::BufferUsageFlagBits::eShaderDeviceAddress| vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR,
			vk::MemoryPropertyFlagBits::eHostVisible| vk::MemoryPropertyFlagBits::eHostCoherent);
		instanceBuffer.map(_device, sizeof(VkAccelerationStructureInstanceKHR), 0);
		memcpy(instanceBuffer.mapped, instances.data(), sizeof(VkAccelerationStructureInstanceKHR) * instances.size());
		instanceBuffer.unmap(_device);

		vk::DeviceOrHostAddressConstKHR instanceDataDeviceAddress{};
		instanceDataDeviceAddress.deviceAddress = getBufferDeviceAddress(instanceBuffer);

		vk::AccelerationStructureGeometryInstancesDataKHR instancesData;
		instancesData.setArrayOfPointers(false);
		instancesData.setData(instanceDataDeviceAddress);

		vk::AccelerationStructureGeometryKHR accelerationStructureGeometry{};
		accelerationStructureGeometry.flags = vk::GeometryFlagBitsKHR::eOpaque;
		accelerationStructureGeometry.geometryType = vk::GeometryTypeKHR::eInstances;
		accelerationStructureGeometry.geometry.instances = instancesData;


		vk::AccelerationStructureBuildGeometryInfoKHR accelerationStructureBuildGeometryInfo{};
		accelerationStructureBuildGeometryInfo.flags = vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace;
		accelerationStructureBuildGeometryInfo.geometryCount = 1;
		accelerationStructureBuildGeometryInfo.pGeometries = &accelerationStructureGeometry;

		uint32_t primitive_count = models.size();

		vk::AccelerationStructureBuildSizesInfoKHR accelerationStructureBuildSizesInfo=_device->logical.getAccelerationStructureBuildSizesKHR(vk::AccelerationStructureBuildTypeKHR::eDevice, accelerationStructureBuildGeometryInfo, primitive_count);
		
		TLASs[i].buffer = Buffer(_device, accelerationStructureBuildSizesInfo.accelerationStructureSize,
			vk::BufferUsageFlagBits::eAccelerationStructureStorageKHR| vk::BufferUsageFlagBits::eShaderDeviceAddress,
			vk::MemoryPropertyFlagBits::eDeviceLocal);

		vk::AccelerationStructureCreateInfoKHR accelerationStructureCreateInfo{};
		accelerationStructureCreateInfo.buffer = TLASs[i].buffer;
		accelerationStructureCreateInfo.size = accelerationStructureBuildSizesInfo.accelerationStructureSize;
		accelerationStructureCreateInfo.type = vk::AccelerationStructureTypeKHR::eTopLevel;
		TLASs[i].handle = _device->logical.createAccelerationStructureKHR(accelerationStructureCreateInfo);

		Buffer scratchBuffer = Buffer(_device, accelerationStructureBuildSizesInfo.buildScratchSize,
			vk::BufferUsageFlagBits::eStorageBuffer| vk::BufferUsageFlagBits::eShaderDeviceAddress,
			vk::MemoryPropertyFlagBits::eDeviceLocal
		);

		vk::AccelerationStructureBuildGeometryInfoKHR accelerationBuildGeometryInfo{};
		accelerationBuildGeometryInfo.type = vk::AccelerationStructureTypeKHR::eTopLevel;
		accelerationBuildGeometryInfo.flags = vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace;
		accelerationBuildGeometryInfo.mode = vk::BuildAccelerationStructureModeKHR::eBuild;
		accelerationBuildGeometryInfo.dstAccelerationStructure = TLASs[i].handle;
		accelerationBuildGeometryInfo.geometryCount = 1;
		accelerationBuildGeometryInfo.pGeometries = &accelerationStructureGeometry;
		accelerationBuildGeometryInfo.scratchData.deviceAddress = getBufferDeviceAddress(scratchBuffer);

		vk::AccelerationStructureBuildRangeInfoKHR accelerationStructureBuildRangeInfo{};
		accelerationStructureBuildRangeInfo.primitiveCount = models.size();
		accelerationStructureBuildRangeInfo.primitiveOffset = 0;
		accelerationStructureBuildRangeInfo.firstVertex = 0;
		accelerationStructureBuildRangeInfo.transformOffset = 0;
		std::vector<vk::AccelerationStructureBuildRangeInfoKHR*> accelerationBuildStructureRangeInfos = { &accelerationStructureBuildRangeInfo };

		vk::CommandBuffer commandBuffer = beginSingleTimeCommands(_device);
		commandBuffer.buildAccelerationStructuresKHR(accelerationBuildGeometryInfo, accelerationBuildStructureRangeInfos);
		endSingleTimeCommands(_device, commandBuffer);

		vk::AccelerationStructureDeviceAddressInfoKHR accelerationDeviceAddressInfo{TLASs[i].handle};
		TLASs[i].deviceAddress = _device->logical.getAccelerationStructureAddressKHR(accelerationDeviceAddressInfo);
		scratchBuffer.destroy(_device);
		instanceBuffer.destroy(_device);
	}
}

void RayTracing::createBlas(std::vector<Model>& models)
{
	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		vk::AccelerationStructureGeometryKHR accelerationGeometry{};
		accelerationGeometry.flags = vk::GeometryFlagBitsKHR::eOpaque;
		accelerationGeometry.geometryType = vk::GeometryTypeKHR::eTriangles;
		

		std::vector<GeometryNode> geometryNodes;
		BLASs[i].resize(models.size());
		for (int j = 0; j < 3;j++) {
			std::vector<vk::AccelerationStructureGeometryKHR> geometries;
			std::vector<uint32_t> maxPrimitiveCounts;
			std::vector<vk::AccelerationStructureBuildRangeInfoKHR> buildRangeInfos;
			std::vector<vk::AccelerationStructureBuildRangeInfoKHR*> pbuildRangeInfos;
			auto& model = models[j];
			for (auto& mesh : model.meshes) {
				vk::DeviceOrHostAddressConstKHR vertexDeviceAddress{};
				vk::DeviceOrHostAddressConstKHR indexDeviceAddress{};
				vk::DeviceOrHostAddressConstKHR transformDeviceAddress{};
				vertexDeviceAddress.deviceAddress = getBufferDeviceAddress(mesh.vertexBuffer);
				indexDeviceAddress.deviceAddress = getBufferDeviceAddress(mesh.indexBuffer);
				transformDeviceAddress.deviceAddress = getBufferDeviceAddress(model.uniformBuffers[i]);

				vk::AccelerationStructureGeometryTrianglesDataKHR triangles{};
				triangles.vertexFormat = vk::Format::eR32G32B32Sfloat;
				triangles.vertexData = vertexDeviceAddress;
				triangles.maxVertex = mesh.vertices.size();
				triangles.vertexStride = sizeof(Vertex);

				triangles.indexType = vk::IndexType::eUint32;
				triangles.indexData = indexDeviceAddress;

				triangles.transformData = {};

				accelerationGeometry.geometry.triangles = triangles;

				geometries.push_back(accelerationGeometry);

				auto numTriangles = static_cast<uint32_t>(mesh.indices.size() / 3);
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


			for (auto& buildRangeInfo : buildRangeInfos) {
				pbuildRangeInfos.push_back(&buildRangeInfo);
			}

			vk::AccelerationStructureBuildGeometryInfoKHR accelerationStructureBuildGeometryInfo{};
			accelerationStructureBuildGeometryInfo.type = vk::AccelerationStructureTypeKHR::eBottomLevel;
			accelerationStructureBuildGeometryInfo.flags = vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace;
			accelerationStructureBuildGeometryInfo.geometryCount = static_cast<uint32_t>(geometries.size());
			accelerationStructureBuildGeometryInfo.pGeometries = geometries.data();

			vk::AccelerationStructureBuildSizesInfoKHR accelerationStructureBuildSizesInfo =	
				_device->logical.getAccelerationStructureBuildSizesKHR(
				vk::AccelerationStructureBuildTypeKHR::eDevice,
				accelerationStructureBuildGeometryInfo,
				maxPrimitiveCounts);
			
			BLASs[i][j].buffer = Buffer(_device, accelerationStructureBuildSizesInfo.accelerationStructureSize,
				vk::BufferUsageFlagBits::eAccelerationStructureStorageKHR| vk::BufferUsageFlagBits::eShaderDeviceAddress | vk::BufferUsageFlagBits::eShaderDeviceAddress,
				vk::MemoryPropertyFlagBits::eDeviceLocal);

			vk::AccelerationStructureCreateInfoKHR accelerationStructureCreateInfo{};
			accelerationStructureCreateInfo.buffer = BLASs[i][j].buffer;
			accelerationStructureCreateInfo.size = accelerationStructureBuildSizesInfo.accelerationStructureSize;
			accelerationStructureCreateInfo.type = vk::AccelerationStructureTypeKHR::eBottomLevel;
			BLASs[i][j].handle = _device->logical.createAccelerationStructureKHR(accelerationStructureCreateInfo);

			Buffer scratchBuffer = Buffer(_device,
				accelerationStructureBuildSizesInfo.buildScratchSize,
				vk::BufferUsageFlagBits::eStorageBuffer| vk::BufferUsageFlagBits::eShaderDeviceAddress,
				vk::MemoryPropertyFlagBits::eDeviceLocal);

			vk::AccelerationStructureBuildGeometryInfoKHR accelerationBuildGeometryInfo{};
			accelerationBuildGeometryInfo.type = vk::AccelerationStructureTypeKHR::eBottomLevel;
			accelerationBuildGeometryInfo.flags = vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace;
			accelerationBuildGeometryInfo.mode = vk::BuildAccelerationStructureModeKHR::eBuild;
			accelerationBuildGeometryInfo.dstAccelerationStructure = BLASs[i][j].handle;
			accelerationBuildGeometryInfo.geometryCount = static_cast<uint32_t>(geometries.size());
			accelerationBuildGeometryInfo.pGeometries = geometries.data();
			accelerationBuildGeometryInfo.scratchData.deviceAddress = getBufferDeviceAddress(scratchBuffer);


			vk::CommandBuffer commandBuffer = beginSingleTimeCommands(_device);
			commandBuffer.buildAccelerationStructuresKHR(accelerationBuildGeometryInfo, pbuildRangeInfos);
			endSingleTimeCommands(_device, commandBuffer);

			vk::AccelerationStructureDeviceAddressInfoKHR accelerationDeviceAddressInfo{};
			accelerationDeviceAddressInfo.accelerationStructure = BLASs[i][j].handle;
			BLASs[i][j].deviceAddress = _device->logical.getAccelerationStructureAddressKHR(accelerationDeviceAddressInfo);
			scratchBuffer.destroy(_device);

		}


		Buffer geometryNodeBuffer = Buffer(_device,
			sizeof(GeometryNode) * static_cast<uint32_t>(geometryNodes.size()),
			vk::BufferUsageFlagBits::eShaderDeviceAddress| vk::BufferUsageFlagBits::eStorageBuffer| vk::BufferUsageFlagBits::eTransferDst,
			vk::MemoryPropertyFlagBits::eDeviceLocal);
		geometryNodeBuffer.fillBuffer(_device,
			geometryNodes.data(),
			sizeof(GeometryNode)* static_cast<uint32_t>(geometryNodes.size()));


		geometryNodeBuffers.push_back(geometryNodeBuffer);

	}
}

void RayTracing::createSBT()
{
	const uint32_t handleSize = rayTracingPipelineProperties.shaderGroupHandleSize;
	
	const uint32_t handleSizeAligned = SBTalignedSize(rayTracingPipelineProperties.shaderGroupHandleSize, rayTracingPipelineProperties.shaderGroupHandleAlignment);

	const uint32_t groupCount = static_cast<uint32_t>(shaderGroups.size());

	const uint32_t sbtSize = groupCount * handleSizeAligned;

	std::vector<uint8_t> shaderHandleStorage = _device->logical.getRayTracingShaderGroupHandlesKHR<uint8_t>(pipeline, 0, groupCount, sbtSize);
	
	const vk::BufferUsageFlags bufferUsageFlags = vk::BufferUsageFlagBits::eShaderBindingTableKHR| vk::BufferUsageFlagBits::eShaderDeviceAddress;
	const vk::MemoryPropertyFlags memoryUsageFlags = vk::MemoryPropertyFlagBits::eHostVisible| vk::MemoryPropertyFlagBits::eHostCoherent;
	raygenShaderBindingTable = Buffer(_device, handleSize, bufferUsageFlags, memoryUsageFlags);
	missShaderBindingTable = Buffer(_device, handleSize * 2, bufferUsageFlags, memoryUsageFlags);
	hitShaderBindingTable = Buffer(_device, handleSize, bufferUsageFlags, memoryUsageFlags);


	raygenShaderBindingTable.map(_device);
	missShaderBindingTable.map(_device);
	hitShaderBindingTable.map(_device);
	memcpy(raygenShaderBindingTable.mapped, shaderHandleStorage.data(), handleSize);
	memcpy(missShaderBindingTable.mapped, shaderHandleStorage.data() + handleSizeAligned, handleSize * 2);
	memcpy(hitShaderBindingTable.mapped, shaderHandleStorage.data() + handleSizeAligned * 3, handleSize);
}

void RayTracing::createRTPipeline()
{
	descriptorSetLayout = DescriptorSetLayout(*_device, DescriptorType::RayTracing);

	vk::PipelineLayoutCreateInfo pipelineLayoutCreateInfo{};
	pipelineLayoutCreateInfo.setLayoutCount = 1;
	pipelineLayoutCreateInfo.pSetLayouts = &descriptorSetLayout;
	pipelineLayout = _device->logical.createPipelineLayout(pipelineLayoutCreateInfo);
	
	std::vector<vk::PipelineShaderStageCreateInfo> shaderStages;

	Shader rayGenShader = Shader(*_device, "shaders/raygen.rgen.spv");
	Shader missShader = Shader(*_device, "shaders/miss.rmiss.spv");
	Shader hitShader = Shader(*_device, "shaders/hit.rchit.spv");
	Shader shadowShader = Shader(*_device, "shaders/shadow.rmiss.spv");

	//RayGen
	vk::PipelineShaderStageCreateInfo rayGenShaderStage{};
	rayGenShaderStage.stage = vk::ShaderStageFlagBits::eRaygenKHR;
	rayGenShaderStage.module = rayGenShader;
	rayGenShaderStage.pName = "main";
	shaderStages.push_back(rayGenShaderStage);

	vk::RayTracingShaderGroupCreateInfoKHR rayGenShaderGroup{};
	rayGenShaderGroup.type = vk::RayTracingShaderGroupTypeKHR::eGeneral;
	rayGenShaderGroup.generalShader = static_cast<uint32_t>(shaderStages.size()-1);
	rayGenShaderGroup.closestHitShader = VK_SHADER_UNUSED_KHR;
	rayGenShaderGroup.anyHitShader = VK_SHADER_UNUSED_KHR;
	rayGenShaderGroup.intersectionShader = VK_SHADER_UNUSED_KHR;
	shaderGroups.push_back(rayGenShaderGroup);
	//Miss

	vk::PipelineShaderStageCreateInfo missShaderStage{};
	missShaderStage.stage = vk::ShaderStageFlagBits::eMissKHR;
	missShaderStage.module = missShader;
	missShaderStage.pName = "main";
	shaderStages.push_back(missShaderStage);

	vk::RayTracingShaderGroupCreateInfoKHR missShaderGroup{};
	missShaderGroup.type = vk::RayTracingShaderGroupTypeKHR::eGeneral;
	missShaderGroup.generalShader = static_cast<uint32_t>(shaderStages.size() - 1);
	missShaderGroup.closestHitShader = VK_SHADER_UNUSED_KHR;
	missShaderGroup.anyHitShader = VK_SHADER_UNUSED_KHR;
	missShaderGroup.intersectionShader = VK_SHADER_UNUSED_KHR;
	shaderGroups.push_back(missShaderGroup);
	
	vk::PipelineShaderStageCreateInfo shadowShaderStage{};
	shadowShaderStage.stage = vk::ShaderStageFlagBits::eMissKHR;
	shadowShaderStage.module = shadowShader;
	shadowShaderStage.pName = "main";
	shaderStages.push_back(shadowShaderStage);
	missShaderGroup.generalShader = static_cast<uint32_t>(shaderStages.size() - 1);
	shaderGroups.push_back(missShaderGroup);

	//Hit

	vk::PipelineShaderStageCreateInfo hitShaderStage{};
	hitShaderStage.stage = vk::ShaderStageFlagBits::eClosestHitKHR;
	hitShaderStage.module = hitShader;
	hitShaderStage.pName = "main";
	shaderStages.push_back(hitShaderStage);

	vk::RayTracingShaderGroupCreateInfoKHR hitShaderGroup{};
	hitShaderGroup.type = vk::RayTracingShaderGroupTypeKHR::eTrianglesHitGroup;
	hitShaderGroup.generalShader = VK_SHADER_UNUSED_KHR;
	hitShaderGroup.closestHitShader = static_cast<uint32_t>(shaderStages.size()-1);
	hitShaderGroup.anyHitShader = VK_SHADER_UNUSED_KHR;
	hitShaderGroup.intersectionShader = VK_SHADER_UNUSED_KHR;
	shaderGroups.push_back(hitShaderGroup);


	vk::RayTracingPipelineCreateInfoKHR rayTracingPipelineCreateInfo{};
	rayTracingPipelineCreateInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
	rayTracingPipelineCreateInfo.pStages = shaderStages.data();
	rayTracingPipelineCreateInfo.groupCount = static_cast<uint32_t>(shaderGroups.size());
	rayTracingPipelineCreateInfo.pGroups = shaderGroups.data();
	rayTracingPipelineCreateInfo.maxPipelineRayRecursionDepth = 2;
	rayTracingPipelineCreateInfo.layout = pipelineLayout;

	vk::Result result;
	std::tie(result,pipeline) = _device->logical.createRayTracingPipelineKHR(VK_NULL_HANDLE, VK_NULL_HANDLE,
		rayTracingPipelineCreateInfo);
	
	rayGenShader.destroy(*_device);
	missShader.destroy(*_device);
	hitShader.destroy(*_device);
	shadowShader.destroy(*_device);
}

void RayTracing::createOutputImages()
{
	outputImages.resize(MAX_FRAMES_IN_FLIGHT);
	for (auto& image : outputImages) {
		image = ImageSet(_device, _swapChain->GetExtent().width, _swapChain->GetExtent().height,
			1, _swapChain->GetImageFormat(),
			vk::ImageTiling::eOptimal,
			vk::ImageUsageFlagBits::eTransferSrc| vk::ImageUsageFlagBits::eStorage,
			vk::MemoryPropertyFlagBits::eDeviceLocal,
			vk::ImageAspectFlagBits::eColor);

		VkCommandBuffer cmdBuf = beginSingleTimeCommands(_device);
		image.image.transitionLayout(_device, cmdBuf, vk::ImageLayout::eGeneral, vk::ImageAspectFlagBits::eColor);
		endSingleTimeCommands(_device, cmdBuf);
	}
}

void RayTracing::createDescriptorSets(std::vector<Buffer>& uboBuffers, std::vector<Buffer>& guiBuffers, Scene& scene)
{
	std::vector<VkDescriptorPoolSize> poolSizes = {
		{VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR,1},
		{VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,1},
		{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,1},
		{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,2},
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,4 },
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,15 }
	};

	VkDescriptorPoolCreateInfo descriptorPoolCreateInfo{};
	descriptorPoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	descriptorPoolCreateInfo.maxSets = 2;
	descriptorPoolCreateInfo.pPoolSizes = poolSizes.data();
	descriptorPoolCreateInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());

	descriptorPool = std::make_shared<DescriptorPool>();
	descriptorPool->_descriptorPool = _device->logical.createDescriptorPool(descriptorPoolCreateInfo);
	descriptorPool->_device = _device;
	descriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {


		descriptorSets[i] = DescriptorSet(*_device, *descriptorPool, descriptorSetLayout);

		VkWriteDescriptorSetAccelerationStructureKHR descriptorAccelerationStructureInfo{};
		descriptorAccelerationStructureInfo.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR;
		descriptorAccelerationStructureInfo.accelerationStructureCount = 1;
		descriptorAccelerationStructureInfo.pAccelerationStructures = &TLASs[i].handle;

		vk::WriteDescriptorSet accelerationStructureWrite{};
		accelerationStructureWrite.pNext = &descriptorAccelerationStructureInfo;
		accelerationStructureWrite.dstSet = descriptorSets[i];
		accelerationStructureWrite.dstBinding = 0;
		accelerationStructureWrite.descriptorCount = 1;
		accelerationStructureWrite.descriptorType = vk::DescriptorType::eAccelerationStructureKHR;

		vk::DescriptorImageInfo storageImageDescriptor{};
		storageImageDescriptor.imageView = outputImages[i];
		storageImageDescriptor.imageLayout = vk::ImageLayout::eGeneral;

		vk::DescriptorBufferInfo bufferInfo = uboBuffers[i].GetBufferInfo();

		vk::WriteDescriptorSet resultImageWrite{};
		resultImageWrite.dstSet = descriptorSets[i];
		resultImageWrite.dstBinding = 1;
		resultImageWrite.dstArrayElement = 0;
		resultImageWrite.descriptorCount = 1;
		resultImageWrite.descriptorType = vk::DescriptorType::eStorageImage;
		resultImageWrite.pImageInfo = &storageImageDescriptor;

		
		vk::WriteDescriptorSet uniformBufferWrite{};
		uniformBufferWrite.dstSet = descriptorSets[i];
		uniformBufferWrite.dstBinding = 2;
		uniformBufferWrite.dstArrayElement = 0;
		uniformBufferWrite.descriptorCount = 1;
		uniformBufferWrite.descriptorType = vk::DescriptorType::eUniformBuffer;
		uniformBufferWrite.pBufferInfo = &bufferInfo;


		std::vector<vk::DescriptorImageInfo> imageInfos(4);
		for (int i = 0; i < 3; i++) {
			auto& imageInfo = imageInfos[i];
			imageInfo.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
			imageInfo.imageView = scene.skybox.material.Get(i);
			imageInfo.sampler = Sampler::Get(SamplerMipMapType::High);
		}
		vk::WriteDescriptorSet descriptorWriteForMap{};
		descriptorWriteForMap.dstSet = descriptorSets[i];
		descriptorWriteForMap.dstBinding = 3;
		descriptorWriteForMap.dstArrayElement = 0;
		descriptorWriteForMap.descriptorType = vk::DescriptorType::eCombinedImageSampler;
		descriptorWriteForMap.descriptorCount = 3;
		descriptorWriteForMap.pImageInfo = imageInfos.data();


		vk::DescriptorImageInfo lutImageInfo{};
		lutImageInfo.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
		lutImageInfo.imageView = scene.skybox.material.Get(3);
		lutImageInfo.sampler = Sampler::Get(SamplerMipMapType::High);
		imageInfos[3] = lutImageInfo;
		vk::WriteDescriptorSet descriptorWriteForLut{};
		descriptorWriteForLut.dstSet = descriptorSets[i];
		descriptorWriteForLut.dstBinding = 4;
		descriptorWriteForLut.dstArrayElement = 0;
		descriptorWriteForLut.descriptorType = vk::DescriptorType::eCombinedImageSampler;
		descriptorWriteForLut.descriptorCount = 1;
		descriptorWriteForLut.pImageInfo = &lutImageInfo;




		vk::DescriptorBufferInfo gnInfo = geometryNodeBuffers[i].GetBufferInfo();

		vk::WriteDescriptorSet descriptorWriteForNode{};
		descriptorWriteForNode.dstSet = descriptorSets[i];
		descriptorWriteForNode.dstBinding = 5;
		descriptorWriteForNode.dstArrayElement = 0;
		descriptorWriteForNode.descriptorType = vk::DescriptorType::eStorageBuffer;
		descriptorWriteForNode.descriptorCount = 1;
		descriptorWriteForNode.pBufferInfo = &gnInfo;

		auto materialCnt = static_cast<uint32_t>(MaterialComponent::END);
		std::vector<vk::DescriptorImageInfo> materialInfos(scene.models.size() * materialCnt);
		for (int j = 0; j < scene.models.size();j++) {
			auto& model = scene.models[j];
			for (int k = 0; k < materialCnt;k++) {
				vk::DescriptorImageInfo imageInfo{};
				imageInfo.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
				imageInfo.imageView = model.material.Get(k);
				imageInfo.sampler = Sampler::Get(SamplerMipMapType::High);
				if (!model.material.hasComponent(k)) {
					imageInfo.imageView = Material::dummy.imageView;
					imageInfo.sampler = Sampler::Get(SamplerMipMapType::Low);
				}

				materialInfos[j*materialCnt + k] = (imageInfo);
			}
		}


		vk::WriteDescriptorSet descriptorWriteForMaterials{};
		descriptorWriteForMaterials.dstSet = descriptorSets[i];
		descriptorWriteForMaterials.dstBinding = 6;
		descriptorWriteForMaterials.dstArrayElement = 0;
		descriptorWriteForMaterials.descriptorType = vk::DescriptorType::eCombinedImageSampler;
		descriptorWriteForMaterials.descriptorCount = static_cast<uint32_t>(materialInfos.size());
		descriptorWriteForMaterials.pImageInfo = materialInfos.data();




		vk::DescriptorBufferInfo guiBufferInfo = guiBuffers[i].GetBufferInfo();

		vk::WriteDescriptorSet guiBufferWrite{};
		guiBufferWrite.dstSet = descriptorSets[i];
		guiBufferWrite.dstBinding = 7;
		guiBufferWrite.dstArrayElement = 0;
		guiBufferWrite.descriptorCount = 1;
		guiBufferWrite.descriptorType = vk::DescriptorType::eUniformBuffer;
		guiBufferWrite.pBufferInfo = &guiBufferInfo;




		std::vector<vk::WriteDescriptorSet> writeDescriptorSets = {
				accelerationStructureWrite,
				resultImageWrite,
				uniformBufferWrite,
				descriptorWriteForMap,
				descriptorWriteForLut,
				descriptorWriteForNode,
				descriptorWriteForMaterials,
				guiBufferWrite
		};
		_device->logical.updateDescriptorSets(writeDescriptorSets, nullptr);
	}
}

void RayTracing::destroy()
{
	for (auto& image : outputImages) {
		image.destroy(_device);
	}

	_device->logical.destroyPipeline(pipeline);
	_device->logical.destroyPipelineLayout(pipelineLayout);
	_device->logical.destroyDescriptorSetLayout(descriptorSetLayout);
	for (auto& blas : BLASs) {
		for (auto& blasPerModels : blas) {
			blasPerModels.buffer.destroy(_device);
			_device->logical.destroyAccelerationStructureKHR(blasPerModels.handle);
		}
	}

	for (auto& tlas : TLASs) {
		tlas.buffer.destroy(_device);
		_device->logical.destroyAccelerationStructureKHR(tlas.handle);
	}

	for (auto& buffer : geometryNodeBuffers) {
		buffer.destroy(_device);
	}

	raygenShaderBindingTable.destroy(_device);
	missShaderBindingTable.destroy(_device);
	hitShaderBindingTable.destroy(_device);
}

void RayTracing::recordCommandBuffer(vk::CommandBuffer commandBuffer,int currentFrame,uint32_t imageIndex)
{


	const uint32_t handleSizeAligned = SBTalignedSize(rayTracingPipelineProperties.shaderGroupHandleSize, rayTracingPipelineProperties.shaderGroupHandleAlignment);

	VkStridedDeviceAddressRegionKHR raygenShaderSbtEntry{};
	raygenShaderSbtEntry.deviceAddress = getBufferDeviceAddress(raygenShaderBindingTable);
	raygenShaderSbtEntry.stride = handleSizeAligned;
	raygenShaderSbtEntry.size = handleSizeAligned;

	VkStridedDeviceAddressRegionKHR missShaderSbtEntry{};
	missShaderSbtEntry.deviceAddress = getBufferDeviceAddress(missShaderBindingTable);
	missShaderSbtEntry.stride = handleSizeAligned;
	missShaderSbtEntry.size = handleSizeAligned * 2;

	VkStridedDeviceAddressRegionKHR hitShaderSbtEntry{};
	hitShaderSbtEntry.deviceAddress = getBufferDeviceAddress(hitShaderBindingTable);
	hitShaderSbtEntry.stride = handleSizeAligned;
	hitShaderSbtEntry.size = handleSizeAligned;

	VkStridedDeviceAddressRegionKHR callableShaderSbtEntry{};
	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, pipeline);
	commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eRayTracingKHR, pipelineLayout, 0, { descriptorSets[currentFrame] }, {});
	
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
	outputSwapChain.transitionLayout(_device, commandBuffer,
		vk::ImageLayout::eTransferDstOptimal);
	outputImage.transitionLayout(_device, commandBuffer,
		vk::ImageLayout::eTransferSrcOptimal);


	vk::ImageCopy copyRegion{};
	copyRegion.srcSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
	copyRegion.srcOffset = vk::Offset3D{ 0, 0, 0 };
	copyRegion.dstSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
	copyRegion.dstOffset = vk::Offset3D{ 0, 0, 0 };
	copyRegion.extent = vk::Extent3D{ _swapChain->GetExtent().width, _swapChain->GetExtent().height, 1};
	
	commandBuffer.copyImage(outputImage,
		vk::ImageLayout::eTransferSrcOptimal,
		outputSwapChain,
		vk::ImageLayout::eTransferDstOptimal,
		copyRegion);

	outputSwapChain.transitionLayout(_device, commandBuffer,
		vk::ImageLayout::ePresentSrcKHR);
	outputImage.transitionLayout(_device, commandBuffer,
		vk::ImageLayout::eGeneral);

}

void RayTracing::loadFunctions()
{
	vkGetBufferDeviceAddressKHR = reinterpret_cast<PFN_vkGetBufferDeviceAddressKHR>(vkGetDeviceProcAddr(_device->logical, "vkGetBufferDeviceAddressKHR"));
	vkCmdBuildAccelerationStructuresKHR = reinterpret_cast<PFN_vkCmdBuildAccelerationStructuresKHR>(vkGetDeviceProcAddr(_device->logical, "vkCmdBuildAccelerationStructuresKHR"));
	vkBuildAccelerationStructuresKHR = reinterpret_cast<PFN_vkBuildAccelerationStructuresKHR>(vkGetDeviceProcAddr(_device->logical, "vkBuildAccelerationStructuresKHR"));
	vkCreateAccelerationStructureKHR = reinterpret_cast<PFN_vkCreateAccelerationStructureKHR>(vkGetDeviceProcAddr(_device->logical, "vkCreateAccelerationStructureKHR"));
	vkDestroyAccelerationStructureKHR = reinterpret_cast<PFN_vkDestroyAccelerationStructureKHR>(vkGetDeviceProcAddr(_device->logical, "vkDestroyAccelerationStructureKHR"));
	vkGetAccelerationStructureBuildSizesKHR = reinterpret_cast<PFN_vkGetAccelerationStructureBuildSizesKHR>(vkGetDeviceProcAddr(_device->logical, "vkGetAccelerationStructureBuildSizesKHR"));
	vkGetAccelerationStructureDeviceAddressKHR = reinterpret_cast<PFN_vkGetAccelerationStructureDeviceAddressKHR>(vkGetDeviceProcAddr(_device->logical, "vkGetAccelerationStructureDeviceAddressKHR"));
	vkCmdTraceRaysKHR = reinterpret_cast<PFN_vkCmdTraceRaysKHR>(vkGetDeviceProcAddr(_device->logical, "vkCmdTraceRaysKHR"));
	vkGetRayTracingShaderGroupHandlesKHR = reinterpret_cast<PFN_vkGetRayTracingShaderGroupHandlesKHR>(vkGetDeviceProcAddr(_device->logical, "vkGetRayTracingShaderGroupHandlesKHR"));
	vkCreateRayTracingPipelinesKHR = reinterpret_cast<PFN_vkCreateRayTracingPipelinesKHR>(vkGetDeviceProcAddr(_device->logical, "vkCreateRayTracingPipelinesKHR"));

}

uint64_t RayTracing::getBufferDeviceAddress(Buffer& buffer)
{
	vk::BufferDeviceAddressInfoKHR bufferDeviceAI{};
	bufferDeviceAI.buffer = buffer;
	return _device->logical.getBufferAddressKHR(bufferDeviceAI);
}
