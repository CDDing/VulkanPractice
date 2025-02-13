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
	//Member Function
public:
	Model() {};
	Model(std::shared_ptr<Device> device, const float& scale, const std::vector<MaterialComponent> components, const std::string& modelPath, const std::vector<std::string>& materialPaths,glm::mat4 transform);
	~Model();

	// ����
	Model(const Model& other)
		: meshes(other.meshes), // shared_ptr�� ���� ī��Ʈ ����
		material(other.material),
		uniformBuffers(other.uniformBuffers),
		transform(other.transform),
		descriptorSets(other.descriptorSets),
		_device(other._device), // shared_ptr ���� (���� ī��Ʈ ����)
		_mipLevels(other._mipLevels) {
	}
	Model & operator=(const Model & other) {
		if (this != &other) {
			meshes = other.meshes;
			material = other.material;
			uniformBuffers = other.uniformBuffers;
			transform = other.transform;
			descriptorSets = other.descriptorSets;
			_device = other._device;
			_mipLevels = other._mipLevels;
		}
		return *this;
	}

	// �̵�
	Model(Model&& other) noexcept
		: meshes(std::move(other.meshes)), // ���� �̵�
		material(std::move(other.material)), // Material �̵�
		uniformBuffers(std::move(other.uniformBuffers)), // ���� �̵�
		transform(std::move(other.transform)), // Transform �̵�
		descriptorSets(std::move(other.descriptorSets)), // ���� �̵�
		_device(std::move(other._device)), // shared_ptr �̵� (������ ����)
		_mipLevels(other._mipLevels) { // �⺻ Ÿ���� �� ����

		other.transform = {};
		other._mipLevels = 0; // �̵� �� �ʱ�ȭ
	}
	Model& operator=(Model&& other) noexcept {
		if (this != &other) {
			meshes = std::move(other.meshes);
			material = std::move(other.material);
			uniformBuffers = std::move(other.uniformBuffers);
			transform = std::move(other.transform);
			descriptorSets = std::move(other.descriptorSets);
			_device = std::move(other._device);
			_mipLevels = other._mipLevels;

			other.transform = {};
			other._mipLevels = 0; // �̵� �� �ʱ�ȭ
		}
		return *this;
	}
	
	void Render();

	void InitUniformBuffer(glm::mat4 transform);
	void InitDescriptorSet(DescriptorSet& descriptorSet);
	void InitDescriptorSetForSkybox(DescriptorSet& descriptorSet);
	void InitDescriptorSetForModelMatrix(DescriptorSet& desciprotrSet);
	void loadModel(const std::string& modelPath, const float& scale);

	friend Model makeSphere(std::shared_ptr<Device> device, glm::mat4 transform, const std::vector<MaterialComponent>& components, const std::vector<std::string>& materialPaths);
	friend Model makeSqaure(std::shared_ptr<Device> device, glm::mat4 transform, const std::vector<MaterialComponent>& components, const std::vector<std::string>& materialPaths);
	friend Model makeBox(std::shared_ptr<Device> device, glm::mat4 transform, const std::vector<MaterialComponent>& components, const std::vector<std::string>& materialPaths);
	friend Model makeSkyBox(std::shared_ptr<Device> device);
private:
	void processNode(aiNode* node, const aiScene* scene, const float& scale);
	Mesh processMesh(aiMesh* mesh, const aiScene* scene, const float& scale);

	//Member
public:
	std::vector<std::shared_ptr<Mesh>> meshes;
	Material material;
	std::vector<Buffer> uniformBuffers;
	Transform transform;
	std::vector<DescriptorSet> descriptorSets;
private:
	std::shared_ptr<Device> _device;
	uint32_t _mipLevels;

};


Model makeSphere(std::shared_ptr<Device> device, glm::mat4 transform, const std::vector<MaterialComponent>& components, const std::vector<std::string>& materialPaths);
Model makeSqaure(std::shared_ptr<Device> device, glm::mat4 transform, const std::vector<MaterialComponent>& components, const std::vector<std::string>& materialPaths);
Model makeBox(std::shared_ptr<Device> device, glm::mat4 transform, const std::vector<MaterialComponent>& components, const std::vector<std::string>& materialPaths);
void GenerateSphere(std::shared_ptr<Device> device, Model& model);
void GenerateSquare(std::shared_ptr<Device> device, Model& model);
void GenerateBox(std::shared_ptr<Device> device, Model& model);
Model makeSkyBox(std::shared_ptr<Device> device);