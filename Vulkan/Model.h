#pragma once
#include <assimp\Importer.hpp>
#include <assimp\postprocess.h>
#include <assimp\scene.h>
class Mesh;
class Model
{
public:
	Model() {};
	Model(Device& device, const std::string& path);

	void Render();
	void deleteModel(Device& device);
	glm::mat4 world = glm::mat4();

	std::vector<std::shared_ptr<Mesh>> meshes;
private:
	void processNode(Device& device, aiNode* node, const aiScene* scene);
	Mesh processMesh(Device& device, aiMesh* mesh, const aiScene* scene);

};

