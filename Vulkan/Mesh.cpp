#include "pch.h"
#include "Mesh.h"

Mesh::Mesh(std::shared_ptr<Device> device, std::vector<Vertex> vertices, std::vector<unsigned int> indices)
{
	this->vertices = vertices;
	this->indices = indices;

	createVertexBuffer(device);
	createIndexBuffer(device);
}

void Mesh::draw(VkCommandBuffer, uint32_t renderFlags, VkPipelineLayout pipelineLayout, uint32_t bindImageSet)
{

}

void Mesh::destroy(std::shared_ptr<Device> device)
{
	vertexBuffer.destroy(device);
	indexBuffer.destroy(device);
}

void Mesh::createIndexBuffer(std::shared_ptr<Device> device)
{
	VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size();
	indexBuffer = Buffer(device, bufferSize,
		vk::BufferUsageFlagBits::eIndexBuffer |
		vk::BufferUsageFlagBits::eTransferDst |
		vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR |
		vk::BufferUsageFlagBits::eShaderDeviceAddress,
		vk::MemoryPropertyFlagBits::eDeviceLocal);

	Buffer stagingBuffer;
	stagingBuffer = Buffer(device, bufferSize, 
		vk::BufferUsageFlagBits::eTransferSrc, 
		vk::MemoryPropertyFlagBits::eHostVisible| 
		vk::MemoryPropertyFlagBits::eHostCoherent);

	stagingBuffer.map(device, bufferSize, 0); 
	memcpy(stagingBuffer.mapped, indices.data(), (size_t)bufferSize);
	stagingBuffer.unmap(device);

	copyBuffer(device, stagingBuffer, indexBuffer, bufferSize);

	stagingBuffer.destroy(device);
}

void Mesh::createVertexBuffer(std::shared_ptr<Device> device)
{
	VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();
	vertexBuffer = Buffer(device, bufferSize, 
		vk::BufferUsageFlagBits::eVertexBuffer | 
		vk::BufferUsageFlagBits::eTransferDst  | 
		vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR| 
		vk::BufferUsageFlagBits::eShaderDeviceAddress,
		vk::MemoryPropertyFlagBits::eDeviceLocal);

	Buffer stagingBuffer = Buffer(device, bufferSize,
		vk::BufferUsageFlagBits::eTransferSrc,
		vk::MemoryPropertyFlagBits::eHostVisible |
		vk::MemoryPropertyFlagBits::eHostCoherent);


	stagingBuffer.map(device, bufferSize, 0);
	memcpy(stagingBuffer.mapped, vertices.data(), (size_t)bufferSize);
	stagingBuffer.unmap(device);

	copyBuffer(device, stagingBuffer, vertexBuffer, bufferSize);
	
	stagingBuffer.destroy(device);

}
