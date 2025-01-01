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
	glm::mat4 world;

	std::vector<std::shared_ptr<Mesh>> meshes;
private:
	void processNode(aiNode* node, const aiScene* scene);
	Mesh processMesh(aiMesh* mesh, const aiScene* scene);
};

