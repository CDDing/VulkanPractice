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
    static VkDescriptorSetLayoutBinding inputLayoutBinding(uint32_t binding, ShaderComponent component) {
        switch (component) {
        case ShaderComponent::UNIFORM:
            return { binding, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,1, VK_SHADER_STAGE_VERTEX_BIT, nullptr };
        case ShaderComponent::SAMPLER:
            return { binding, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,1,VK_SHADER_STAGE_FRAGMENT_BIT,nullptr };
        default:
            return VkDescriptorSetLayoutBinding({});
        }
    }
    static std::vector<VkDescriptorSetLayoutBinding> inputAttributeDescriptions(const std::vector<ShaderComponent> components) {
        std::vector<VkDescriptorSetLayoutBinding> result;
        uint32_t binding = 0;
        for (ShaderComponent component : components) {
            result.push_back(DescriptorSetLayout::inputLayoutBinding(binding, component));
            binding++;
        }
        return result;
    }
private:
	VkDescriptorSetLayout _descriptorSetLayout;
};