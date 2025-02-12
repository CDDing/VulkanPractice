#pragma once
class Shader
{
public:
	Shader();
	Shader(vk::Device& device, const std::string& filename);
	operator vk::ShaderModule () {
		return _shader;
	}
	void destroy(vk::Device& device) {
		device.destroyShaderModule(_shader);
	}
private:
	vk::ShaderModule _shader;
};

