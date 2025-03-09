#pragma once
class Material
{
public:
	static Material createMaterialForSkybox(DContext& context);
	static DImage dummy;
	static DImage GetDefaultMaterial(DContext& context) {
		const uint32_t pixelData = 0xFFFFFFFF;

		int width = 1, height = 1;
		uint32_t mipLevels = 1;
		vk::DeviceSize imageSize = 4;


		DBuffer stagingBuffer(context,
			imageSize,
			vk::BufferUsageFlagBits::eTransferSrc,
			vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);

		void* data = stagingBuffer.memory.mapMemory(0, imageSize);
		memcpy(data, &pixelData, static_cast<size_t>(imageSize));
		stagingBuffer.memory.unmapMemory();

		DImage image = DImage(context, mipLevels,
			vk::Format::eR8G8B8A8Unorm,
			vk::Extent2D(width, height),
			vk::ImageTiling::eOptimal,
			vk::ImageUsageFlagBits::eTransferSrc |
			vk::ImageUsageFlagBits::eTransferDst |
			vk::ImageUsageFlagBits::eSampled,
			vk::ImageLayout::eUndefined,
			vk::MemoryPropertyFlagBits::eDeviceLocal,
			vk::ImageAspectFlagBits::eColor);



		vk::raii::CommandBuffer cmdBuf = beginSingleTimeCommands(context);
		image.setImageLayout(cmdBuf, vk::ImageLayout::eTransferDstOptimal);
		image.copyFromBuffer(cmdBuf, stagingBuffer.buffer);
		image.setImageLayout(cmdBuf, vk::ImageLayout::eShaderReadOnlyOptimal);
		endSingleTimeCommands(context, cmdBuf);


		return image;
	}
	Material(std::nullptr_t) {};
	Material(DContext& context, std::vector<MaterialComponent> components, const std::vector<std::string>& filesPath);
	Material(const Material& other) = delete;
	Material& operator=(const Material& other) = delete;

	Material(Material&& other) = default;
	Material& operator=(Material&& other) = default;
	
	DImage& Get(MaterialComponent component) { return materials[static_cast<int>(component)]; }
	DImage& Get(int idx) { return materials[idx]; }
	bool hasComponent(int idx) { return components[idx]; }

	std::vector<vk::raii::DescriptorSet> descriptorSets = {};
private:
	std::unordered_map<int,DImage> materials = {};
	std::vector<bool> components;
	void loadImage(DContext& context, const std::string& filePath, const MaterialComponent component,vk::Format format);
	void loadImageFromDDSFile(DContext& context, const std::wstring& filePath, int cnt,int idx);
};

