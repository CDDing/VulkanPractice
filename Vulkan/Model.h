#pragma once
#include <assimp\Importer.hpp>
#include <assimp\postprocess.h>
#include <assimp\scene.h>
class Mesh;
class Material;
enum ImageType {
	TEXTURE,
	NORMALMAP,
	HEIGHTMAP,
};
class Model
{
public:
	Model() {};
	Model(Device& device, const float& scale, const std::vector<MaterialComponent> components, const std::string& modelPath, const std::vector<std::string>& materialPaths);

	void Render();
	void destroy(Device& device);
	glm::mat4 world = glm::mat4();

	std::vector<std::shared_ptr<Mesh>> meshes;
	Material material;
	void loadModel(Device& device, const std::string& modelPath, const float& scale);
private:
	
	void processNode(Device& device, aiNode* node, const aiScene* scene, const float& scale);
	Mesh processMesh(Device& device, aiMesh* mesh, const aiScene* scene, const float& scale);
	uint32_t _mipLevels;
};


Model makeSphere(Device& device, const float& scale, const std::vector<MaterialComponent>& components, const std::vector<std::string>& materialPaths);
Model makeSqaure(Device& device, const float& scale, const std::vector<MaterialComponent>& components, const std::vector<std::string>& materialPaths);
Model makeBox(Device& device, const float& scale, const std::vector<MaterialComponent>& components, const std::vector<std::string>& materialPaths);
void GenerateSphere(Device& device, Model& model,const float& scale);
void GenerateSquare(Device& device, Model& model, const float& scale);
void GenerateBox(Device& device, Model& model, const float& scale);
Model makeSkyBox(Device& device);