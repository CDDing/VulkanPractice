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
	Pipeline(DContext& context, vk::Extent2D& swapChainExtent, std::vector<std::vector<vk::DescriptorSetLayout>>& descriptorSetLayouts, vk::raii::RenderPass& renderPass, const std::string& vsShaderPath, const std::string& psShaderPath, ShaderType type);
	operator vk::raii::Pipeline& () {
		return pipeline;
	}
	operator vk::Pipeline() {
		return pipeline;
	}
	vk::raii::PipelineLayout& GetLayout() { return pipelineLayout; }

private:
	vk::raii::PipelineLayout pipelineLayout;
	vk::raii::Pipeline pipeline;
};