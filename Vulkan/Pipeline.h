#pragma once
class Pipeline
{
public:

	enum {
		DEFAULT,
		SKYBOX,
		END,
	};
	Pipeline();
	Pipeline(Device& device, VkExtent2D& swapChainExtent, VkDescriptorSetLayout& descriptorSetLayout, RenderPass& renderPass, const std::string& vsShaderPath, const std::string& psShaderPath);
	VkPipeline& Get() { return _pipeline; }
	VkPipelineLayout& GetLayout() { return _pipelineLayout; }
private:
	VkPipelineLayout _pipelineLayout;
	VkPipeline _pipeline;
};