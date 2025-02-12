#pragma once

struct QueueFamilyIndices {
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily;
    bool isComplete() {
        return graphicsFamily.has_value() && presentFamily.has_value();
    }
};

struct SwapChainSupportDetails {
    vk::SurfaceCapabilitiesKHR capabilities;
    std::vector<vk::SurfaceFormatKHR> formats;
    std::vector<vk::PresentModeKHR> presentModes;
};

enum class VertexComponent { Position, Normal, UV, Color, Tangent };
struct Vertex {
    glm::vec3 pos;
    float padding;
    
    glm::vec3 normal;
    float padding2;
    
    glm::vec2 texCoord;
    glm::vec2 padding3;
    
    glm::vec3 tangent;
    float padding4;
    glm::vec4 color;
    void operator=(std::tuple<glm::vec3, glm::vec3, glm::vec2> mem) {
        pos = std::get<0>(mem);
        normal = std::get<1>(mem);
        texCoord = std::get<2>(mem);

    }
    static vk::VertexInputBindingDescription getBindingDescription() {
        
        return { 0,sizeof(Vertex),vk::VertexInputRate::eVertex };
    }
    bool operator==(const Vertex& other) const {
        return pos == other.pos && normal == other.normal && texCoord == other.texCoord && tangent == other.tangent;
    }
    static vk::VertexInputAttributeDescription inputAttributeDescription(uint32_t binding, uint32_t location, VertexComponent component) {
        switch (component) {
        case VertexComponent::Position:
            return vk::VertexInputAttributeDescription({ location, binding, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, pos) });
        case VertexComponent::Normal:
            return vk::VertexInputAttributeDescription({ location, binding, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, normal) });
        case VertexComponent::UV:
            return vk::VertexInputAttributeDescription({ location, binding, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, texCoord) });
        case VertexComponent::Color:
            return vk::VertexInputAttributeDescription({ location, binding, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(Vertex, color) });
        case VertexComponent::Tangent:
            return vk::VertexInputAttributeDescription({ location, binding, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(Vertex, tangent) });
        default:
            return vk::VertexInputAttributeDescription({});
        }
    }
    static std::vector<vk::VertexInputAttributeDescription> inputAttributeDescriptions(uint32_t binding, const std::vector<VertexComponent> components) {
        std::vector<vk::VertexInputAttributeDescription> result;
        uint32_t location = 0;
        for (VertexComponent component : components) {
            result.push_back(Vertex::inputAttributeDescription(binding, location, component));
            location++;
        }
        return result;
    }

};
struct UniformBufferObject {
    alignas(16) glm::mat4 view;
    alignas(16) glm::mat4 proj;
    glm::vec4 lights[4];
    alignas(16) glm::vec3 camPos;
};
struct Transform {
    alignas(16) glm::mat4 model;
};

struct GUIControl {
    alignas(4) bool useNormalMap;
    alignas(4) float roughness;
    alignas(4) float metallic;
    alignas(4) bool RayTracing;
};


enum class MaterialComponent {
    ALBEDO,
    NORMAL,
    ROUGHNESS,
    METALNESS,
    ao,
    END,
};