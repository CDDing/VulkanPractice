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
	Model(std::shared_ptr<Device> device, const float& scale, const std::vector<MaterialComponent> components, const std::string& modelPath, const std::vector<std::string>& materialPaths,glm::mat4 transform);

	void Render();
	void destroy(std::shared_ptr<Device> device);

	std::vector<Mesh> meshes;
	Material material;
	void InitUniformBuffer(std::shared_ptr<Device> device, glm::mat4 transform);
	virtual void InitDescriptorSet(std::shared_ptr<Device> device);
	void InitDescriptorSetForModelMatrix(std::shared_ptr<Device> device);
	std::vector<DescriptorSet> descriptorSets;
	void loadModel(std::shared_ptr<Device> device, const std::string& modelPath, const float& scale);
	std::vector<Buffer> uniformBuffers;
	Transform transform;
private:
	void processNode(std::shared_ptr<Device> device, aiNode* node, const aiScene* scene, const float& scale);
	Mesh processMesh(std::shared_ptr<Device> device, aiMesh* mesh, const aiScene* scene, const float& scale);
	uint32_t _mipLevels;
};


Model makeSphere(std::shared_ptr<Device> device, glm::mat4 transform, const std::vector<MaterialComponent>& components, const std::vector<std::string>& materialPaths);
Model makeSqaure(std::shared_ptr<Device> device, glm::mat4 transform, const std::vector<MaterialComponent>& components, const std::vector<std::string>& materialPaths);
Model makeBox(std::shared_ptr<Device> device, glm::mat4 transform, const std::vector<MaterialComponent>& components, const std::vector<std::string>& materialPaths);
void GenerateSphere(std::shared_ptr<Device> device, Model& model);
void GenerateSquare(std::shared_ptr<Device> device, Model& model);
void GenerateBox(std::shared_ptr<Device> device, Model& model);