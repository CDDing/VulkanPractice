#pragma once
class Mesh
{
public:
	Mesh() {};
	Mesh(std::shared_ptr<Device> device, std::vector<Vertex> vertices, std::vector<unsigned int> indices);

	std::vector<Vertex> vertices;
	std::vector<unsigned int> indices;

	void draw(VkCommandBuffer, uint32_t renderFlags, VkPipelineLayout pipelineLayout, uint32_t bindImageSet);

	Buffer vertexBuffer;
	Buffer indexBuffer;
	void destroy(std::shared_ptr<Device> device);
private:
	void createIndexBuffer(std::shared_ptr<Device> device);
    void createVertexBuffer(std::shared_ptr<Device> device);
};

