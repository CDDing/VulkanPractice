#pragma once
class Mesh
{
public:
	Mesh(std::nullptr_t) : vertexBuffer(nullptr), indexBuffer(nullptr) {};
	Mesh(DContext& context, std::vector<Vertex>& vertices, std::vector<unsigned int>& indices);
	Mesh(const Mesh&) = delete;
	Mesh& operator=(const Mesh&) = delete;
	Mesh(Mesh&&) noexcept = default;
	Mesh& operator=(Mesh&&) noexcept = default;
	void draw(VkCommandBuffer, uint32_t renderFlags, VkPipelineLayout pipelineLayout, uint32_t bindImageSet);

	std::vector<Vertex> vertices;
	std::vector<unsigned int> indices;

	DBuffer vertexBuffer;
	DBuffer indexBuffer;
private:
	void createIndexBuffer(DContext& context);
    void createVertexBuffer(DContext& context);
};

