#pragma once
class Mesh
{
public:
	Mesh() {};
	Mesh(std::shared_ptr<Device> device, std::vector<Vertex> vertices, std::vector<unsigned int> indices);
    // 복사
    Mesh(const Mesh& other)
        : vertices(other.vertices), indices(other.indices), vertexBuffer(other.vertexBuffer), indexBuffer(other.indexBuffer), _device(other._device) {
    }
    Mesh& operator=(const Mesh& other) {
        if (this != &other) {
            vertices = other.vertices;
            indices = other.indices;
            vertexBuffer = other.vertexBuffer;
            indexBuffer = other.indexBuffer;
            _device = other._device;
        }
        return *this;
    }

    // 이동
    Mesh(Mesh&& other) noexcept
        : vertices(std::move(other.vertices)), indices(std::move(other.indices)), vertexBuffer(std::move(other.vertexBuffer)), indexBuffer(std::move(other.indexBuffer)), _device(std::move(other._device)) {
    }

    Mesh& operator=(Mesh&& other) noexcept {
        if (this != &other) {
            vertices = std::move(other.vertices);
            indices = std::move(other.indices);
            vertexBuffer = std::move(other.vertexBuffer);
            indexBuffer = std::move(other.indexBuffer);
            _device = std::move(other._device);
        }
        return *this;
    }
	std::vector<Vertex> vertices;
	std::vector<unsigned int> indices;

	void draw(VkCommandBuffer, uint32_t renderFlags, VkPipelineLayout pipelineLayout, uint32_t bindImageSet);

	Buffer vertexBuffer;
	Buffer indexBuffer;
private:
	std::shared_ptr<Device> _device;
	void createIndexBuffer();
    void createVertexBuffer();
};

