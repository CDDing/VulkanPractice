#include "pch.h"
#include "Sampler.h"
std::vector<vk::raii::Sampler> Sampler::samplers = {};

vk::raii::Sampler createSampler(Device& device, SamplerMipMapType mipmapType, SamplerModeType modeType, SamplerFilterType filterType)
{
    vk::SamplerCreateInfo samplerInfo{};
    samplerInfo.anisotropyEnable = VK_TRUE;
    Sampler::SetMipMap(samplerInfo, mipmapType);
    Sampler::SetMode(samplerInfo, modeType);
    Sampler::SetFilter(samplerInfo, filterType);

    VkPhysicalDeviceProperties properties = device.physical.getProperties();
    samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
    samplerInfo.borderColor = vk::BorderColor::eIntOpaqueBlack;
    samplerInfo.unnormalizedCoordinates = vk::False;
    samplerInfo.compareEnable = vk::False;
    samplerInfo.compareOp = vk::CompareOp::eAlways;

    auto sampler = device.logical.createSampler(samplerInfo);

    return sampler;
}
