#include "pch.h"
#include "RayTracing.h"


RayTracing::RayTracing(DContext& context, SwapChain& swapChain, Scene& scene, std::vector<DBuffer>& uboBuffers, std::vector<DBuffer>& guiBuffers) :
	pipelineLayout(nullptr), pipeline(nullptr), descriptorPool(nullptr),raygenShaderBindingTable(nullptr), missShaderBindingTable(nullptr), hitShaderBindingTable(nullptr), descriptorSetLayout(nullptr)
{
	this->swapChain = &swapChain;
	auto deviceProperties2 = context.physical.getProperties2<vk::PhysicalDeviceProperties2, vk::PhysicalDeviceRayTracingPipelinePropertiesKHR>();
	rayTracingPipelineProperties = deviceProperties2.get<vk::PhysicalDeviceRayTracingPipelinePropertiesKHR>();
	loadFunctions(context);

	createOutputImages(context);
	createBlas(context, scene.models);
	createTlas(context, scene.models);
	createRTPipeline(context);
	createSBT(context);
	createDescriptorSets(context, scene, uboBuffers,guiBuffers);
}

void RayTracing::createTlas(DContext& context, std::vector<Model>& models)
{	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
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
		DBuffer instanceBuffer(context,
			sizeof(VkAccelerationStructureInstanceKHR) * instances.size(),
			vk::BufferUsageFlagBits::eShaderDeviceAddress| vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR,
			vk::MemoryPropertyFlagBits::eHostVisible| vk::MemoryPropertyFlagBits::eHostCoherent);
		instanceBuffer.map(sizeof(VkAccelerationStructureInstanceKHR), 0);
		memcpy(instanceBuffer.mapped, instances.data(), sizeof(VkAccelerationStructureInstanceKHR) * instances.size());
		instanceBuffer.unmap();

		vk::DeviceOrHostAddressConstKHR instanceDataDeviceAddress{};
		instanceDataDeviceAddress.deviceAddress = getBufferDeviceAddress(context,instanceBuffer.buffer);

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

		vk::AccelerationStructureBuildSizesInfoKHR accelerationStructureBuildSizesInfo = context.logical.getAccelerationStructureBuildSizesKHR(vk::AccelerationStructureBuildTypeKHR::eDevice, accelerationStructureBuildGeometryInfo, primitive_count);
		
		auto tlasBuffer = DBuffer(context, accelerationStructureBuildSizesInfo.accelerationStructureSize,
			vk::BufferUsageFlagBits::eAccelerationStructureStorageKHR| vk::BufferUsageFlagBits::eShaderDeviceAddress,
			vk::MemoryPropertyFlagBits::eDeviceLocal);

		vk::AccelerationStructureCreateInfoKHR accelerationStructureCreateInfo{};
		accelerationStructureCreateInfo.buffer = tlasBuffer.buffer;
		accelerationStructureCreateInfo.size = accelerationStructureBuildSizesInfo.accelerationStructureSize;
		accelerationStructureCreateInfo.type = vk::AccelerationStructureTypeKHR::eTopLevel;
		auto accelerationStructure= context.logical.createAccelerationStructureKHR(accelerationStructureCreateInfo);

		DBuffer scratchBuffer(context, accelerationStructureBuildSizesInfo.buildScratchSize,
			vk::BufferUsageFlagBits::eStorageBuffer| vk::BufferUsageFlagBits::eShaderDeviceAddress,
			vk::MemoryPropertyFlagBits::eDeviceLocal
		);

		vk::AccelerationStructureBuildGeometryInfoKHR accelerationBuildGeometryInfo{};
		accelerationBuildGeometryInfo.type = vk::AccelerationStructureTypeKHR::eTopLevel;
		accelerationBuildGeometryInfo.flags = vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace;
		accelerationBuildGeometryInfo.mode = vk::BuildAccelerationStructureModeKHR::eBuild;
		accelerationBuildGeometryInfo.dstAccelerationStructure = accelerationStructure;
		accelerationBuildGeometryInfo.geometryCount = 1;
		accelerationBuildGeometryInfo.pGeometries = &accelerationStructureGeometry;
		accelerationBuildGeometryInfo.scratchData.deviceAddress = getBufferDeviceAddress(context, scratchBuffer.buffer);

		vk::AccelerationStructureBuildRangeInfoKHR accelerationStructureBuildRangeInfo{};
		accelerationStructureBuildRangeInfo.primitiveCount = models.size();
		accelerationStructureBuildRangeInfo.primitiveOffset = 0;
		accelerationStructureBuildRangeInfo.firstVertex = 0;
		accelerationStructureBuildRangeInfo.transformOffset = 0;
		std::vector<vk::AccelerationStructureBuildRangeInfoKHR*> accelerationBuildStructureRangeInfos = { &accelerationStructureBuildRangeInfo };

		vk::raii::CommandBuffer commandBuffer = beginSingleTimeCommands(context);
		commandBuffer.buildAccelerationStructuresKHR(accelerationBuildGeometryInfo, accelerationBuildStructureRangeInfos);
		endSingleTimeCommands(context, commandBuffer);

		vk::AccelerationStructureDeviceAddressInfoKHR accelerationDeviceAddressInfo{accelerationStructure};
		auto deviceAddress = context.logical.getAccelerationStructureAddressKHR(accelerationDeviceAddressInfo);
		TLASs.push_back({ std::move(accelerationStructure),deviceAddress,std::move(tlasBuffer) });
	}
}

