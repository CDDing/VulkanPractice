#pragma once
class Mesh
{
public:
	Mesh() {};
	Mesh(Device& device, std::vector<Vertex> vertices, std::vector<unsigned int> indices);

	std::vector<Vertex> vertices;
	std::vector<unsigned int> indices;

	void draw(VkCommandBuffer, uint32_t renderFlags, VkPipelineLayout pipelineLayout, uint32_t bindImageSet);

	Buffer vertexBuffer;
	Buffer indexBuffer;
	void destroy(Device& device);
private:
	void createIndexBuffer(Device& device);
    void createVertexBuffer(Device& device);
};

