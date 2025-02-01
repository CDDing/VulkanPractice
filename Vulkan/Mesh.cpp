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
	vkDestroyBuffer(device, vertexBuffer.Get(), nullptr);
	vkFreeMemory(device, vertexBuffer.GetMemory(), nullptr);

	vkDestroyBuffer(device, indexBuffer.Get(), nullptr);
	vkFreeMemory(device, indexBuffer.GetMemory(), nullptr);
}

void Mesh::createIndexBuffer(Device& device)
{
	VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size();

	Buffer stagingBuffer;
	stagingBuffer = Buffer(device, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

	void* data;
	vkMapMemory(device, stagingBuffer.GetMemory(), 0, bufferSize, 0, &data);
	memcpy(data, indices.data(), (size_t)bufferSize);
	vkUnmapMemory(device, stagingBuffer.GetMemory());

	indexBuffer = Buffer(device, bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT  | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	copyBuffer(device, stagingBuffer.Get(), indexBuffer.Get(), bufferSize);

	vkDestroyBuffer(device, stagingBuffer.Get(), nullptr);
	vkFreeMemory(device, stagingBuffer.GetMemory(), nullptr);
}

void Mesh::createVertexBuffer(Device& device)
{
	VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();
	vertexBuffer = Buffer(device, bufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT  | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	Buffer stagingBuffer;
	stagingBuffer = Buffer(device, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);



	void* data;
	vkMapMemory(device, stagingBuffer.GetMemory(), 0, bufferSize, 0, &data);
	memcpy(data, vertices.data(), (size_t)bufferSize);
	vkUnmapMemory(device, stagingBuffer.GetMemory());

	copyBuffer(device, stagingBuffer.Get(), vertexBuffer.Get(), bufferSize);

	vkDestroyBuffer(device, stagingBuffer.Get(), nullptr);
	vkFreeMemory(device, stagingBuffer.GetMemory(), nullptr);

}
