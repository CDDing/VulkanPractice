#pragma once
#include <assimp\Importer.hpp>
#include <assimp\postprocess.h>
#include <assimp\scene.h>
class Mesh;
class Material;
struct Transform;
enum ImageType {
	TEXTURE,
	NORMALMAP,
	HEIGHTMAP,
};
class Model
{
public:
	Model() {};
	Model(Device& device, const float& scale, const std::vector<MaterialComponent> components, const std::string& modelPath, const std::vector<std::string>& materialPaths,glm::mat4 transform);

	void Render();
	void destroy(Device& device);

	std::vector<std::shared_ptr<Mesh>> meshes;
	Material material;
	void InitUniformBuffer(Device& device, glm::mat4 transform);
	void InitDescriptorSet(Device& device, DescriptorSet& descriptorSet);
	void InitDescriptorSetForSkybox(Device& device, DescriptorSet& descriptorSet);
	void InitDescriptorSetForModelMatrix(Device& device, DescriptorSet& desciprotrSet);
	std::vector<DescriptorSet> descriptorSets;
	void loadModel(Device& device, const std::string& modelPath, const float& scale);
	std::vector<Buffer> uniformBuffers;
private:
	Transform _transform;
	std::vector<void*> _uniformBuffersMapped;
	void processNode(Device& device, aiNode* node, const aiScene* scene, const float& scale);
	Mesh processMesh(Device& device, aiMesh* mesh, const aiScene* scene, const float& scale);
	uint32_t _mipLevels;
};


Model makeSphere(Device& device, glm::mat4 transform, const std::vector<MaterialComponent>& components, const std::vector<std::string>& materialPaths);
Model makeSqaure(Device& device, glm::mat4 transform, const std::vector<MaterialComponent>& components, const std::vector<std::string>& materialPaths);
Model makeBox(Device& device, glm::mat4 transform, const std::vector<MaterialComponent>& components, const std::vector<std::string>& materialPaths);
void GenerateSphere(Device& device, Model& model);
void GenerateSquare(Device& device, Model& model);
void GenerateBox(Device& device, Model& model);
Model makeSkyBox(Device& device);