void RayTracing::createBlas(DContext& context, std::vector<Model>& models)
{
	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		vk::AccelerationStructureGeometryKHR accelerationGeometry{};
		accelerationGeometry.flags = vk::GeometryFlagBitsKHR::eOpaque;
		accelerationGeometry.geometryType = vk::GeometryTypeKHR::eTriangles;
		
		std::vector<AccelerationStructure> blass;
		std::vector<GeometryNode> geometryNodes;
		for (int j = 0; j < models.size();j++) {
			std::vector<vk::AccelerationStructureGeometryKHR> geometries;
			std::vector<uint32_t> maxPrimitiveCounts;
			std::vector<vk::AccelerationStructureBuildRangeInfoKHR> buildRangeInfos;
			std::vector<vk::AccelerationStructureBuildRangeInfoKHR*> pbuildRangeInfos;
			auto& model = models[j];
			for (auto& mesh : model.meshes) {
				vk::DeviceOrHostAddressConstKHR vertexDeviceAddress{};
				vk::DeviceOrHostAddressConstKHR indexDeviceAddress{};
				vk::DeviceOrHostAddressConstKHR transformDeviceAddress{};
				vertexDeviceAddress.deviceAddress = getBufferDeviceAddress(context, mesh.vertexBuffer.buffer);
				indexDeviceAddress.deviceAddress = getBufferDeviceAddress(context, mesh.indexBuffer.buffer);
				transformDeviceAddress.deviceAddress = getBufferDeviceAddress(context, model.uniformBuffers[i].buffer);

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
				//pbuildRangeInfos.push_back(&buildRangeInfo);
			}
			pbuildRangeInfos.push_back(buildRangeInfos.data());

			vk::AccelerationStructureBuildGeometryInfoKHR accelerationStructureBuildGeometryInfo{};
			accelerationStructureBuildGeometryInfo.type = vk::AccelerationStructureTypeKHR::eBottomLevel;
			accelerationStructureBuildGeometryInfo.flags = vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace;
			accelerationStructureBuildGeometryInfo.setGeometries(geometries);

			vk::AccelerationStructureBuildSizesInfoKHR accelerationStructureBuildSizesInfo =	
				context.logical.getAccelerationStructureBuildSizesKHR(
				vk::AccelerationStructureBuildTypeKHR::eDevice,
				accelerationStructureBuildGeometryInfo,
				maxPrimitiveCounts);
			
			auto blasbuffer = DBuffer(context, accelerationStructureBuildSizesInfo.accelerationStructureSize,
				vk::BufferUsageFlagBits::eAccelerationStructureStorageKHR| vk::BufferUsageFlagBits::eShaderDeviceAddress,
				vk::MemoryPropertyFlagBits::eDeviceLocal);

			vk::AccelerationStructureCreateInfoKHR accelerationStructureCreateInfo{};
			accelerationStructureCreateInfo.buffer = blasbuffer.buffer;
			accelerationStructureCreateInfo.size = accelerationStructureBuildSizesInfo.accelerationStructureSize;
			accelerationStructureCreateInfo.type = vk::AccelerationStructureTypeKHR::eBottomLevel;
			auto accelerationStructure = context.logical.createAccelerationStructureKHR(accelerationStructureCreateInfo);

			DBuffer scratchBuffer(context,
				accelerationStructureBuildSizesInfo.buildScratchSize,
				vk::BufferUsageFlagBits::eStorageBuffer| vk::BufferUsageFlagBits::eShaderDeviceAddress,
				vk::MemoryPropertyFlagBits::eDeviceLocal);

			vk::AccelerationStructureBuildGeometryInfoKHR accelerationBuildGeometryInfo{};
			accelerationBuildGeometryInfo.type = vk::AccelerationStructureTypeKHR::eBottomLevel;
			accelerationBuildGeometryInfo.flags = vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace;
			accelerationBuildGeometryInfo.mode = vk::BuildAccelerationStructureModeKHR::eBuild;
			accelerationBuildGeometryInfo.dstAccelerationStructure = accelerationStructure;
			accelerationBuildGeometryInfo.geometryCount = static_cast<uint32_t>(geometries.size());
			accelerationBuildGeometryInfo.pGeometries = geometries.data();
			accelerationBuildGeometryInfo.scratchData.deviceAddress = getBufferDeviceAddress(context, scratchBuffer.buffer);


			vk::raii::CommandBuffer commandBuffer = beginSingleTimeCommands(context);
			commandBuffer.buildAccelerationStructuresKHR(accelerationBuildGeometryInfo, pbuildRangeInfos);
			endSingleTimeCommands(context, commandBuffer);

			vk::AccelerationStructureDeviceAddressInfoKHR accelerationDeviceAddressInfo{};
			accelerationDeviceAddressInfo.accelerationStructure = accelerationStructure;
			auto deviceAddress = context.logical.getAccelerationStructureAddressKHR(accelerationDeviceAddressInfo);
			AccelerationStructure as{ std::move(accelerationStructure), deviceAddress,std::move(blasbuffer) };

			blass.push_back(std::move(as));
		}
		BLASs.push_back(std::move(blass));

		DBuffer geometryNodeBuffer(context,
			sizeof(GeometryNode) * static_cast<uint32_t>(geometryNodes.size()),
			vk::BufferUsageFlagBits::eShaderDeviceAddress| vk::BufferUsageFlagBits::eStorageBuffer| vk::BufferUsageFlagBits::eTransferDst,
			vk::MemoryPropertyFlagBits::eDeviceLocal);
		geometryNodeBuffer.fillBuffer(context,
			geometryNodes.data(),
			sizeof(GeometryNode)* static_cast<uint32_t>(geometryNodes.size()));


		geometryNodeBuffers.push_back(std::move(geometryNodeBuffer));

	}
}

