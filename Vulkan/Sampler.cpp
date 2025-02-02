#include "pch.h"
#include "Sampler.h"
std::vector<Sampler> Sampler::samplers = {};
Sampler::Sampler()
{
}

Sampler::Sampler(Device& device, SamplerMipMapType mipmapType, SamplerModeType modeType, SamplerFilterType filterType)
{
    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.anisotropyEnable = VK_TRUE;
    Sampler::SetMipMap(samplerInfo, mipmapType);
    Sampler::SetMode(samplerInfo, modeType);
    Sampler::SetFilter(samplerInfo, filterType);

    VkPhysicalDeviceProperties properties{};
    vkGetPhysicalDeviceProperties(device, &properties);
    samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;

    if (vkCreateSampler(device, &samplerInfo, nullptr, &_sampler) != VK_SUCCESS) {
        throw std::runtime_error("failed to create texture sampler!");
    }
}
