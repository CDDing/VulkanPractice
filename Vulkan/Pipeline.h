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
	Pipeline(Device& device, VkExtent2D& swapChainExtent, std::vector<std::vector<VkDescriptorSetLayout>>& descriptorSetLayouts, RenderPass& renderPass, const std::string& vsShaderPath, const std::string& psShaderPath, ShaderType type);
	operator VkPipeline& () {
		return _pipeline;
	}
	VkPipelineLayout& GetLayout() { return _pipelineLayout; }

	void destroy(Device& device) {
		vkDestroyPipeline(device, _pipeline, nullptr);
		vkDestroyPipelineLayout(device, _pipelineLayout, nullptr);
	}
private:
	VkPipelineLayout _pipelineLayout;
	VkPipeline _pipeline;
};