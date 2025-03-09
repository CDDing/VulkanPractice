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
	Model(DContext& context, const std::vector<MaterialComponent> components, const std::string& modelPath, const std::vector<std::string>& materialPaths, glm::mat4 transform);
	Model(DContext& context, const std::vector<MaterialComponent> components, BaseModel modelType, const std::vector<std::string>& materialPaths, glm::mat4 transform);
	virtual ~Model() = default;

	Model(const Model&) = delete;
	Model& operator=(const Model&) = delete;

	Model(Model&&) = default;
	Model& operator=(Model&&) = default;
	void Render();

	void InitUniformBuffer(DContext& context, glm::mat4 transform);
	virtual void InitDescriptorSet(DContext& context);
	void InitDescriptorSetForModelMatrix(DContext& context);
	void loadModel(DContext& context, const std::string& modelPath);


	std::vector<Mesh> meshes = {};
	std::vector<DBuffer> uniformBuffers = {};
	std::vector<vk::raii::DescriptorSet> descriptorSets = {};
	Material material;
	Transform transform;
private:
	void processNode(DContext& context, aiNode* node, const aiScene* scene);
	void processMesh(DContext& context, aiMesh* mesh, const aiScene* scene);
	
	uint32_t _mipLevels;
};

void GenerateSphere(DContext& context, Model& model);
void GenerateSquare(DContext& context, Model& model);
void GenerateBox(DContext& context, Model& model);