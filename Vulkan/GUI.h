#pragma once
#include "imgui.h"
#include "backends/imgui_impl_vulkan.h"
#include "backends/imgui_impl_glfw.h"
class Sampler;
class Image;
class ImageView;
class DescriptorPool;
class DescriptorSetLayout;
class Buffer;
class DescriptorSet;
class RenderPass;
class Device;
class GUI
{
private:
	Sampler _sampler;
	Buffer _vertexBuffer;
	Buffer _indexBuffer;
	int32_t _vertexCount = 0;
	int32_t _indexCount = 0;
	Image _fontImage;
	ImageView _fontImageView;
	VkPipeline _pipeline;
	VkPipelineLayout _pipelineLayout;
	VkPipelineCache _pipelineCache;
	DescriptorPool _descriptorPool;
	DescriptorSetLayout _descriptorSetLayout;
	DescriptorSet _descriptorSet;
	Device* _device;
	ImGuiStyle vulkanStyle;	
	struct PushConstBlock {
		glm::vec2 scale;
		glm::vec2 translate;
	} pushConstBlock;

public:
	GUI();
	GUI(Device& device) : _device(&device) 
	{
		ImGui::CreateContext();

		ImGuiIO& io = ImGui::GetIO();
		io.FontGlobalScale = 1;

		ImGuiStyle& style = ImGui::GetStyle();
		style.ScaleAllSizes(1);
	}
	void destroy();
	void newFrame();
	void updateBuffers();
	void drawFrame(VkCommandBuffer commandBuffer);
	void init(float width, float height); 
	void initDescriptorSet();
	void initResources(GLFWwindow* window, VkInstance Instance,RenderPass renderPass);
	void setStyle(uint32_t index)
	{
		switch (index)
		{
		case 0:
		{
			ImGuiStyle& style = ImGui::GetStyle();
			style = vulkanStyle;
			break;
		}
		case 1:
			ImGui::StyleColorsClassic();
			break;
		case 2:
			ImGui::StyleColorsDark();
			break;
		case 3:
			ImGui::StyleColorsLight();
			break;
		}
	}
 
};