void RayTracing::createSBT(DContext& context)
{
	const uint32_t handleSize = rayTracingPipelineProperties.shaderGroupHandleSize;
	
	const uint32_t handleSizeAligned = SBTalignedSize(rayTracingPipelineProperties.shaderGroupHandleSize, rayTracingPipelineProperties.shaderGroupHandleAlignment);

	const uint32_t groupCount = static_cast<uint32_t>(shaderGroups.size());

	const uint32_t sbtSize = groupCount * handleSizeAligned;

	
	std::vector<uint8_t> shaderHandleStorage = pipeline.getRayTracingShaderGroupHandlesKHR<uint8_t>(0, groupCount, sbtSize);;
	
	const vk::BufferUsageFlags bufferUsageFlags = vk::BufferUsageFlagBits::eShaderBindingTableKHR| vk::BufferUsageFlagBits::eShaderDeviceAddress;
	const vk::MemoryPropertyFlags memoryUsageFlags = vk::MemoryPropertyFlagBits::eHostVisible| vk::MemoryPropertyFlagBits::eHostCoherent;
	raygenShaderBindingTable = DBuffer(context, handleSize, bufferUsageFlags, memoryUsageFlags);
	missShaderBindingTable = DBuffer(context, handleSize * 2, bufferUsageFlags, memoryUsageFlags);
	hitShaderBindingTable = DBuffer(context, handleSize, bufferUsageFlags, memoryUsageFlags);


	raygenShaderBindingTable.map();
	missShaderBindingTable.map();
	hitShaderBindingTable.map();
	memcpy(raygenShaderBindingTable.mapped, shaderHandleStorage.data(), handleSize);
	memcpy(missShaderBindingTable.mapped, shaderHandleStorage.data() + handleSizeAligned, handleSize * 2);
	memcpy(hitShaderBindingTable.mapped, shaderHandleStorage.data() + handleSizeAligned * 3, handleSize);
}

