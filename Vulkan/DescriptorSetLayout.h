#pragma once

enum class ShaderType {
	DEFAULT,
	SKYBOX,
    DEFERRED,
};
enum class ShaderComponent {
    UNIFORM_BUFFER,
    STORAGE_BUFFER,
    COMBINED_IMAGE,
    STORAGE_IMAGE,
    AS
};
class DescriptorSetLayout
{
public:
	DescriptorSetLayout();
	DescriptorSetLayout(Device& device,DescriptorType type);
    operator VkDescriptorSetLayout& () {
        return _descriptorSetLayout;
    }
    VkDescriptorSetLayout* operator&() {
        return &_descriptorSetLayout;
    }
    void destroy(Device& device) {
        vkDestroyDescriptorSetLayout(device, _descriptorSetLayout, nullptr);
    }
    static VkDescriptorSetLayoutBinding inputLayoutBinding(uint32_t binding, ShaderComponent component,int cnt,VkShaderStageFlags shaderStageFlags) {
        uint32_t descriptorCount = static_cast<uint32_t>(cnt);
        switch (component) {
        case ShaderComponent::UNIFORM_BUFFER:
            return { binding, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,descriptorCount, shaderStageFlags, nullptr };
        case ShaderComponent::STORAGE_BUFFER:
            return { binding,VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,descriptorCount,shaderStageFlags,nullptr };
        case ShaderComponent::COMBINED_IMAGE:
            return { binding, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,descriptorCount,shaderStageFlags,nullptr };
        case ShaderComponent::AS:
            return { binding, VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR,descriptorCount,shaderStageFlags,nullptr };
        case ShaderComponent::STORAGE_IMAGE:
            return { binding, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,descriptorCount,shaderStageFlags,nullptr };
        default:
            return VkDescriptorSetLayoutBinding({});
        }
    }
    static std::vector<VkDescriptorSetLayoutBinding> inputAttributeDescriptions(const std::vector<std::tuple<ShaderComponent,int, VkShaderStageFlags>> components) {
        std::vector<VkDescriptorSetLayoutBinding> result;
        uint32_t binding = 0;
        for (auto pair: components) {
            auto component = std::get<0>(pair);
            auto cnt = std::get<1>(pair);
            auto shaderStageFlags = std::get<2>(pair);
            result.push_back(DescriptorSetLayout::inputLayoutBinding(binding, component,cnt,shaderStageFlags));
            binding++;
        }
        return result;
    }
    static std::vector<std::tuple<ShaderComponent,int,VkShaderStageFlags>> GetComponents(DescriptorType type) {
        std::vector<std::tuple<ShaderComponent,int, VkShaderStageFlags>> results;
        switch (type) {
        case DescriptorType::Material:
            results = { {ShaderComponent::COMBINED_IMAGE,5,VK_SHADER_STAGE_FRAGMENT_BIT } };
            break;
        case DescriptorType::VP:
            results = { {ShaderComponent::UNIFORM_BUFFER,1, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT }, };
            break;
        case DescriptorType::Skybox:
            results = { { ShaderComponent::COMBINED_IMAGE,3 ,VK_SHADER_STAGE_FRAGMENT_BIT},
            { ShaderComponent::COMBINED_IMAGE,1,VK_SHADER_STAGE_FRAGMENT_BIT}, };
            break;
        case DescriptorType::Model:
            results = { {ShaderComponent::UNIFORM_BUFFER,1,VK_SHADER_STAGE_VERTEX_BIT}};
            break;
        case DescriptorType::GBuffer:
            results = { {ShaderComponent::COMBINED_IMAGE,6, VK_SHADER_STAGE_FRAGMENT_BIT} };
            break;
        case DescriptorType::ImGUI:
            results = { {ShaderComponent::COMBINED_IMAGE,1, VK_SHADER_STAGE_FRAGMENT_BIT} };
            break;
        case DescriptorType::RayTracing:
            results = { 
                //AS
                {ShaderComponent::AS,1, VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR},
                
                //Output Image
                {ShaderComponent::STORAGE_IMAGE,1,VK_SHADER_STAGE_RAYGEN_BIT_KHR},
                
                //VP(Camera Matrices, Lights)
                {ShaderComponent::UNIFORM_BUFFER,1,VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR},
                
                //Skybox
                {ShaderComponent::COMBINED_IMAGE,3,VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_MISS_BIT_KHR},
                {ShaderComponent::COMBINED_IMAGE,1,VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR}, 
                
                //Geometry Nodes
                {ShaderComponent::STORAGE_BUFFER,1,VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR},
            
                //Textures
                // 머테리얼 컴포넌트 수
                // 곱하기
                // 모델 수
                // 만큼의 텍스쳐 전달
                {ShaderComponent::COMBINED_IMAGE,3 * static_cast<int>(MaterialComponent::END),VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR},
            };
            break;
        }
        return results;
    }
private:
	VkDescriptorSetLayout _descriptorSetLayout;
};