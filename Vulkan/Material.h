#pragma once
enum class MaterialComponent {
	TEXTURE,
	NORMAL,
	ROUGHNESS,
	METALNESS,
	END,
};
struct MaterialData {
		Image image;
		ImageView imageView;
		Sampler sampler;
	};
class Material
{
public:

	Material() {};
	Material(Device& device, std::vector<MaterialComponent> components, const std::vector<std::string>& filesPath);
	void destroy(Device& device);
	MaterialData& Get(MaterialComponent component) { return _materials[static_cast<int>(component)]; }
	MaterialData& Get(int idx) { return _materials[idx]; }
	bool hasComponent(int idx) { return _components[idx]; }

	static Material createMaterialForSkybox(Device& device);

	std::vector<DescriptorSet> descriptorSets;
private:
	std::vector<MaterialData> _materials;
	std::vector<bool> _components;
	void loadImage(Device& device, const std::string& filePath, const MaterialComponent component);
};

