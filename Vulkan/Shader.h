#pragma once
class Shader
{
public:
	Shader();
	Shader(std::shared_ptr<Device> device, const std::string& filename);
	operator vk::ShaderModule () {
		return _shader;
	}
	~Shader() {
		if(_shader)
			_device->logical.destroyShaderModule(_shader);
		_shader = VK_NULL_HANDLE;
	}
private:
	vk::ShaderModule _shader = VK_NULL_HANDLE;;
	std::shared_ptr<Device> _device;
};

