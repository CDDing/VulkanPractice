#include "pch.h"
#include "Shader.h"

Shader::Shader()
{
}

Shader::Shader(Device& device, const std::string& filename)
{
    auto code = FileLoader::readFile(filename);

    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());
    
    
    if (vkCreateShaderModule(device.Get(), &createInfo, nullptr, &_shader) != VK_SUCCESS) {
        throw std::runtime_error("failed to create shader module!");
    }
}
