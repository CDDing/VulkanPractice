#include "pch.h"
#include "Mesh.h"

Mesh::Mesh(Device& device, std::vector<Vertex>& vertices, std::vector<unsigned int>& indices) :
	indexBuffer(nullptr), vertexBuffer(nullptr)
{
	this->vertices = vertices;
	this->indices = indices;

	createVertexBuffer(device);
	createIndexBuffer(device);
}

void Mesh::draw(VkCommandBuffer, uint32_t renderFlags, VkPipelineLayout pipelineLayout, uint32_t bindImageSet)
{

}

void Mesh::createIndexBuffer(Device& device)
{
	VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size();
	indexBuffer = DBuffer(device, bufferSize,
		vk::BufferUsageFlagBits::eIndexBuffer |
		vk::BufferUsageFlagBits::eTransferDst |
		vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR |
		vk::BufferUsageFlagBits::eShaderDeviceAddress,
		vk::MemoryPropertyFlagBits::eDeviceLocal);

	DBuffer stagingBuffer(device, bufferSize, 
		vk::BufferUsageFlagBits::eTransferSrc, 
		vk::MemoryPropertyFlagBits::eHostVisible| 
		vk::MemoryPropertyFlagBits::eHostCoherent);

	stagingBuffer.map(device, bufferSize, 0); 
	memcpy(stagingBuffer.mapped, indices.data(), (size_t)bufferSize);
	stagingBuffer.unmap(device);

	copyBuffer(device, stagingBuffer, indexBuffer, bufferSize);

}

void Mesh::createVertexBuffer(Device& device)
{
	VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();
	vertexBuffer = DBuffer(device, bufferSize, 
		vk::BufferUsageFlagBits::eVertexBuffer | 
		vk::BufferUsageFlagBits::eTransferDst  | 
		vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR| 
		vk::BufferUsageFlagBits::eShaderDeviceAddress,
		vk::MemoryPropertyFlagBits::eDeviceLocal);

	DBuffer stagingBuffer(device, bufferSize,
		vk::BufferUsageFlagBits::eTransferSrc,
		vk::MemoryPropertyFlagBits::eHostVisible |
		vk::MemoryPropertyFlagBits::eHostCoherent);


	stagingBuffer.map(device, bufferSize, 0);
	memcpy(stagingBuffer.mapped, vertices.data(), (size_t)bufferSize);
	stagingBuffer.unmap(device);

	copyBuffer(device, stagingBuffer, vertexBuffer, bufferSize);
	
}
