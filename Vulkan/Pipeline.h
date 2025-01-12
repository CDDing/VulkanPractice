#pragma once
enum class ShaderType;
class DescriptorSetLayout;
class Pipeline
{
public:

	enum {
		DEFAULT,
		SKYBOX,
		END,
	};
	Pipeline();
	Pipeline(Device& device, VkExtent2D& swapChainExtent, std::vector<std::vector<VkDescriptorSetLayout>>& descriptorSetLayouts, RenderPass& renderPass, const std::string& vsShaderPath, const std::string& psShaderPath, ShaderType type);
	VkPipeline& Get() { return _pipeline; }
	VkPipelineLayout& GetLayout() { return _pipelineLayout; }
private:
	VkPipelineLayout _pipelineLayout;
	VkPipeline _pipeline;
};