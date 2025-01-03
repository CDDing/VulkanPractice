#include "pch.h"
#include "Mesh.h"

Mesh::Mesh(Device& device, std::vector<Vertex> vertices, std::vector<unsigned int> indices, std::vector<Texture> textures)
{
	this->vertices = vertices;
	this->indices = indices;
	this->textures = textures;

	createVertexBuffer(device);
	createIndexBuffer(device);
}

void Mesh::deleteMesh(Device& device)
{
	vkDestroyBuffer(device.Get(), vertexBuffer.Get(), nullptr);
	vkFreeMemory(device.Get(), vertexBuffer.GetMemory(), nullptr);

	vkDestroyBuffer(device.Get(), indexBuffer.Get(), nullptr);
	vkFreeMemory(device.Get(), indexBuffer.GetMemory(), nullptr);
}

void Mesh::createIndexBuffer(Device& device)
{
	VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size();

	Buffer stagingBuffer;
	stagingBuffer = Buffer(device, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

	void* data;
	vkMapMemory(device.Get(), stagingBuffer.GetMemory(), 0, bufferSize, 0, &data);
	memcpy(data, indices.data(), (size_t)bufferSize);
	vkUnmapMemory(device.Get(), stagingBuffer.GetMemory());

	indexBuffer = Buffer(device, bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	copyBuffer(device, stagingBuffer.Get(), indexBuffer.Get(), bufferSize);

	vkDestroyBuffer(device.Get(), stagingBuffer.Get(), nullptr);
	vkFreeMemory(device.Get(), stagingBuffer.GetMemory(), nullptr);
}

void Mesh::createVertexBuffer(Device& device)
{
	VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();
	vertexBuffer = Buffer(device, bufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	Buffer stagingBuffer;
	stagingBuffer = Buffer(device, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);



	void* data;
	vkMapMemory(device.Get(), stagingBuffer.GetMemory(), 0, bufferSize, 0, &data);
	memcpy(data, vertices.data(), (size_t)bufferSize);
	vkUnmapMemory(device.Get(), stagingBuffer.GetMemory());

	copyBuffer(device, stagingBuffer.Get(), vertexBuffer.Get(), bufferSize);

	vkDestroyBuffer(device.Get(), stagingBuffer.Get(), nullptr);
	vkFreeMemory(device.Get(), stagingBuffer.GetMemory(), nullptr);

}
