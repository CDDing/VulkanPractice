#pragma once
#include "imgui.h"
#include "backends/imgui_impl_vulkan.h"
#include "backends/imgui_impl_glfw.h"
class Device;
class VulkanApp;
class GUI
{
private:
	vk::raii::Pipeline pipeline;
	vk::raii::PipelineLayout pipelineLayout;
	vk::raii::PipelineCache pipelineCache;
	vk::raii::DescriptorPool descriptorPool;
	vk::raii::DescriptorSetLayout descriptorSetLayout;
	vk::raii::DescriptorSet descriptorSet;
	DBuffer vertexBuffer;
	DBuffer indexBuffer;
	DImage fontImage;
	int32_t vertexCount = 0;
	int32_t indexCount = 0;
	ImGuiStyle vulkanStyle;	
	struct PushConstBlock {
		glm::vec2 scale;
		glm::vec2 translate;
	} pushConstBlock;

public:
	GUI(DContext& context, vk::raii::RenderPass& renderPass);
	~GUI();
	void newFrame();
	void AddFloatGUI(std::string text, float& value,float min,float max);
	void AddBoolGUI(std::string text, bool& value);
	void AddText(const std::string text);
	void AddText(const char* format, ...);
	void End();
	void updateBuffers(DContext& context);
	void drawFrame(vk::raii::CommandBuffer& commandBuffer);
	void initDescriptorSet(DContext& context);
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

