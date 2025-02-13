#include "pch.h"
#include "Mesh.h"

Mesh::Mesh(std::shared_ptr<Device> device, std::vector<Vertex> vertices, std::vector<unsigned int> indices) :_device(device)
{
	this->vertices = vertices;
	this->indices = indices;

	createVertexBuffer();
	createIndexBuffer();
}

void Mesh::draw(VkCommandBuffer, uint32_t renderFlags, VkPipelineLayout pipelineLayout, uint32_t bindImageSet)
{

}
void Mesh::createIndexBuffer()
{
	VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size();
	indexBuffer = Buffer(_device, bufferSize,
		vk::BufferUsageFlagBits::eIndexBuffer |
		vk::BufferUsageFlagBits::eTransferDst |
		vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR |
		vk::BufferUsageFlagBits::eShaderDeviceAddress,
		vk::MemoryPropertyFlagBits::eDeviceLocal);

	Buffer stagingBuffer;
	stagingBuffer = Buffer(_device, bufferSize, 
		vk::BufferUsageFlagBits::eTransferSrc, 
		vk::MemoryPropertyFlagBits::eHostVisible| 
		vk::MemoryPropertyFlagBits::eHostCoherent);

	stagingBuffer.map(bufferSize, 0); 
	memcpy(stagingBuffer.mapped, indices.data(), (size_t)bufferSize);
	stagingBuffer.unmap();

	copyBuffer(_device, stagingBuffer, indexBuffer, bufferSize);

}

void Mesh::createVertexBuffer()
{
	VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();
	vertexBuffer = Buffer(_device, bufferSize, 
		vk::BufferUsageFlagBits::eVertexBuffer | 
		vk::BufferUsageFlagBits::eTransferDst  | 
		vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR| 
		vk::BufferUsageFlagBits::eShaderDeviceAddress,
		vk::MemoryPropertyFlagBits::eDeviceLocal);

	Buffer stagingBuffer = Buffer(_device, bufferSize,
		vk::BufferUsageFlagBits::eTransferSrc,
		vk::MemoryPropertyFlagBits::eHostVisible |
		vk::MemoryPropertyFlagBits::eHostCoherent);


	stagingBuffer.map(bufferSize, 0);
	memcpy(stagingBuffer.mapped, vertices.data(), (size_t)bufferSize);
	stagingBuffer.unmap();

	copyBuffer(_device, stagingBuffer, vertexBuffer, bufferSize);
	

}
