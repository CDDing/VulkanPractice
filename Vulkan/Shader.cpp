#include "pch.h"
#include "Shader.h"
vk::raii::ShaderModule createShader(Device& device, const std::string& filename) {
	auto code = FileLoader::readFile(filename);
	vk::ShaderModuleCreateInfo createInfo{ {},code.size(),reinterpret_cast<const uint32_t*>(code.data()) };
	return vk::raii::ShaderModule(device.logical, createInfo);
}