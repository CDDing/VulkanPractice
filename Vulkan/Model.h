#pragma once
#include <assimp\Importer.hpp>
#include <assimp\postprocess.h>
#include <assimp\scene.h>
class Mesh;
class Material;
struct Transform;
enum BaseModel {
	Sphere,
	Box,
	Square,
};
class Model
{
public:
	Model(std::nullptr_t);
	Model(Device& device, const std::vector<MaterialComponent> components, const std::string& modelPath, const std::vector<std::string>& materialPaths, glm::mat4 transform);
	Model(Device& device, const std::vector<MaterialComponent> components, BaseModel modelType, const std::vector<std::string>& materialPaths, glm::mat4 transform);
	virtual ~Model() = default;
	void Render();

	void InitUniformBuffer(Device& device, glm::mat4 transform);
	virtual void InitDescriptorSet(Device& device);
	void InitDescriptorSetForModelMatrix(Device& device);
	void loadModel(Device& device, const std::string& modelPath);


	std::vector<Mesh> meshes = {};
	std::vector<DBuffer> uniformBuffers = {};
	std::vector<vk::raii::DescriptorSet> descriptorSets = {};
	Material material;
	Transform transform;
private:
	void processNode(Device& device, aiNode* node, const aiScene* scene);
	void processMesh(Device& device, aiMesh* mesh, const aiScene* scene);
	
	uint32_t _mipLevels;
};

void GenerateSphere(Device& device, Model& model);
void GenerateSquare(Device& device, Model& model);
void GenerateBox(Device& device, Model& model);