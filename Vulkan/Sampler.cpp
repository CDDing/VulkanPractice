#include "pch.h"
#include "Sampler.h"
std::vector<Sampler> Sampler::samplers = {};
Sampler::Sampler()
{
}

Sampler::Sampler(Device& device, SamplerMipMapType mipmapType, SamplerModeType modeType, SamplerFilterType filterType)
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

    _sampler = device.logical.createSampler(samplerInfo);
}
