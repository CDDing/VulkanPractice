#include "pch.h"
#include "RayTracing.h"

void RayTracing::init(Device& device, int currentFrame)
{

	// Get ray tracing pipeline properties, which will be used later on in the sample
	rayTracingPipelineProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR;
	VkPhysicalDeviceProperties2 deviceProperties2{};
	deviceProperties2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
	deviceProperties2.pNext = &rayTracingPipelineProperties;
	vkGetPhysicalDeviceProperties2(device.GetPhysical(), &deviceProperties2);

	loadFunctions(device);
	createBlas(device,currentFrame);
	createTlas();
}

void RayTracing::createTlas()
{
}

void RayTracing::createBlas(Device&device, int currentFrame)
{
	std::vector<Model> models;
	
	VkAccelerationStructureGeometryKHR accelerationGeometry{};
	accelerationGeometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
	accelerationGeometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
	accelerationGeometry.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
	accelerationGeometry.geometry.triangles.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
	
	std::vector<VkAccelerationStructureGeometryKHR> geometries;
	uint32_t numTriangles = 1;
	for (auto& model : models) {
		for (const auto& mesh : model.meshes) {
			VkDeviceOrHostAddressConstKHR vertexDeviceAddress{};
			VkDeviceOrHostAddressConstKHR indexDeviceAddress{};
			VkDeviceOrHostAddressConstKHR transformDeviceAddress{};
			vertexDeviceAddress.deviceAddress = getBufferDeviceAddress(device, mesh->vertexBuffer);
			indexDeviceAddress.deviceAddress = getBufferDeviceAddress(device, mesh->indexBuffer);
			transformDeviceAddress.deviceAddress = getBufferDeviceAddress(device, model.uniformBuffers[currentFrame]);

			VkAccelerationStructureGeometryTrianglesDataKHR triangles;
			triangles.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
			triangles.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
			triangles.vertexData = vertexDeviceAddress;
			triangles.maxVertex = mesh->vertices.size();
			triangles.vertexStride = sizeof(Vertex);

			triangles.indexType = VK_INDEX_TYPE_UINT32;
			triangles.indexData = indexDeviceAddress;
			
			triangles.transformData.deviceAddress = 0;
			triangles.transformData.hostAddress = nullptr;
			triangles.transformData = transformDeviceAddress;

			accelerationGeometry.geometry.triangles = triangles;

			geometries.push_back(accelerationGeometry);

		}
	}

	VkAccelerationStructureBuildGeometryInfoKHR accelerationStructureBuildGeometryInfo{};
	accelerationStructureBuildGeometryInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
	accelerationStructureBuildGeometryInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
	accelerationStructureBuildGeometryInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
	accelerationStructureBuildGeometryInfo.geometryCount = static_cast<uint32_t>(geometries.size());
	accelerationStructureBuildGeometryInfo.pGeometries = geometries.data();

	VkAccelerationStructureBuildSizesInfoKHR accelerationStructureBuildSizesInfo{};
	accelerationStructureBuildSizesInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
	vkGetAccelerationStructureBuildSizesKHR(device.Get(), VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
		&accelerationStructureBuildGeometryInfo,
		&numTriangles,
		&accelerationStructureBuildSizesInfo);
	
	blas.buffer = Buffer(device, accelerationStructureBuildSizesInfo.accelerationStructureSize ,
		VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, 
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	VkAccelerationStructureCreateInfoKHR accelerationStructureCreateInfo{};
	accelerationStructureCreateInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
	accelerationStructureCreateInfo.buffer = blas.buffer.Get();
	accelerationStructureCreateInfo.size = accelerationStructureBuildSizesInfo.accelerationStructureSize;
	accelerationStructureCreateInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
	vkCreateAccelerationStructureKHR(device.Get(), &accelerationStructureCreateInfo, nullptr, &blas.handle);
	
	Buffer scratchBuffer = Buffer(device,
		accelerationStructureBuildSizesInfo.buildScratchSize,
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	VkAccelerationStructureBuildGeometryInfoKHR accelerationBuildGeometryInfo{};
	accelerationBuildGeometryInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
	accelerationBuildGeometryInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
	accelerationBuildGeometryInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
	accelerationBuildGeometryInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
	accelerationBuildGeometryInfo.dstAccelerationStructure = blas.handle;
	accelerationBuildGeometryInfo.geometryCount = static_cast<uint32_t>(geometries.size());
	accelerationBuildGeometryInfo.pGeometries = geometries.data();
	accelerationBuildGeometryInfo.scratchData.deviceAddress = getBufferDeviceAddress(device,scratchBuffer);
	

	VkAccelerationStructureBuildRangeInfoKHR accelerationStructureBuildRangeInfo{};
	accelerationStructureBuildRangeInfo.primitiveCount = numTriangles;
	accelerationStructureBuildRangeInfo.primitiveOffset = 0;
	accelerationStructureBuildRangeInfo.firstVertex = 0;
	accelerationStructureBuildRangeInfo.transformOffset = 0;
	std::vector<VkAccelerationStructureBuildRangeInfoKHR*> accelerationBuildStructureRangeInfos =
	{ &accelerationStructureBuildRangeInfo };

	VkCommandBuffer commandBuffer = beginSingleTimeCommands(device);
	vkCmdBuildAccelerationStructuresKHR(commandBuffer, 1, &accelerationBuildGeometryInfo, accelerationBuildStructureRangeInfos.data());
	endSingleTimeCommands(device, commandBuffer);

	VkAccelerationStructureDeviceAddressInfoKHR accelerationDeviceAddressInfo{};
	accelerationDeviceAddressInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
	accelerationDeviceAddressInfo.accelerationStructure = blas.handle;
	blas.deviceAddress = vkGetAccelerationStructureDeviceAddressKHR(device.Get(), &accelerationDeviceAddressInfo);

	scratchBuffer.destroy(device);
}

void RayTracing::destroy(Device& device)
{

	vkDestroyBuffer(device.Get(), blas.buffer.Get(), nullptr);
	vkFreeMemory(device.Get(), blas.buffer.GetMemory(), nullptr);
	vkDestroyAccelerationStructureKHR(device.Get(), blas.handle, nullptr);


	//vkDestroyBuffer(device.Get(), tlas.buffer.Get(), nullptr);
	//vkFreeMemory(device.Get(), tlas.buffer.GetMemory(), nullptr);
	//vkDestroyAccelerationStructureKHR(device.Get(), tlas.handle, nullptr);
}

void RayTracing::loadFunctions(Device& device)
{
	vkGetBufferDeviceAddressKHR = reinterpret_cast<PFN_vkGetBufferDeviceAddressKHR>(vkGetDeviceProcAddr(device.Get(), "vkGetBufferDeviceAddressKHR"));
	vkCmdBuildAccelerationStructuresKHR = reinterpret_cast<PFN_vkCmdBuildAccelerationStructuresKHR>(vkGetDeviceProcAddr(device.Get(), "vkCmdBuildAccelerationStructuresKHR"));
	vkBuildAccelerationStructuresKHR = reinterpret_cast<PFN_vkBuildAccelerationStructuresKHR>(vkGetDeviceProcAddr(device.Get(), "vkBuildAccelerationStructuresKHR"));
	vkCreateAccelerationStructureKHR = reinterpret_cast<PFN_vkCreateAccelerationStructureKHR>(vkGetDeviceProcAddr(device.Get(), "vkCreateAccelerationStructureKHR"));
	vkDestroyAccelerationStructureKHR = reinterpret_cast<PFN_vkDestroyAccelerationStructureKHR>(vkGetDeviceProcAddr(device.Get(), "vkDestroyAccelerationStructureKHR"));
	vkGetAccelerationStructureBuildSizesKHR = reinterpret_cast<PFN_vkGetAccelerationStructureBuildSizesKHR>(vkGetDeviceProcAddr(device.Get(), "vkGetAccelerationStructureBuildSizesKHR"));
	vkGetAccelerationStructureDeviceAddressKHR = reinterpret_cast<PFN_vkGetAccelerationStructureDeviceAddressKHR>(vkGetDeviceProcAddr(device.Get(), "vkGetAccelerationStructureDeviceAddressKHR"));
	vkCmdTraceRaysKHR = reinterpret_cast<PFN_vkCmdTraceRaysKHR>(vkGetDeviceProcAddr(device.Get(), "vkCmdTraceRaysKHR"));
	vkGetRayTracingShaderGroupHandlesKHR = reinterpret_cast<PFN_vkGetRayTracingShaderGroupHandlesKHR>(vkGetDeviceProcAddr(device.Get(), "vkGetRayTracingShaderGroupHandlesKHR"));
	vkCreateRayTracingPipelinesKHR = reinterpret_cast<PFN_vkCreateRayTracingPipelinesKHR>(vkGetDeviceProcAddr(device.Get(), "vkCreateRayTracingPipelinesKHR"));

}

uint64_t RayTracing::getBufferDeviceAddress(Device& device, Buffer& buffer)
{
	VkBufferDeviceAddressInfoKHR bufferDeviceAI{};
	bufferDeviceAI.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
	bufferDeviceAI.buffer = buffer.Get();
	return vkGetBufferDeviceAddressKHR(device.Get(), &bufferDeviceAI);
}
