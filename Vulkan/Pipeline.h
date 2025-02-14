#pragma once
enum class ShaderType;
class DescriptorSetLayout;
class Pipeline
{
public:

	enum {
		DEFAULT,
		SKYBOX,
		DEFERRED,
		END,
	};
	Pipeline();
	Pipeline(std::shared_ptr<Device> device, vk::Extent2D& swapChainExtent, std::vector<std::vector<vk::DescriptorSetLayout>>& descriptorSetLayouts, RenderPass& renderPass, const std::string& vsShaderPath, const std::string& psShaderPath, ShaderType type);
	operator vk::Pipeline& () {
		return _pipeline;
	}
	vk::PipelineLayout& GetLayout() { return _pipelineLayout; }

	void destroy(vk::Device& device) {
		device.destroyPipeline(_pipeline);
		device.destroyPipelineLayout(_pipelineLayout);
	}
private:
	vk::PipelineLayout _pipelineLayout;
	vk::Pipeline _pipeline;
};