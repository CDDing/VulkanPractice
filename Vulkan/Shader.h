#pragma once
class Shader
{
public:
	Shader();
	Shader(Device* device, const std::string& filename);
	VkShaderModule& Get() { return _shader; }
private:
	VkShaderModule _shader;
};

