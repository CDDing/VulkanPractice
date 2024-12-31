#pragma once
class Pipeline
{
public:
	Pipeline();
	Pipeline(Device& device, VkExtent2D& swapChainExtent, VkDescriptorSetLayout& descriptorSetLayout, RenderPass& renderPass);
	VkPipeline& Get() { return _pipeline; }
	VkPipelineLayout& GetLayout() { return _pipelineLayout; }
private:
	VkPipelineLayout _pipelineLayout;
	VkPipeline _pipeline;
};

