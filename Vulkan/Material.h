#pragma once
enum class MaterialComponent {
	ALBEDO,
	NORMAL,
	ROUGHNESS,
	METALNESS,
	ao,
	END,
};
class Material
{
public:

	Material() {};
	Material(Device& device, std::vector<MaterialComponent> components, const std::vector<std::string>& filesPath);
	void destroy(Device& device);
	ImageSet& Get(MaterialComponent component) { return _materials[static_cast<int>(component)]; }
	ImageSet& Get(int idx) { return _materials[idx]; }
	bool hasComponent(int idx) { return _components[idx]; }

	static Material createMaterialForSkybox(Device& device);
	static ImageSet dummy;
	std::vector<DescriptorSet> descriptorSets;
	static ImageSet GetDefaultMaterial(Device& device){
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

		Image image= Image(device,width, height, mipLevels, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);


		image.transitionLayout(device, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT);
		copyBufferToImage(device, stagingBuffer, image, static_cast<uint32_t>(width), static_cast<uint32_t>(height));
		image.transitionLayout(device, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT);


		vkDestroyBuffer(device, stagingBuffer, nullptr);
		vkFreeMemory(device, stagingBuffer.GetMemory(), nullptr);

		ImageView imageView = ImageView(device, image, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT, mipLevels);
		//materialData.sampler = Sampler::Get(SamplerMipMapType::Low);

		ImageSet materialData(image,imageView);
		return materialData;
	}
private:
	std::vector<ImageSet> _materials;
	std::vector<bool> _components;
	void loadImage(Device& device, const std::string& filePath, const MaterialComponent component,VkFormat format);
	void loadImageFromDDSFile(Device& device, const std::wstring& filePath, int cnt);
};