void RayTracing::createRTPipeline(DContext& context)
{

	std::vector<vk::DescriptorSetLayoutBinding> bindings;
	auto components = DescriptorSetLayout::GetComponents(DescriptorType::RayTracing);
	bindings = DescriptorSetLayout::inputAttributeDescriptions(components);
	vk::DescriptorSetLayoutCreateInfo layoutInfo{ {},bindings };

	descriptorSetLayout = vk::raii::DescriptorSetLayout(context.logical, layoutInfo);


	vk::PipelineLayoutCreateInfo pipelineLayoutCreateInfo{};
	pipelineLayoutCreateInfo.setSetLayouts(*descriptorSetLayout);
	pipelineLayout = context.logical.createPipelineLayout(pipelineLayoutCreateInfo);
	
	std::vector<vk::PipelineShaderStageCreateInfo> shaderStages;


	vk::raii::ShaderModule rayGenShader = createShader(context, "shaders/raygen.rgen.spv");
	vk::raii::ShaderModule missShader = createShader(context, "shaders/miss.rmiss.spv");
	vk::raii::ShaderModule hitShader = createShader(context, "shaders/hit.rchit.spv");
	vk::raii::ShaderModule shadowShader = createShader(context, "shaders/shadow.rmiss.spv");

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

	pipeline = vk::raii::Pipeline(context.logical, nullptr, nullptr, rayTracingPipelineCreateInfo);
	
}

void RayTracing::createOutputImages(DContext& context)
{
	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT;i++) {
		outputImages.push_back(DImage(context,1,
			swapChain->imageFormat,
			swapChain->extent,
			vk::ImageTiling::eOptimal,
			vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eTransferSrc,
			vk::ImageLayout::eUndefined,
			vk::MemoryPropertyFlagBits::eDeviceLocal,
			vk::ImageAspectFlagBits::eColor));
		auto cmdBuf = beginSingleTimeCommands(context);
		outputImages[i].setImageLayout(cmdBuf, vk::ImageLayout::eGeneral);
		endSingleTimeCommands(context, cmdBuf);
	}
}

