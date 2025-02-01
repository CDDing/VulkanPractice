#pragma once
enum class MaterialComponent {
	ALBEDO,
	NORMAL,
	ROUGHNESS,
	METALNESS,
	ao,
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
	static MaterialData dummy;
	std::vector<DescriptorSet> descriptorSets;
	static MaterialData GetDefaultMaterial(Device& device){
		MaterialData materialData;
		const uint32_t pixelData = 0xFFFFFFFF;

		int width = 1, height = 1;
		uint32_t mipLevels = 1;
		

		Buffer stagingBuffer;
		VkDeviceSize imageSize = 4;
		stagingBuffer = Buffer(device, imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

		void* data;
		vkMapMemory(device, stagingBuffer.GetMemory(), 0, imageSize, 0, &data);
		memcpy(data, &pixelData, static_cast<size_t>(imageSize));
		vkUnmapMemory(device, stagingBuffer.GetMemory());

		materialData.image = Image(device,width, height, mipLevels, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

		transitionImageLayout(device, materialData.image.Get(), VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT, mipLevels);
		copyBufferToImage(device, stagingBuffer.Get(), materialData.image.Get(), static_cast<uint32_t>(width), static_cast<uint32_t>(height));
		transitionImageLayout(device, materialData.image.Get(), VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT, mipLevels);

		vkDestroyBuffer(device, stagingBuffer.Get(), nullptr);
		vkFreeMemory(device, stagingBuffer.GetMemory(), nullptr);

		materialData.imageView = ImageView(device, materialData.image.Get(), VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT, mipLevels);
		materialData.sampler = Sampler(device, mipLevels);

		return materialData;
	}
private:
	std::vector<MaterialData> _materials;
	std::vector<bool> _components;
	void loadImage(Device& device, const std::string& filePath, const MaterialComponent component,VkFormat format);
	void loadImageFromDDSFile(Device& device, const std::wstring& filePath, int cnt);
};

