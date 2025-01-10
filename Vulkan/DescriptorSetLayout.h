#pragma once

enum class ShaderType {
	DEFAULT,
	SKYBOX,
};
enum class ShaderComponent {
    UNIFORM,
    SAMPLER,
};
class DescriptorSetLayout
{
public:
	DescriptorSetLayout();
	DescriptorSetLayout(Device& device,ShaderType type);
	VkDescriptorSetLayout& Get() { return _descriptorSetLayout; }
    static VkDescriptorSetLayoutBinding inputLayoutBinding(uint32_t binding, ShaderComponent component,int cnt) {
        switch (component) {
        case ShaderComponent::UNIFORM:
            return { binding, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,static_cast<uint32_t>(cnt), VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, nullptr };
        case ShaderComponent::SAMPLER:
            return { binding, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,static_cast<uint32_t>(cnt),VK_SHADER_STAGE_FRAGMENT_BIT,nullptr };
        default:
            return VkDescriptorSetLayoutBinding({});
        }
    }
    static std::vector<VkDescriptorSetLayoutBinding> inputAttributeDescriptions(const std::vector<std::pair<ShaderComponent,int>> components) {
        std::vector<VkDescriptorSetLayoutBinding> result;
        uint32_t binding = 0;
        for (auto pair: components) {
            auto component = pair.first;
            auto cnt = pair.second;
            result.push_back(DescriptorSetLayout::inputLayoutBinding(binding, component,cnt));
            binding++;
        }
        return result;
    }
    static std::vector<std::pair<ShaderComponent,int>> GetComponents(ShaderType type) {
        std::vector<std::pair<ShaderComponent,int>> results;
        switch (type) {
        case ShaderType::DEFAULT:
            results = { {ShaderComponent::UNIFORM,1},
                {ShaderComponent::SAMPLER,5}, };
            break;
        case ShaderType::SKYBOX:
            results = { {ShaderComponent::UNIFORM,1},
                {ShaderComponent::SAMPLER,1} };
            break;
        }
        return results;
    }
private:
	VkDescriptorSetLayout _descriptorSetLayout;
};