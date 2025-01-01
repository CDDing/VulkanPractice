#pragma once
class Mesh
{
public:
	Mesh() {};
	Mesh(Device& device, std::vector<Vertex> vertices, std::vector<unsigned int> indices, std::vector<Texture> textures);

	std::vector<Vertex> vertices;
	std::vector<unsigned int> indices;
	std::vector<Texture> textures;

	Buffer vertexBuffer;
	Buffer indexBuffer;
	void deleteMesh(Device& device);
private:
	void createIndexBuffer(Device& device);
    void createVertexBuffer(Device& device);
};

