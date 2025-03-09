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
vk::raii::Sampler createSampler(DContext& context, SamplerMipMapType mipmapType, SamplerModeType modeType, SamplerFilterType filterType);
class Sampler
{
public:
	static std::vector<vk::raii::Sampler> samplers;
	static void destroy(DContext& context) {
		for (auto& sampler : samplers) {
			sampler.~Sampler();
		}
	}
	static void init(DContext& context) {
		auto maxValue0 = static_cast<int>(SamplerMipMapType::END);
		auto maxValue1 = static_cast<int>(SamplerFilterType::END);
		auto maxValue2 = static_cast<int>(SamplerModeType::END);
		auto maxValue = maxValue0 * maxValue1 * maxValue2;


		for (auto i = 0; i < maxValue; i++) {
			samplers.push_back(createSampler(context, GetMipMapType(i),
				GetModeType(i),
				GetFilterType(i)));
		}
	}
	static vk::raii::Sampler& Get(SamplerMipMapType mipmapType = SamplerMipMapType::Low, 
		SamplerModeType modeType = SamplerModeType::Clamp, 
		SamplerFilterType filterType = SamplerFilterType::Linear) {
		auto index = GetIndex(mipmapType, modeType, filterType);
		return samplers[index];
	}

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

	static void SetMode(vk::SamplerCreateInfo& createInfo,SamplerModeType modeType) {
		switch (modeType ){
		case SamplerModeType::Clamp:
			createInfo.addressModeU = vk::SamplerAddressMode::eClampToEdge;
			createInfo.addressModeV = vk::SamplerAddressMode::eClampToEdge;
			createInfo.addressModeW = vk::SamplerAddressMode::eClampToEdge;
				break;
			default:
				break;
		}
	}
	static void SetMipMap(vk::SamplerCreateInfo& createInfo, SamplerMipMapType mipmapType) {
		switch (mipmapType) {
		case SamplerMipMapType::Low:
			createInfo.mipmapMode = vk::SamplerMipmapMode::eLinear;
			createInfo.mipLodBias = 0.0f;
			createInfo.minLod = 0.0f;
			createInfo.maxLod = 1.0f;
			break;
		case SamplerMipMapType::High:
			createInfo.mipmapMode = vk::SamplerMipmapMode::eLinear;
			createInfo.mipLodBias = 0.0f;
			createInfo.minLod = 0.0f;
			createInfo.maxLod = 15.0f;

			break;
		default:
			break;
		}
	}
	static void SetFilter(vk::SamplerCreateInfo& createInfo, SamplerFilterType filterType) {
		switch (filterType) {
		case SamplerFilterType::Linear:
			createInfo.magFilter = vk::Filter::eLinear;
			createInfo.minFilter = vk::Filter::eLinear;
			break;
		case SamplerFilterType::Nearest:
			createInfo.magFilter = vk::Filter::eNearest;
			createInfo.minFilter = vk::Filter::eNearest;
			break;
		default:
			break;
		}
	}
};