void RayTracing::createDescriptorSets(DContext& context, Scene& scene, std::vector<DBuffer>& uboBuffers, std::vector<DBuffer>& guiBuffers)
{
	std::vector<vk::DescriptorPoolSize> poolSizes = {
		{vk::DescriptorType::eAccelerationStructureKHR,1},
		{vk::DescriptorType::eStorageImage,1},
		{vk::DescriptorType::eStorageBuffer,1},
		{vk::DescriptorType::eUniformBuffer,2},
		{ vk::DescriptorType::eCombinedImageSampler,4 },
		{ vk::DescriptorType::eCombinedImageSampler,15 }
	};

	vk::DescriptorPoolCreateInfo descriptorPoolCreateInfo{};
	descriptorPoolCreateInfo.maxSets = 2;
	descriptorPoolCreateInfo.setPoolSizes(poolSizes);
	descriptorPoolCreateInfo.setFlags(vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet);
	descriptorPool = vk::raii::DescriptorPool(context.logical, descriptorPoolCreateInfo);
	
	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {

		vk::DescriptorSetAllocateInfo descriptorSetAllocateInfo{ descriptorPool,*descriptorSetLayout };
		descriptorSets.push_back(std::move(vk::raii::DescriptorSets(context.logical, descriptorSetAllocateInfo).front()));

		vk::WriteDescriptorSetAccelerationStructureKHR descriptorAccelerationStructureInfo{};
		descriptorAccelerationStructureInfo.setAccelerationStructures({ *TLASs[i].handle });

		vk::WriteDescriptorSet accelerationStructureWrite{};
		accelerationStructureWrite.pNext = &descriptorAccelerationStructureInfo;
		accelerationStructureWrite.dstSet = descriptorSets[i];
		accelerationStructureWrite.dstBinding = 0;
		accelerationStructureWrite.descriptorCount = 1;
		accelerationStructureWrite.descriptorType = vk::DescriptorType::eAccelerationStructureKHR;

		vk::DescriptorImageInfo storageImageDescriptor{};
		storageImageDescriptor.imageView = outputImages[i].view;
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
			imageInfo.imageView = scene.skybox.material.Get(i).view;
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
		lutImageInfo.imageView = scene.skybox.material.Get(3).view;
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
				imageInfo.imageView = model.material.Get(k).view;
				imageInfo.sampler = Sampler::Get(SamplerMipMapType::High);
				if (!model.material.hasComponent(k)) {
					imageInfo.imageView = Material::dummy.view;
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
		descriptorWriteForMaterials.descriptorCount = 1;
		descriptorWriteForMaterials.setImageInfo(materialInfos);




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
		context.logical.updateDescriptorSets(writeDescriptorSets, nullptr);
	}
}


void RayTracing::recordCommandBuffer(DContext& context, vk::raii::CommandBuffer& commandBuffer,int currentFrame,uint32_t imageIndex)
{


	const uint32_t handleSizeAligned = SBTalignedSize(rayTracingPipelineProperties.shaderGroupHandleSize, rayTracingPipelineProperties.shaderGroupHandleAlignment);

	VkStridedDeviceAddressRegionKHR raygenShaderSbtEntry{};
	raygenShaderSbtEntry.deviceAddress = getBufferDeviceAddress(context, raygenShaderBindingTable.buffer);
	raygenShaderSbtEntry.stride = handleSizeAligned;
	raygenShaderSbtEntry.size = handleSizeAligned;

	VkStridedDeviceAddressRegionKHR missShaderSbtEntry{};
	missShaderSbtEntry.deviceAddress = getBufferDeviceAddress(context, missShaderBindingTable.buffer);
	missShaderSbtEntry.stride = handleSizeAligned;
	missShaderSbtEntry.size = handleSizeAligned * 2;

	VkStridedDeviceAddressRegionKHR hitShaderSbtEntry{};
	hitShaderSbtEntry.deviceAddress = getBufferDeviceAddress(context, hitShaderBindingTable.buffer);
	hitShaderSbtEntry.stride = handleSizeAligned;
	hitShaderSbtEntry.size = handleSizeAligned;

	VkStridedDeviceAddressRegionKHR callableShaderSbtEntry{};
	commandBuffer.bindPipeline(vk::PipelineBindPoint::eRayTracingKHR, pipeline);
	commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eRayTracingKHR, pipelineLayout, 0, { descriptorSets[currentFrame] }, {});
	
	commandBuffer.traceRaysKHR(
		raygenShaderSbtEntry,
	missShaderSbtEntry,
	hitShaderSbtEntry,
		callableShaderSbtEntry,
		swapChain->extent.width,
		swapChain->extent.height,
		1);

	auto& outputSwapChain = swapChain->images[imageIndex];
	auto& outputImage = outputImages[currentFrame];
	vk::AccessFlags sourceAccessMask{};
	vk::PipelineStageFlags sourceStage = static_cast<vk::PipelineStageFlags>(65536);

	vk::AccessFlags destinationAccessMask = vk::AccessFlagBits::eTransferWrite; 
	vk::PipelineStageFlags destinationStage = static_cast<vk::PipelineStageFlags>(65536);

	sourceStage = vk::PipelineStageFlagBits::eTopOfPipe;
	destinationStage = vk::PipelineStageFlagBits::eTransfer;

	sourceAccessMask = vk::AccessFlagBits::eNone;
	destinationAccessMask = vk::AccessFlagBits::eTransferWrite;

	vk::ImageLayout layout = vk::ImageLayout::eUndefined;
	vk::ImageAspectFlags aspectMask = vk::ImageAspectFlagBits::eColor;
	vk::ImageSubresourceRange imageSubresourceRange(aspectMask, 0, 1, 0, 1);
	vk::ImageMemoryBarrier    imageMemoryBarrier(sourceAccessMask,
		destinationAccessMask,
		layout,
		vk::ImageLayout::eTransferDstOptimal,
		VK_QUEUE_FAMILY_IGNORED,
		VK_QUEUE_FAMILY_IGNORED,
		outputSwapChain,
		imageSubresourceRange);

	commandBuffer.pipelineBarrier(sourceStage, destinationStage, {}, nullptr, nullptr, imageMemoryBarrier);
	layout = vk::ImageLayout::eTransferDstOptimal;
	outputImage.setImageLayout(commandBuffer, vk::ImageLayout::eTransferSrcOptimal);


	vk::ImageCopy copyRegion{};
	copyRegion.srcSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
	copyRegion.srcOffset = vk::Offset3D{ 0, 0, 0 };
	copyRegion.dstSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
	copyRegion.dstOffset = vk::Offset3D{ 0, 0, 0 };
	copyRegion.extent = vk::Extent3D{ swapChain->extent.width, swapChain->extent.height, 1};
	
	commandBuffer.copyImage(outputImage.image,
		vk::ImageLayout::eTransferSrcOptimal,
		outputSwapChain,
		vk::ImageLayout::eTransferDstOptimal,
		copyRegion);

	sourceAccessMask= vk::AccessFlagBits::eTransferWrite;
	destinationAccessMask = vk::AccessFlagBits::eMemoryRead;

	sourceStage = vk::PipelineStageFlagBits::eTransfer;
	destinationStage = vk::PipelineStageFlagBits::eBottomOfPipe;

	aspectMask = vk::ImageAspectFlagBits::eColor;
	imageSubresourceRange = { aspectMask, 0, 1, 0, 1 };
	imageMemoryBarrier = { sourceAccessMask,
		destinationAccessMask,
		layout,
		vk::ImageLayout::ePresentSrcKHR,
		VK_QUEUE_FAMILY_IGNORED,
		VK_QUEUE_FAMILY_IGNORED,
		outputSwapChain,
		imageSubresourceRange };

	commandBuffer.pipelineBarrier(sourceStage, destinationStage, {}, nullptr, nullptr, imageMemoryBarrier);

	outputImage.setImageLayout(commandBuffer, vk::ImageLayout::eGeneral);

}

