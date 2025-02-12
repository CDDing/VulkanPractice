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
    operator vk::DescriptorSetLayout& () {
        return _descriptorSetLayout;
    }
    vk::DescriptorSetLayout* operator&() {
        return &_descriptorSetLayout;
    }
    void destroy(Device& device) {
        device.logical.destroyDescriptorSetLayout(_descriptorSetLayout);
    }
    static vk::DescriptorSetLayoutBinding inputLayoutBinding(uint32_t binding, ShaderComponent component,int cnt,vk::ShaderStageFlags shaderStageFlags) {
        uint32_t descriptorCount = static_cast<uint32_t>(cnt);
        switch (component) {
        case ShaderComponent::UNIFORM_BUFFER:
            return { binding, vk::DescriptorType::eUniformBuffer,descriptorCount, shaderStageFlags, nullptr };
        case ShaderComponent::STORAGE_BUFFER:
            return { binding,vk::DescriptorType::eStorageBuffer,descriptorCount,shaderStageFlags,nullptr };
        case ShaderComponent::COMBINED_IMAGE:
            return { binding, vk::DescriptorType::eCombinedImageSampler,descriptorCount,shaderStageFlags,nullptr };
        case ShaderComponent::AS:
            return { binding, vk::DescriptorType::eAccelerationStructureKHR,descriptorCount,shaderStageFlags,nullptr };
        case ShaderComponent::STORAGE_IMAGE:
            return { binding, vk::DescriptorType::eStorageImage,descriptorCount,shaderStageFlags,nullptr };
        default:
            return VkDescriptorSetLayoutBinding({});
        }
    }
    static std::vector<vk::DescriptorSetLayoutBinding> inputAttributeDescriptions(const std::vector<std::tuple<ShaderComponent,int, vk::ShaderStageFlags>> components) {
        std::vector<vk::DescriptorSetLayoutBinding> result;
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
    static std::vector<std::tuple<ShaderComponent,int,vk::ShaderStageFlags>> GetComponents(DescriptorType type) {
        std::vector<std::tuple<ShaderComponent,int, vk::ShaderStageFlags>> results;
        switch (type) {
        case DescriptorType::Material:
            results = { {ShaderComponent::COMBINED_IMAGE,5,vk::ShaderStageFlagBits::eFragment} };
            break;
        case DescriptorType::VP:
            results = { {ShaderComponent::UNIFORM_BUFFER,1, vk::ShaderStageFlagBits::eVertex| vk::ShaderStageFlagBits::eFragment }, };
            break;
        case DescriptorType::Skybox:
            results = { { ShaderComponent::COMBINED_IMAGE,3 ,vk::ShaderStageFlagBits::eFragment},
            { ShaderComponent::COMBINED_IMAGE,1,vk::ShaderStageFlagBits::eFragment}, };
            break;
        case DescriptorType::Model:
            results = { {ShaderComponent::UNIFORM_BUFFER,1,vk::ShaderStageFlagBits::eVertex}};
            break;
        case DescriptorType::GBuffer:
            results = { {ShaderComponent::COMBINED_IMAGE,6, vk::ShaderStageFlagBits::eFragment} };
            break;
        case DescriptorType::ImGUI:
            results = { {ShaderComponent::COMBINED_IMAGE,1, vk::ShaderStageFlagBits::eFragment} };
            break;
        case DescriptorType::RayTracing:
            results = { 
                //AS
                {ShaderComponent::AS,1, vk::ShaderStageFlagBits::eRaygenKHR | vk::ShaderStageFlagBits::eClosestHitKHR},
                
                //Output Image
                {ShaderComponent::STORAGE_IMAGE,1,vk::ShaderStageFlagBits::eRaygenKHR},
                
                //VP(Camera Matrices, Lights)
                {ShaderComponent::UNIFORM_BUFFER,1,vk::ShaderStageFlagBits::eRaygenKHR | vk::ShaderStageFlagBits::eClosestHitKHR},
                
                //Skybox
                {ShaderComponent::COMBINED_IMAGE,3,vk::ShaderStageFlagBits::eClosestHitKHR | vk::ShaderStageFlagBits::eMissKHR},
                {ShaderComponent::COMBINED_IMAGE,1,vk::ShaderStageFlagBits::eClosestHitKHR}, 
                
                //Geometry Nodes
                {ShaderComponent::STORAGE_BUFFER,1,vk::ShaderStageFlagBits::eClosestHitKHR},
            
                //Textures
                // 머테리얼 컴포넌트 수
                // 곱하기
                // 모델 수
                // 만큼의 텍스쳐 전달
                {ShaderComponent::COMBINED_IMAGE,3 * static_cast<int>(MaterialComponent::END),vk::ShaderStageFlagBits::eClosestHitKHR},

                //GUI
                {ShaderComponent::UNIFORM_BUFFER,1,vk::ShaderStageFlagBits::eRaygenKHR | vk::ShaderStageFlagBits::eClosestHitKHR},

            };
            break;
        }
        return results;
    }
private:
	vk::DescriptorSetLayout _descriptorSetLayout;
};