#pragma once
class Material
{
	//static
public:
	static ImageSet GetDefaultMaterial(std::shared_ptr<Device> device) {
		const uint32_t pixelData = 0xFFFFFFFF;

		ImageSet materialData;
		int width = 1, height = 1;
		uint32_t mipLevels = 1;


		Buffer stagingBuffer;
		vk::DeviceSize imageSize = 4;
		stagingBuffer = Buffer(device, imageSize, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);

		stagingBuffer.map(imageSize, 0);
		memcpy(stagingBuffer.mapped, &pixelData, static_cast<size_t>(imageSize));
		stagingBuffer.unmap();

		materialData.image = Image(device, width, height, mipLevels,
			vk::Format::eR8G8B8A8Unorm,
			vk::ImageTiling::eOptimal,
			vk::ImageUsageFlagBits::eTransferSrc |
			vk::ImageUsageFlagBits::eTransferDst |
			vk::ImageUsageFlagBits::eSampled,
			vk::MemoryPropertyFlagBits::eDeviceLocal);


		vk::CommandBuffer cmdBuf = beginSingleTimeCommands(device);
		materialData.image.transitionLayout(cmdBuf, vk::ImageLayout::eTransferDstOptimal);
		endSingleTimeCommands(device, cmdBuf);

		copyBufferToImage(device, stagingBuffer, materialData.image, static_cast<uint32_t>(width), static_cast<uint32_t>(height));

		cmdBuf = beginSingleTimeCommands(device);
		materialData.image.transitionLayout(cmdBuf, vk::ImageLayout::eShaderReadOnlyOptimal);
		endSingleTimeCommands(device, cmdBuf);


		materialData.imageView = ImageView(device, materialData.image, vk::Format::eR8G8B8A8Unorm, vk::ImageAspectFlagBits::eColor, mipLevels);
		//materialData.sampler = Sampler::Get(SamplerMipMapType::Low);

		return materialData;
	}
	static Material createMaterialForSkybox(std::shared_ptr<Device> device);
	static ImageSet dummy;
	
	//Member Function
public:
	
	Material() {};
	Material(std::shared_ptr<Device> device, std::vector<MaterialComponent> components, const std::vector<std::string>& filesPath);
	~Material();
	
	// 복사
	Material(const Material& other)
		: descriptorSets(other.descriptorSets),
		_device(other._device), // shared_ptr 복사 (참조 카운트 증가)
		_materials(other._materials),
		_components(other._components) {
	}
	Material& operator=(const Material& other) {
		if (this != &other) {
			descriptorSets = other.descriptorSets;
			_device = other._device;
			_materials = other._materials;
			_components = other._components;
		}
		return *this;
	}

	// 이동
	Material(Material&& other) noexcept
		: descriptorSets(std::move(other.descriptorSets)),
		_device(std::move(other._device)), // shared_ptr 이동 (소유권 이전)
		_materials(std::move(other._materials)),
		_components(std::move(other._components)) {
	}
	Material& operator=(Material&& other) noexcept {
		if (this != &other) {
			descriptorSets = std::move(other.descriptorSets);
			_device = std::move(other._device);
			_materials = std::move(other._materials);
			_components = std::move(other._components);
		}
		return *this;
	}

	ImageSet& Get(MaterialComponent component) { return _materials[static_cast<int>(component)]; }
	ImageSet& Get(int idx) { return _materials[idx]; }
	bool hasComponent(int idx) { return _components[idx]; }

private:
	void loadImage(const std::string& filePath, const MaterialComponent component,vk::Format format);
	void loadImageFromDDSFile(const std::wstring& filePath, int cnt);

	//Member
public:
	std::vector<DescriptorSet> descriptorSets;

private:
	std::shared_ptr<Device> _device;
	std::vector<ImageSet> _materials;
	std::vector<bool> _components;
};

