#include "pch.h"
#include "Skybox.h"

void Skybox::InitDescriptorSet(std::shared_ptr<Device> device)
{
    for (size_t frame = 0; frame < MAX_FRAMES_IN_FLIGHT; frame++) {
        std::vector<vk::DescriptorImageInfo> imageInfos(4);
        for (int i = 0; i < 3; i++) {
            auto& imageInfo = imageInfos[i];
            imageInfo.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
            imageInfo.imageView = material.Get(i).imageView;
            imageInfo.sampler = Sampler::Get(SamplerMipMapType::High);
        }
        vk::WriteDescriptorSet descriptorWriteForMap{};
        descriptorWriteForMap.dstSet = material.descriptorSets[frame];
        descriptorWriteForMap.dstBinding = 0;
        descriptorWriteForMap.dstArrayElement = 0;
        descriptorWriteForMap.descriptorType = vk::DescriptorType::eCombinedImageSampler;
        descriptorWriteForMap.descriptorCount = 3;
        descriptorWriteForMap.pImageInfo = imageInfos.data();


        vk::DescriptorImageInfo lutImageInfo{};
        lutImageInfo.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
        lutImageInfo.imageView = material.Get(3).imageView;
        lutImageInfo.sampler = Sampler::Get(SamplerMipMapType::High);
        imageInfos[3] = lutImageInfo;
        vk::WriteDescriptorSet descriptorWriteForLut{};
        descriptorWriteForLut.dstSet = material.descriptorSets[frame];
        descriptorWriteForLut.dstBinding = 1;
        descriptorWriteForLut.dstArrayElement = 0;
        descriptorWriteForLut.descriptorType = vk::DescriptorType::eCombinedImageSampler;
        descriptorWriteForLut.descriptorCount = 1;
        descriptorWriteForLut.pImageInfo = &lutImageInfo;

        std::vector<vk::WriteDescriptorSet> descriptorWrites;
        descriptorWrites = { descriptorWriteForMap,descriptorWriteForLut };
        device->logical.updateDescriptorSets(descriptorWrites, nullptr);
    }
}

Skybox makeSkyBox(std::shared_ptr<Device> device)
{
    Skybox model;
    GenerateBox(device, model);
    model.material = Material::createMaterialForSkybox(device);



    return model;
}
