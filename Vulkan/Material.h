#pragma once
class Material
{
public:

	Material() {};
	Material(std::shared_ptr<Device> device, std::vector<MaterialComponent> components, const std::vector<std::string>& filesPath);
	void destroy(std::shared_ptr<Device> device);
	ImageSet& Get(MaterialComponent component) { return _materials[static_cast<int>(component)]; }
	ImageSet& Get(int idx) { return _materials[idx]; }
	bool hasComponent(int idx) { return _components[idx]; }

	static Material createMaterialForSkybox(std::shared_ptr<Device> device);
	static ImageSet dummy;
	std::vector<DescriptorSet> descriptorSets;
	static ImageSet GetDefaultMaterial(std::shared_ptr<Device> device){
		const uint32_t pixelData = 0xFFFFFFFF;

		int width = 1, height = 1;
		uint32_t mipLevels = 1;
		

		Buffer stagingBuffer;
		vk::DeviceSize imageSize = 4;
		stagingBuffer = Buffer(device, imageSize, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible| vk::MemoryPropertyFlagBits::eHostCoherent);

		void* data = device->logical.mapMemory(stagingBuffer.GetMemory(), 0, imageSize);
		memcpy(data, &pixelData, static_cast<size_t>(imageSize));
		device->logical.unmapMemory(stagingBuffer.GetMemory());

		Image image= Image(device,width, height, mipLevels, 
			vk::Format::eR8G8B8A8Unorm,
			vk::ImageTiling::eOptimal,
			vk::ImageUsageFlagBits::eTransferSrc| 
			vk::ImageUsageFlagBits::eTransferDst|
			vk::ImageUsageFlagBits::eSampled,
			vk::MemoryPropertyFlagBits::eDeviceLocal);


		vk::CommandBuffer cmdBuf = beginSingleTimeCommands(device);
		image.transitionLayout(device, cmdBuf,vk::ImageLayout::eTransferDstOptimal);
		endSingleTimeCommands(device, cmdBuf);

		copyBufferToImage(device, stagingBuffer, image, static_cast<uint32_t>(width), static_cast<uint32_t>(height));

		cmdBuf = beginSingleTimeCommands(device);
		image.transitionLayout(device, cmdBuf,vk::ImageLayout::eShaderReadOnlyOptimal);
		endSingleTimeCommands(device, cmdBuf);

		stagingBuffer.destroy(device);

		ImageView imageView = ImageView(device, image, vk::Format::eR8G8B8A8Unorm, vk::ImageAspectFlagBits::eColor, mipLevels);
		//materialData.sampler = Sampler::Get(SamplerMipMapType::Low);

		ImageSet materialData;
		materialData.image = image;
		materialData.imageView = imageView;
		return materialData;
	}
private:
	std::vector<ImageSet> _materials;
	std::vector<bool> _components;
	void loadImage(std::shared_ptr<Device> device, const std::string& filePath, const MaterialComponent component,vk::Format format);
	void loadImageFromDDSFile(std::shared_ptr<Device> device, const std::wstring& filePath, int cnt);
};

