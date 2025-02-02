#pragma once
enum class SamplerMipMapType {
	Low,
	High,

	END

};
enum class SamplerModeType {
	Clamp,

	END
};
enum class SamplerFilterType {
	Linear,
	Nearest,

	END

};
class Sampler
{
public:
	Sampler();
	Sampler(Device& device, SamplerMipMapType mipmapType, SamplerModeType modeType, SamplerFilterType filterType);
	operator VkSampler& () {
		return _sampler;
	}
	void destroy(Device& device) {
		vkDestroySampler(device, _sampler, nullptr);
	}
	static void init(Device& device) {
		auto maxValue0 = static_cast<int>(SamplerMipMapType::END);
		auto maxValue1 = static_cast<int>(SamplerFilterType::END);
		auto maxValue2 = static_cast<int>(SamplerModeType::END);
		auto maxValue = maxValue0 * maxValue1 * maxValue2;


		for (auto i = 0; i < maxValue; i++) {
			samplers.push_back(Sampler(device, GetMipMapType(i),
				GetModeType(i),
				GetFilterType(i)));
		}
	}
	static void destroySamplers(Device& device) {
		for (auto& sampler : samplers) {
			sampler.destroy(device);
		}
	}
	static Sampler& Get(SamplerMipMapType mipmapType = SamplerMipMapType::Low, 
		SamplerModeType modeType = SamplerModeType::Clamp, 
		SamplerFilterType filterType = SamplerFilterType::Linear) {
		auto index = GetIndex(mipmapType, modeType, filterType);
		return samplers[index];
	}
private:

	//Index
	static int GetIndex(SamplerMipMapType mipmapType, SamplerModeType modeType, SamplerFilterType filterType) {
		int value0 = static_cast<int>(mipmapType);
		int value1 = static_cast<int>(modeType);
		int value2 = static_cast<int>(filterType);
		return value0 +
			value1 * static_cast<int>(SamplerMipMapType::END) +
			value2 * static_cast<int>(SamplerMipMapType::END) * static_cast<int>(SamplerModeType::END);
	}
	static SamplerMipMapType GetMipMapType(int index) {
		return static_cast<SamplerMipMapType>(index % static_cast<int>(SamplerMipMapType::END));
	}

	static SamplerModeType GetModeType(int index) {
		return static_cast<SamplerModeType>((index / static_cast<int>(SamplerMipMapType::END)) % static_cast<int>(SamplerModeType::END));
	}

	static SamplerFilterType GetFilterType(int index) {
		return static_cast<SamplerFilterType>(index / (static_cast<int>(SamplerMipMapType::END) * static_cast<int>(SamplerModeType::END)));
	}

	static void SetMode(VkSamplerCreateInfo& createInfo,SamplerModeType modeType) {
		switch (modeType ){
		case SamplerModeType::Clamp:
			createInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
			createInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
			createInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
				break;
			default:
				break;
		}
	}
	static void SetMipMap(VkSamplerCreateInfo& createInfo, SamplerMipMapType mipmapType) {
		switch (mipmapType) {
		case SamplerMipMapType::Low:
			createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
			createInfo.mipLodBias = 0.0f;
			createInfo.minLod = 0.0f;
			createInfo.maxLod = 1.0f;
			break;
		case SamplerMipMapType::High:
			createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
			createInfo.mipLodBias = 0.0f;
			createInfo.minLod = 0.0f;
			createInfo.maxLod = 15.0f;

			break;
		default:
			break;
		}
	}
	static void SetFilter(VkSamplerCreateInfo& createInfo, SamplerFilterType filterType) {
		switch (filterType) {
		case SamplerFilterType::Linear:
			createInfo.magFilter = VK_FILTER_LINEAR;
			createInfo.minFilter = VK_FILTER_LINEAR;
			break;
		case SamplerFilterType::Nearest:
			createInfo.magFilter = VK_FILTER_NEAREST;
			createInfo.minFilter = VK_FILTER_NEAREST;
		default:
			break;
		}
	}
	VkSampler _sampler;

	static std::vector<Sampler> samplers;
};
