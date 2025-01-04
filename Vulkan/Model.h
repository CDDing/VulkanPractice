#pragma once
#include <assimp\Importer.hpp>
#include <assimp\postprocess.h>
#include <assimp\scene.h>
class Mesh;
enum ImageType {
	TEXTURE,
	NORMALMAP,
	HEIGHTMAP,
};
class Model
{
public:
	Model() {};
	Model(Device& device, const std::string& modelPath);
	Model(Device& device, const std::string& modelPath, const std::string& texturePath);
	Model(Device& device, const std::string& modelPath, const std::string& texturePath, const std::string& normalMapPath);


	void Render();
	void deleteModel(Device& device);
	glm::mat4 world = glm::mat4();

	std::vector<std::shared_ptr<Mesh>> meshes;
	std::vector<Texture> images;
	void loadModel(Device& device, const std::string& modelPath);
	void loadImage(Device& device, const std::string& filePath);
private:
	
	void processNode(Device& device, aiNode* node, const aiScene* scene);
	Mesh processMesh(Device& device, aiMesh* mesh, const aiScene* scene);
	uint32_t _mipLevels;
};


Model makeSphere(Device& device, const float& scale, const std::string& texturePath);
Model makeSphere(Device& device, const float& scale, const std::string& texturePath, const std::string& normalMapPath);
Model makeSqaure(Device& device, const float& scale, const std::string& texturePath);
Model makeSqaure(Device& device, const float& scale, const std::string& texturePath, const std::string& normalMapPath);
void GenerateSphere(Device& device, Model& model,const float& scale);
void GenerateSquare(Device& device, Model& model, const float& scale);