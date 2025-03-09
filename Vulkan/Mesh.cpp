#include "pch.h"
#include "Mesh.h"

Mesh::Mesh(DContext& context, std::vector<Vertex>& vertices, std::vector<unsigned int>& indices) :
	indexBuffer(nullptr), vertexBuffer(nullptr)
{
	this->vertices = vertices;
	this->indices = indices;

	createVertexBuffer(context);
	createIndexBuffer(context);
}

void Mesh::draw(VkCommandBuffer, uint32_t renderFlags, VkPipelineLayout pipelineLayout, uint32_t bindImageSet)
{

}

void Mesh::createIndexBuffer(DContext& context)
{
	VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size();
	indexBuffer = DBuffer(context, bufferSize,
		vk::BufferUsageFlagBits::eIndexBuffer |
		vk::BufferUsageFlagBits::eTransferDst |
		vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR |
		vk::BufferUsageFlagBits::eShaderDeviceAddress,
		vk::MemoryPropertyFlagBits::eDeviceLocal);

	DBuffer stagingBuffer(context, bufferSize, 
		vk::BufferUsageFlagBits::eTransferSrc, 
		vk::MemoryPropertyFlagBits::eHostVisible| 
		vk::MemoryPropertyFlagBits::eHostCoherent);

	stagingBuffer.map(bufferSize, 0); 
	memcpy(stagingBuffer.mapped, indices.data(), (size_t)bufferSize);
	stagingBuffer.unmap();

	copyBuffer(context, stagingBuffer, indexBuffer, bufferSize);

}

void Mesh::createVertexBuffer(DContext& context)
{
	VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();
	vertexBuffer = DBuffer(context, bufferSize, 
		vk::BufferUsageFlagBits::eVertexBuffer | 
		vk::BufferUsageFlagBits::eTransferDst  | 
		vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR| 
		vk::BufferUsageFlagBits::eShaderDeviceAddress,
		vk::MemoryPropertyFlagBits::eDeviceLocal);

	DBuffer stagingBuffer(context, bufferSize,
		vk::BufferUsageFlagBits::eTransferSrc,
		vk::MemoryPropertyFlagBits::eHostVisible |
		vk::MemoryPropertyFlagBits::eHostCoherent);


	stagingBuffer.map(bufferSize, 0);
	memcpy(stagingBuffer.mapped, vertices.data(), (size_t)bufferSize);
	stagingBuffer.unmap();

	copyBuffer(context, stagingBuffer, vertexBuffer, bufferSize);
	
}
