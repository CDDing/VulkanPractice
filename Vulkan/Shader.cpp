#include "pch.h"
#include "Shader.h"

Shader::Shader()
{
}

Shader::Shader(vk::Device& device, const std::string& filename)
{
    auto code = FileLoader::readFile(filename);

    vk::ShaderModuleCreateInfo createInfo{};
    createInfo.codeSize = code.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());
    
    
    _shader = device.createShaderModule(createInfo);
}
