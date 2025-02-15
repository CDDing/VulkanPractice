#pragma once
class Mesh
{
public:
	Mesh() {};
	Mesh(std::shared_ptr<Device> device, std::vector<Vertex> vertices, std::vector<unsigned int> indices);

	void destroy(std::shared_ptr<Device> device);
	void draw(VkCommandBuffer, uint32_t renderFlags, VkPipelineLayout pipelineLayout, uint32_t bindImageSet);

	std::vector<Vertex> vertices;
	std::vector<unsigned int> indices;

	Buffer vertexBuffer;
	Buffer indexBuffer;
private:
	void createIndexBuffer(std::shared_ptr<Device> device);
    void createVertexBuffer(std::shared_ptr<Device> device);
};

