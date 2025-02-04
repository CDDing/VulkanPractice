#include "pch.h"
#include "Mesh.h"

Mesh::Mesh(Device& device, std::vector<Vertex> vertices, std::vector<unsigned int> indices)
{
	this->vertices = vertices;
	this->indices = indices;

	createVertexBuffer(device);
	createIndexBuffer(device);
}

void Mesh::draw(VkCommandBuffer, uint32_t renderFlags, VkPipelineLayout pipelineLayout, uint32_t bindImageSet)
{

}

void Mesh::destroy(Device& device)
{
	vertexBuffer.destroy(device);
	indexBuffer.destroy(device);
}

void Mesh::createIndexBuffer(Device& device)
{
	VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size();
	indexBuffer = Buffer(device, bufferSize,
		VK_BUFFER_USAGE_TRANSFER_DST_BIT |
		VK_BUFFER_USAGE_INDEX_BUFFER_BIT |
		VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR |
		VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	Buffer stagingBuffer;
	stagingBuffer = Buffer(device, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

	stagingBuffer.map(device, bufferSize, 0); 
	memcpy(stagingBuffer.mapped, indices.data(), (size_t)bufferSize);
	stagingBuffer.unmap(device);

	copyBuffer(device, stagingBuffer, indexBuffer, bufferSize);

	stagingBuffer.destroy(device);
}

void Mesh::createVertexBuffer(Device& device)
{
	VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();
	vertexBuffer = Buffer(device, bufferSize, 
		VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | 
		VK_BUFFER_USAGE_TRANSFER_DST_BIT  | 
		VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | 
		VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	Buffer stagingBuffer;
	stagingBuffer = Buffer(device, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);



	stagingBuffer.map(device, bufferSize, 0);
	memcpy(stagingBuffer.mapped, vertices.data(), (size_t)bufferSize);
	stagingBuffer.unmap(device);

	copyBuffer(device, stagingBuffer, vertexBuffer, bufferSize);
	
	stagingBuffer.destroy(device);

}
