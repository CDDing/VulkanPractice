
#include "pch.h"
#include "Material.h"
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
DImage Material::dummy(nullptr);
Material::Material(DContext& context, std::vector<MaterialComponent> components, const std::vector<std::string>& filesPath) 
{
	this->components.resize(static_cast<int>(MaterialComponent::END));
	for (auto component : components) {
		this->components[static_cast<int>(component)] = true;
	}


    //Match components and filesPath size
    for (int i = 0; i < components.size(); i++) {
        auto filePath = filesPath[i];
        auto component = components[i];
        loadImage(context, filePath, component, vk::Format::eR8G8B8A8Unorm);
    }
}

Material Material::createMaterialForSkybox(DContext& context)
{
    Material material(nullptr);
    material.loadImageFromDDSFile(context, L"Resources/textures/IBL/sampleEnvHDR.dds", 6, 0);
    material.loadImageFromDDSFile(context, L"Resources/textures/IBL/sampleDiffuseHDR.dds", 6, 1);
    material.loadImageFromDDSFile(context, L"Resources/textures/IBL/sampleSpecularHDR.dds", 6, 2);
    material.loadImageFromDDSFile(context, L"Resources/textures/IBL/sampleBrdf.dds", 1, 3);
    
    return material;
}

void Material::loadImage(DContext& context, const std::string& filePath, const MaterialComponent component,vk::Format format)
{
    int texWidth, texHeight, texChannels;
    stbi_uc* pixels = stbi_load(filePath.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
    vk::DeviceSize imageSize = texWidth * texHeight * 4;
    uint32_t mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(texWidth, texHeight)))) + 1;
    if (!pixels) {
        throw std::runtime_error("failed to load texture image!");
    }


    DImage materialData(context,mipLevels,format,
        vk::Extent2D(texWidth,texHeight),
        vk::ImageTiling::eOptimal,
        vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled,
        vk::ImageLayout::eUndefined,
        vk::MemoryPropertyFlagBits::eDeviceLocal,
        vk::ImageAspectFlagBits::eColor);
    
    DBuffer stagingBuffer(context, imageSize, vk::BufferUsageFlagBits::eTransferSrc,
        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);

    stagingBuffer.map(imageSize, 0);
    memcpy(stagingBuffer.mapped, pixels, static_cast<size_t>(imageSize));
    stagingBuffer.unmap();

    vk::raii::CommandBuffer cmdBuf = beginSingleTimeCommands(context);
	materialData.setImageLayout(cmdBuf, vk::ImageLayout::eTransferDstOptimal);
	materialData.copyFromBuffer(cmdBuf, stagingBuffer.buffer);
	materialData.generateMipmaps(cmdBuf);
	endSingleTimeCommands(context, cmdBuf);

    stbi_image_free(pixels);
    materials[static_cast<int>(component)] = std::move(materialData);
}

void Material::loadImageFromDDSFile(DContext& context, const std::wstring& filePath, int cnt, int idx)
{
    int width, height;
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

    DBuffer stagingBuffer(context, imageSize, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible| vk::MemoryPropertyFlagBits::eHostCoherent);



    stagingBuffer.map(imageSize, 0);
    for (int i = 0; i < cnt; i++) {
        const DirectX::Image* img = imageData.GetImage(0, i, 0);

        memcpy((stbi_uc*)stagingBuffer.mapped + i * layerSize, img->pixels, static_cast<size_t>(layerSize));

    };
    stagingBuffer.unmap();

	vk::raii::CommandBuffer commandBuffer = beginSingleTimeCommands(context);
    CubemapImage ci(nullptr);
	DImage di(nullptr);
    switch (cnt) {
    case 6:

        ci = CubemapImage(context,layerSize,mipLevels,vk::Format::eR32G32B32A32Sfloat,
            vk::Extent2D(width,height),
            vk::ImageTiling::eOptimal,
			vk::ImageUsageFlagBits::eTransferSrc |
			vk::ImageUsageFlagBits::eTransferDst |
			vk::ImageUsageFlagBits::eSampled,
			vk::ImageLayout::eUndefined,
			vk::MemoryPropertyFlagBits::eDeviceLocal,
			vk::ImageAspectFlagBits::eColor);
		ci.setImageLayout(commandBuffer, vk::ImageLayout::eTransferDstOptimal);
		ci.copyFromBuffer(commandBuffer, stagingBuffer.buffer);
		ci.generateMipmaps(commandBuffer);

		//TODO 이미지 벡터에 넣기
        //materials.push_back(std::move(ci));
        materials[idx] = std::move(ci);
		endSingleTimeCommands(context, commandBuffer);
        break;
    case 1:

        di = DImage(context,mipLevels,vk::Format::eR32G32B32A32Sfloat,
            vk::Extent2D(width,height),
            vk::ImageTiling::eOptimal,
            vk::ImageUsageFlagBits::eTransferSrc|
            vk::ImageUsageFlagBits::eTransferDst|
            vk::ImageUsageFlagBits::eSampled,
            vk::ImageLayout::eUndefined,
            vk::MemoryPropertyFlagBits::eDeviceLocal,
            vk::ImageAspectFlagBits::eColor);
        di.setImageLayout(commandBuffer, vk::ImageLayout::eTransferDstOptimal);
        di.copyFromBuffer(commandBuffer, stagingBuffer.buffer);
        di.generateMipmaps(commandBuffer);

        //TODO 이미지 벡터에 넣기
        materials[idx]=(std::move(di));
		endSingleTimeCommands(context, commandBuffer);
        break;
    }
}
