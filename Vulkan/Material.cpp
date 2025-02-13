
#include "pch.h"
#include "Material.h"
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
ImageSet Material::dummy = {};
Material::Material(std::shared_ptr<Device> device, std::vector<MaterialComponent> components, const std::vector<std::string>& filesPath) : _device(device)
{
	_components.resize(static_cast<int>(MaterialComponent::END));
    _materials.resize(static_cast<int>(MaterialComponent::END));
	for (auto component : components) {
		_components[static_cast<int>(component)] = true;
	}


    //Match components and filesPath size
    for (int i = 0; i < components.size(); i++) {
        auto filePath = filesPath[i];
        auto component = components[i];
        loadImage(filePath, component, vk::Format::eR8G8B8A8Unorm);
    }
}

Material::~Material()
{
	for (auto& material : _materials) {
        material.~ImageSet();
	}
}

Material Material::createMaterialForSkybox(std::shared_ptr<Device> device)
{
    Material material;
    material._device = device;
    material.loadImageFromDDSFile(L"Resources/textures/IBL/sampleEnvHDR.dds", 6);
    material.loadImageFromDDSFile(L"Resources/textures/IBL/sampleDiffuseHDR.dds", 6);
    material.loadImageFromDDSFile(L"Resources/textures/IBL/sampleSpecularHDR.dds", 6);
    material.loadImageFromDDSFile(L"Resources/textures/IBL/sampleBrdf.dds", 1);
    
    return material;
}

void Material::loadImage(const std::string& filePath, const MaterialComponent component,vk::Format format)
{
    int texWidth, texHeight, texChannels;
    stbi_uc* pixels = stbi_load(filePath.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
    vk::DeviceSize imageSize = texWidth * texHeight * 4;
    uint32_t mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(texWidth, texHeight)))) + 1;
    if (!pixels) {
        throw std::runtime_error("failed to load texture image!");
    }

    _materials[static_cast<int>(component)] = ImageSet(_device,
        texWidth, texHeight, mipLevels, format,
        vk::ImageTiling::eOptimal,
        vk::ImageUsageFlagBits::eTransferSrc| vk::ImageUsageFlagBits::eTransferDst| vk::ImageUsageFlagBits::eSampled,
        vk::MemoryPropertyFlagBits::eDeviceLocal,
        vk::ImageAspectFlagBits::eColor);

    _materials[static_cast<int>(component)].image.fillImage(pixels, imageSize);
    _materials[static_cast<int>(component)].image.generateMipmaps();


    stbi_image_free(pixels);
}

void Material::loadImageFromDDSFile(const std::wstring& filePath, int cnt)
{
    ImageSet materialData;
    int width, height, channels;
    uint32_t mipLevels;
    DirectX::ScratchImage imageData;
    DirectX::LoadFromDDSFile(filePath.c_str(), DirectX::DDS_FLAGS_NONE, nullptr, imageData);
    DirectX::TexMetadata metadata = imageData.GetMetadata();
    width = metadata.width;
    height = metadata.height;

   
    size_t bpp = DirectX::BitsPerPixel(metadata.format);
    VkDeviceSize layerSize = width * height * bpp / 8;
    VkDeviceSize imageSize = layerSize * cnt;
    mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(width, height)))) + 1;
    //mipLevels = 01;

    Buffer stagingBuffer;
    stagingBuffer = Buffer(_device, imageSize, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible| vk::MemoryPropertyFlagBits::eHostCoherent);



    stagingBuffer.map(imageSize, 0);
    for (int i = 0; i < cnt; i++) {
        const DirectX::Image* img = imageData.GetImage(0, i, 0);

        memcpy((stbi_uc*)stagingBuffer.mapped + i * layerSize, img->pixels, static_cast<size_t>(layerSize));

    };
    stagingBuffer.unmap();

    materialData.image = Image(_device, width, height, mipLevels, vk::Format::eR32G32B32A32Sfloat, vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eTransferSrc| vk::ImageUsageFlagBits::eTransferDst| vk::ImageUsageFlagBits::eSampled, vk::MemoryPropertyFlagBits::eDeviceLocal, cnt);

    if (cnt == 6) {
        transitionImageLayoutForCubemap(_device, materialData.image, vk::Format::eR32G32B32A32Sfloat, vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal, vk::ImageAspectFlagBits::eColor, mipLevels);
        copyBufferToImageForCubemap(_device, stagingBuffer, materialData.image, static_cast<uint32_t>(width), static_cast<uint32_t>(height), layerSize);
        //transitionImageLayoutForCubemap(device, image.image.Get(), VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT, mipLevels);


        generateMipmapsForCubemap(_device, materialData.image, vk::Format::eR32G32B32A32Sfloat, width, height, mipLevels);
    }
    else {
        VkCommandBuffer cmdBuf = beginSingleTimeCommands(_device);
        materialData.image.transitionLayout(cmdBuf, vk::ImageLayout::eTransferDstOptimal, vk::ImageAspectFlagBits::eColor);
        endSingleTimeCommands(_device, cmdBuf);
        copyBufferToImage(_device, stagingBuffer, materialData.image, static_cast<uint32_t>(width), static_cast<uint32_t>(height));
        //transitionImageLayoutForCubemap(device, image.image.Get(), VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT, mipLevels);


        materialData.image.generateMipmaps();
    }
    
    materialData.imageView = ImageView(_device, materialData.image, vk::Format::eR32G32B32A32Sfloat, vk::ImageAspectFlagBits::eColor, mipLevels, cnt);
    //materialData.sampler = Sampler::Get(SamplerMipMapType::High);

    _materials.push_back(std::move(materialData));
}
