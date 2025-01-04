#pragma once
class Mesh
{
public:
	Mesh() {};
	Mesh(Device& device, std::vector<Vertex> vertices, std::vector<unsigned int> indices, std::vector<Texture> textures);

	std::vector<Vertex> vertices;
	std::vector<unsigned int> indices;
	std::vector<Texture> textures;

	void draw(VkCommandBuffer, uint32_t renderFlags, VkPipelineLayout pipelineLayout, uint32_t bindImageSet);

	Buffer vertexBuffer;
	Buffer indexBuffer;
	void deleteMesh(Device& device);
private:
	void createIndexBuffer(Device& device);
    void createVertexBuffer(Device& device);
};

