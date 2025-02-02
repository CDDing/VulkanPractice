
#include "pch.h"
#include "Material.h"
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
MaterialData Material::dummy = {};
Material::Material(Device& device, std::vector<MaterialComponent> components, const std::vector<std::string>& filesPath)
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
        loadImage(device, filePath, component, VK_FORMAT_R8G8B8A8_UNORM);
    }
}

void Material::destroy(Device& device)
{
	for (auto& material : _materials) {
        material.image.destroy(device);
        material.imageView.destroy(device);
	}
}

Material Material::createMaterialForSkybox(Device& device)
{
    Material material;
    material.loadImageFromDDSFile(device, L"Resources/textures/IBL/sampleEnvHDR.dds", 6);
    material.loadImageFromDDSFile(device, L"Resources/textures/IBL/sampleDiffuseHDR.dds", 6);
    material.loadImageFromDDSFile(device, L"Resources/textures/IBL/sampleSpecularHDR.dds", 6);
    material.loadImageFromDDSFile(device, L"Resources/textures/IBL/sampleBrdf.dds", 1);
    
    return material;
}

void Material::loadImage(Device& device, const std::string& filePath, const MaterialComponent component,VkFormat format)
{
    MaterialData materialData;
    int texWidth, texHeight, texChannels;
    stbi_uc* pixels = stbi_load(filePath.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
    VkDeviceSize imageSize = texWidth * texHeight * 4;
    uint32_t mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(texWidth, texHeight)))) + 1;
    if (!pixels) {
        throw std::runtime_error("failed to load texture image!");
    }

    Buffer stagingBuffer;

    stagingBuffer = Buffer(device, imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    void* data;
    vkMapMemory(device, stagingBuffer.GetMemory(), 0, imageSize, 0, &data);
    memcpy(data, pixels, static_cast<size_t>(imageSize));
    vkUnmapMemory(device, stagingBuffer.GetMemory());

    stbi_image_free(pixels);
    materialData.image = Image(device, texWidth, texHeight, mipLevels, format, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    transitionImageLayout(device, materialData.image, format, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT, mipLevels);
    copyBufferToImage(device, stagingBuffer, materialData.image, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));
    //transitionImageLayout(textureImage, format, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT, mipLevels);

    stagingBuffer.destroy(device);

    generateMipmaps(device, materialData.image, format, texWidth, texHeight, mipLevels);

    materialData.imageView = ImageView(device, materialData.image, format, VK_IMAGE_ASPECT_COLOR_BIT, mipLevels);
    //materialData.sampler = Sampler::Get(SamplerMipMapType::High);

    _materials[static_cast<int>(component)] = materialData;
}

void Material::loadImageFromDDSFile(Device& device, const std::wstring& filePath, int cnt)
{
    MaterialData materialData;
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
    stagingBuffer = Buffer(device, imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);



    void* data;
    vkMapMemory(device, stagingBuffer.GetMemory(), 0, imageSize, 0, &data);
    for (int i = 0; i < cnt; i++) {
        const DirectX::Image* img = imageData.GetImage(0, i, 0);

        memcpy((stbi_uc*)data + i * layerSize, img->pixels, static_cast<size_t>(layerSize));

    };
    vkUnmapMemory(device, stagingBuffer.GetMemory());


    materialData.image = Image(device, width, height, mipLevels, VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, cnt);

    if (cnt == 6) {
        transitionImageLayoutForCubemap(device, materialData.image, VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT, mipLevels);
        copyBufferToImageForCubemap(device, stagingBuffer, materialData.image, static_cast<uint32_t>(width), static_cast<uint32_t>(height), layerSize);
        //transitionImageLayoutForCubemap(device, image.image.Get(), VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT, mipLevels);


        generateMipmapsForCubemap(device, materialData.image, VK_FORMAT_R32G32B32A32_SFLOAT, width, height, mipLevels);
    }
    else {
        transitionImageLayout(device, materialData.image, VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT, mipLevels);
        copyBufferToImage(device, stagingBuffer, materialData.image, static_cast<uint32_t>(width), static_cast<uint32_t>(height));
        //transitionImageLayoutForCubemap(device, image.image.Get(), VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT, mipLevels);


        generateMipmaps(device, materialData.image, VK_FORMAT_R32G32B32A32_SFLOAT, width, height, mipLevels);

    }

    stagingBuffer.destroy(device);
    materialData.imageView = ImageView(device, materialData.image, VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_ASPECT_COLOR_BIT, mipLevels, cnt);
    //materialData.sampler = Sampler::Get(SamplerMipMapType::High);

    _materials.push_back(materialData);
}
