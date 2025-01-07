#include "pch.h"
#include "Material.h"
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
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
        loadImage(device, filePath, component);
    }
}

void Material::destroy(Device& device)
{
	for (auto& material : _materials) {

		vkDestroyImageView(device.Get(), material.imageView.Get(), nullptr);
		vkDestroyImage(device.Get(), material.image.Get(), nullptr);
		vkFreeMemory(device.Get(), material.image.GetMemory(), nullptr);
		vkDestroySampler(device.Get(), material.sampler.Get(), nullptr);
	}
}

Material Material::createMaterialForSkybox(Device& device)
{
    Material material;
    MaterialData materialData;
    int width, height, channels;
    uint32_t mipLevels;
    stbi_uc* faceData[6];
    faceData[0] = stbi_load("Resources/textures/Cubemap/right.jpg", &width, &height, &channels, STBI_rgb_alpha);
    faceData[1] = stbi_load("Resources/textures/Cubemap/left.jpg", &width, &height, &channels, STBI_rgb_alpha);
    faceData[2] = stbi_load("Resources/textures/Cubemap/top.jpg", &width, &height, &channels, STBI_rgb_alpha);
    faceData[3] = stbi_load("Resources/textures/Cubemap/bottom.jpg", &width, &height, &channels, STBI_rgb_alpha);
    faceData[4] = stbi_load("Resources/textures/Cubemap/front.jpg", &width, &height, &channels, STBI_rgb_alpha);
    faceData[5] = stbi_load("Resources/textures/Cubemap/back.jpg", &width, &height, &channels, STBI_rgb_alpha);
    VkDeviceSize imageSize = width * height * 4 * 6;
    VkDeviceSize layerSize = width * height * 4;
    mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(width, height)))) + 1;
    //mipLevels = 01;

    Buffer stagingBuffer;
    stagingBuffer = Buffer(device, imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    void* data;
    vkMapMemory(device.Get(), stagingBuffer.GetMemory(), 0, imageSize, 0, &data);
    for (int i = 0; i < 6; i++) {
        memcpy((stbi_uc*)data + i * layerSize, faceData[i], static_cast<size_t>(layerSize));

    }
    vkUnmapMemory(device.Get(), stagingBuffer.GetMemory());

    for (int i = 0; i < 6; i++) {
        stbi_image_free(faceData[i]);
    }

    materialData.image = Image(device, width, height, mipLevels, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 6);

    transitionImageLayoutForCubemap(device, materialData.image.Get(), VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT, mipLevels);
    copyBufferToImageForCubemap(device, stagingBuffer.Get(), materialData.image.Get(), static_cast<uint32_t>(width), static_cast<uint32_t>(height), layerSize);
    //transitionImageLayoutForCubemap(device, image.image.Get(), VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT, mipLevels);

    vkDestroyBuffer(device.Get(), stagingBuffer.Get(), nullptr);
    vkFreeMemory(device.Get(), stagingBuffer.GetMemory(), nullptr);

    generateMipmapsForCubemap(device, materialData.image.Get(), VK_FORMAT_R8G8B8A8_UNORM, width, height, mipLevels);


    materialData.imageView = ImageView(device, materialData.image.Get(), VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT, mipLevels, 6);
    materialData.sampler = Sampler(device, mipLevels);

    material._components.resize(static_cast<int>(MaterialComponent::END));
    material._components[static_cast<int>(MaterialComponent::ALBEDO)] = true;
    material._materials.push_back(materialData);
    return material;
}

void Material::loadImage(Device& device, const std::string& filePath, const MaterialComponent component)
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
    vkMapMemory(device.Get(), stagingBuffer.GetMemory(), 0, imageSize, 0, &data);
    memcpy(data, pixels, static_cast<size_t>(imageSize));
    vkUnmapMemory(device.Get(), stagingBuffer.GetMemory());

    stbi_image_free(pixels);
    materialData.image = Image(device, texWidth, texHeight, mipLevels, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    transitionImageLayout(device, materialData.image.Get(), VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT, mipLevels);
    copyBufferToImage(device, stagingBuffer.Get(), materialData.image.Get(), static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));
    //transitionImageLayout(textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT, mipLevels);

    vkDestroyBuffer(device.Get(), stagingBuffer.Get(), nullptr);
    vkFreeMemory(device.Get(), stagingBuffer.GetMemory(), nullptr);

    generateMipmaps(device, materialData.image.Get(), VK_FORMAT_R8G8B8A8_SRGB, texWidth, texHeight, mipLevels);

    materialData.imageView = ImageView(device, materialData.image.Get(), VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT, mipLevels);
    materialData.sampler = Sampler(device, mipLevels);

    _materials[static_cast<int>(component)] = materialData;
}
