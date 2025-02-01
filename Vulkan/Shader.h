#pragma once
class Shader
{
public:
	Shader();
	Shader(Device& device, const std::string& filename);
	operator VkShaderModule& () {
		return _shader;
	}
	void destroy(Device& device) {
		vkDestroyShaderModule(device, _shader, nullptr);
	}
private:
	VkShaderModule _shader;
};