void RayTracing::loadFunctions(DContext& context)
{
	//vkGetBufferDeviceAddressKHR = reinterpret_cast<PFN_vkGetBufferDeviceAddressKHR>(vkGetDeviceProcAddr(context.logical, "vkGetBufferDeviceAddressKHR"));
	//vkCmdBuildAccelerationStructuresKHR = reinterpret_cast<PFN_vkCmdBuildAccelerationStructuresKHR>(vkGetDeviceProcAddr(context.logical, "vkCmdBuildAccelerationStructuresKHR"));
	//vkBuildAccelerationStructuresKHR = reinterpret_cast<PFN_vkBuildAccelerationStructuresKHR>(vkGetDeviceProcAddr(context.logical, "vkBuildAccelerationStructuresKHR"));
	//vkCreateAccelerationStructureKHR = reinterpret_cast<PFN_vkCreateAccelerationStructureKHR>(vkGetDeviceProcAddr(context.logical, "vkCreateAccelerationStructureKHR"));
	//vkDestroyAccelerationStructureKHR = reinterpret_cast<PFN_vkDestroyAccelerationStructureKHR>(vkGetDeviceProcAddr(context.logical, "vkDestroyAccelerationStructureKHR"));
	//vkGetAccelerationStructureBuildSizesKHR = reinterpret_cast<PFN_vkGetAccelerationStructureBuildSizesKHR>(vkGetDeviceProcAddr(context.logical, "vkGetAccelerationStructureBuildSizesKHR"));
	//vkGetAccelerationStructureDeviceAddressKHR = reinterpret_cast<PFN_vkGetAccelerationStructureDeviceAddressKHR>(vkGetDeviceProcAddr(context.logical, "vkGetAccelerationStructureDeviceAddressKHR"));
	//vkCmdTraceRaysKHR = reinterpret_cast<PFN_vkCmdTraceRaysKHR>(vkGetDeviceProcAddr(context.logical, "vkCmdTraceRaysKHR"));
	//vkGetRayTracingShaderGroupHandlesKHR = reinterpret_cast<PFN_vkGetRayTracingShaderGroupHandlesKHR>(vkGetDeviceProcAddr(context.logical, "vkGetRayTracingShaderGroupHandlesKHR"));
	//vkCreateRayTracingPipelinesKHR = reinterpret_cast<PFN_vkCreateRayTracingPipelinesKHR>(vkGetDeviceProcAddr(context.logical, "vkCreateRayTracingPipelinesKHR"));

}

uint64_t RayTracing::getBufferDeviceAddress(DContext& context, vk::raii::Buffer& buffer)
{
	vk::BufferDeviceAddressInfoKHR bufferDeviceAI{};
	bufferDeviceAI.buffer = buffer;
	return context.logical.getBufferAddressKHR(bufferDeviceAI);
}
