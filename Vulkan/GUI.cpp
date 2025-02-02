#include "pch.h"
#include "GUI.h"
void GUI::initResources(GLFWwindow* window, VkInstance Instance, RenderPass renderPass)
{
	ImGuiIO& io = ImGui::GetIO();

	unsigned char* fontData;
	int texWidth, texHeight;
	io.Fonts->GetTexDataAsRGBA32(&fontData, &texWidth, &texHeight);
	VkDeviceSize uploadSize = texWidth * texHeight * 4 * sizeof(char);
	/*if (_device->Get()->extensionSupported(VK_KHR_DRIVER_PROPERTIES_EXTENSION_NAME))
	{
		VkPhysicalDeviceProperties2 deviceProperties2 = {};
		deviceProperties2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
		deviceProperties2.pNext = &driverProperties;
		driverProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DRIVER_PROPERTIES;
		vkGetPhysicalDeviceProperties2(_device->GetPhysical(), &deviceProperties2);
	}*/
	_fontImage = ImageSet(*_device, texWidth, texHeight, 1,
		VK_FORMAT_R8G8B8A8_UNORM,
		VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		VK_IMAGE_ASPECT_COLOR_BIT);

	
	_fontImage.image.fillImage(*_device, fontData, uploadSize);
	_fontImage.image.transitionLayout(*_device,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT);
	_fontImage.image.transitionLayout(*_device, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT);

	VkDescriptorPoolSize pool_sizes[] =
	{
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 10*IMGUI_IMPL_VULKAN_MINIMUM_IMAGE_SAMPLER_POOL_SIZE },
	};
	VkDescriptorPoolCreateInfo pool_info = {};
	pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
	pool_info.maxSets = 0;
	for (VkDescriptorPoolSize& pool_size : pool_sizes)
		pool_info.maxSets += pool_size.descriptorCount;
	pool_info.poolSizeCount = (uint32_t)IM_ARRAYSIZE(pool_sizes);
	pool_info.pPoolSizes = pool_sizes;
	auto err = vkCreateDescriptorPool(*_device, &pool_info, nullptr, &_descriptorPool);
	check_vk_result(err);
	_descriptorSetLayout = DescriptorSetLayout(*_device, DescriptorType::ImGUI);
	_descriptorSet = DescriptorSet(*_device, _descriptorPool, _descriptorSetLayout);
	initDescriptorSet();

	//파이프라인
	VkPipelineCacheCreateInfo pipelineCacheCreateInfo = {};
	pipelineCacheCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
	vkCreatePipelineCache(*_device, &pipelineCacheCreateInfo, nullptr, &_pipelineCache);

	// Pipeline layout
	// Push constants for UI rendering parameters
	VkPushConstantRange pushConstantRange{};
	pushConstantRange.size = sizeof(PushConstBlock);
	pushConstantRange.offset = 0;
	pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{};
	pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutCreateInfo.setLayoutCount = 1;
	pipelineLayoutCreateInfo.pSetLayouts = &_descriptorSetLayout;
	pipelineLayoutCreateInfo.pushConstantRangeCount = 1;
	pipelineLayoutCreateInfo.pPushConstantRanges = &pushConstantRange;
	vkCreatePipelineLayout(*_device, &pipelineLayoutCreateInfo, nullptr, &_pipelineLayout);

	// Setup graphics pipeline for UI rendering
	VkPipelineInputAssemblyStateCreateInfo inputAssemblyState{};
	inputAssemblyState.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssemblyState.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssemblyState.primitiveRestartEnable = VK_FALSE;
	inputAssemblyState.flags = 0;

	VkPipelineRasterizationStateCreateInfo rasterizationState{};
	rasterizationState.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizationState.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizationState.cullMode = VK_CULL_MODE_NONE;
	rasterizationState.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterizationState.flags = 0;
	rasterizationState.depthClampEnable = VK_FALSE;
	rasterizationState.lineWidth = 1.0f;


	// Enable blending
	VkPipelineColorBlendAttachmentState blendAttachmentState{};
	blendAttachmentState.blendEnable = VK_TRUE;
	blendAttachmentState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	blendAttachmentState.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	blendAttachmentState.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	blendAttachmentState.colorBlendOp = VK_BLEND_OP_ADD;
	blendAttachmentState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	blendAttachmentState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	blendAttachmentState.alphaBlendOp = VK_BLEND_OP_ADD;

	VkPipelineColorBlendStateCreateInfo colorBlendState{};
	colorBlendState.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlendState.attachmentCount = 1;
	colorBlendState.pAttachments = &blendAttachmentState;

	VkPipelineDepthStencilStateCreateInfo depthStencilState{};
	depthStencilState.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthStencilState.depthTestEnable = VK_FALSE;
	depthStencilState.depthWriteEnable = VK_FALSE;
	depthStencilState.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
	depthStencilState.back.compareOp = VK_COMPARE_OP_ALWAYS;

	VkPipelineViewportStateCreateInfo viewportState{};
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.viewportCount = 1;
	viewportState.scissorCount = 1;
	viewportState.flags = 0;

	VkPipelineMultisampleStateCreateInfo multisampleState{};
	multisampleState.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampleState.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	multisampleState.flags = 0;

	std::vector<VkDynamicState> dynamicStateEnables = {
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR
	};
	VkPipelineDynamicStateCreateInfo dynamicState{};
	dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicState.pDynamicStates = dynamicStateEnables.data();
	dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStateEnables.size());
	dynamicState.flags = 0;

	std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages{};

	VkGraphicsPipelineCreateInfo pipelineCreateInfo{};
	pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineCreateInfo.layout = _pipelineLayout;
	pipelineCreateInfo.renderPass = renderPass;
	pipelineCreateInfo.flags = 0;
	pipelineCreateInfo.basePipelineIndex = -1;
	pipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;

	pipelineCreateInfo.pInputAssemblyState = &inputAssemblyState;
	pipelineCreateInfo.pRasterizationState = &rasterizationState;
	pipelineCreateInfo.pColorBlendState = &colorBlendState;
	pipelineCreateInfo.pMultisampleState = &multisampleState;
	pipelineCreateInfo.pViewportState = &viewportState;
	pipelineCreateInfo.pDepthStencilState = &depthStencilState;
	pipelineCreateInfo.pDynamicState = &dynamicState;
	pipelineCreateInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
	pipelineCreateInfo.pStages = shaderStages.data();

	// Vertex bindings an attributes based on ImGui vertex definition
	std::vector<VkVertexInputBindingDescription> vertexInputBindings = {
		VkVertexInputBindingDescription{0,sizeof(ImDrawVert),VK_VERTEX_INPUT_RATE_VERTEX},
	};

	VkVertexInputAttributeDescription vInputAttribDescription1{};
	vInputAttribDescription1.location = 0;
	vInputAttribDescription1.binding = 0;
	vInputAttribDescription1.format = VK_FORMAT_R32G32_SFLOAT;
	vInputAttribDescription1.offset = offsetof(ImDrawVert, pos);
	VkVertexInputAttributeDescription vInputAttribDescription2{};
	vInputAttribDescription2.location = 1;
	vInputAttribDescription2.binding = 0;
	vInputAttribDescription2.format = VK_FORMAT_R32G32_SFLOAT;
	vInputAttribDescription2.offset = offsetof(ImDrawVert, uv);
	VkVertexInputAttributeDescription vInputAttribDescription3{};
	vInputAttribDescription3.location = 2;
	vInputAttribDescription3.binding = 0;
	vInputAttribDescription3.format = VK_FORMAT_R8G8B8A8_UNORM;
	vInputAttribDescription3.offset = offsetof(ImDrawVert, col);
	std::vector<VkVertexInputAttributeDescription> vertexInputAttributes = {
		vInputAttribDescription1,vInputAttribDescription2,vInputAttribDescription3
	};
	VkPipelineVertexInputStateCreateInfo vertexInputState{};
	vertexInputState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputState.vertexBindingDescriptionCount = static_cast<uint32_t>(vertexInputBindings.size());
	vertexInputState.pVertexBindingDescriptions = vertexInputBindings.data();
	vertexInputState.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertexInputAttributes.size());
	vertexInputState.pVertexAttributeDescriptions = vertexInputAttributes.data();

	pipelineCreateInfo.pVertexInputState = &vertexInputState;

	auto vertexShaderModule = Shader(*_device, "shaders/imgui.vert.spv");
	auto fragmentShaderModule = Shader(*_device, "shaders/imgui.frag.spv");
	shaderStages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
	shaderStages[0].pName = "main";
	shaderStages[0].module = vertexShaderModule;

	shaderStages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shaderStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	shaderStages[1].pName = "main";
	shaderStages[1].module = fragmentShaderModule;

	vkCreateGraphicsPipelines(*_device, _pipelineCache, 1, &pipelineCreateInfo, nullptr, &_pipeline);

	vertexShaderModule.destroy(*_device);
	fragmentShaderModule.destroy(*_device);

	ImGui_ImplGlfw_InitForVulkan(window, true);
	ImGui_ImplVulkan_InitInfo init_info = {};
	init_info.Instance = Instance;
	init_info.PhysicalDevice = *_device;
	init_info.Device = *_device;
	init_info.QueueFamily = ImGui_ImplVulkanH_SelectQueueFamilyIndex(*_device);
	VkQueue queue;
	vkGetDeviceQueue(*_device, init_info.QueueFamily, 0, &queue);
	init_info.Queue = queue;
	init_info.PipelineCache = _pipelineCache;
	init_info.DescriptorPool = _descriptorPool;
	init_info.RenderPass = renderPass;
	init_info.Subpass = 0;
	init_info.MinImageCount = 2;
	init_info.ImageCount = 2;
	init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
	init_info.Allocator = nullptr;
	init_info.CheckVkResultFn = check_vk_result;
	ImGui_ImplVulkan_Init(&init_info);
}

GUI::GUI()
{

}

void GUI::destroy()
{

	ImGui_ImplVulkan_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	_vertexBuffer.destroy(*_device);
	_indexBuffer.destroy(*_device);
	_fontImage.destroy(*_device);
	vkDestroyPipelineCache(*_device, _pipelineCache, nullptr);
	vkDestroyPipeline(*_device, _pipeline, nullptr);
	vkDestroyPipelineLayout(*_device, _pipelineLayout, nullptr);
	_descriptorPool.destroy(*_device);
	_descriptorSetLayout.destroy(*_device);
}
void GUI::newFrame()
{
	ImGui_ImplVulkan_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();
	// Debug window
	//ImGui::ShowDemoWindow();

	// Render to generate draw buffers
}
void GUI::AddFloatGUI(std::string text, float& value,float min, float max) {
	ImGui::SliderFloat(text.c_str(), &value,min,max);
}
void GUI::AddBoolGUI(std::string text, bool& value) {
	ImGui::Checkbox(text.c_str(), &value);
}
void GUI::End(){

	ImGui::Render();
}
void GUI::updateBuffers()
{
	ImDrawData* imDrawData = ImGui::GetDrawData();

	VkDeviceSize vertexBufferSize = imDrawData->TotalVtxCount * sizeof(ImDrawVert);
	VkDeviceSize indexBufferSize = imDrawData->TotalIdxCount * sizeof(ImDrawIdx);

	if ((vertexBufferSize == 0) || (indexBufferSize == 0)) {
		return;
	}
	
	// Vertex buffer
	if ((_vertexBuffer == VK_NULL_HANDLE) || (_vertexCount != imDrawData->TotalVtxCount)) {
		_vertexBuffer.unmap(*_device);
		_vertexBuffer.destroy(*_device);
		_vertexBuffer = Buffer(*_device, vertexBufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
		_vertexCount = imDrawData->TotalVtxCount;
		_vertexBuffer.map(*_device);
	}

	// Index buffer
	if ((_indexBuffer == VK_NULL_HANDLE) || (_indexCount < imDrawData->TotalIdxCount)) {
		_indexBuffer.unmap(*_device);
		_indexBuffer.destroy(*_device);
		_indexBuffer = Buffer(*_device, indexBufferSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
		_indexCount = imDrawData->TotalIdxCount;
		_indexBuffer.map(*_device);
	}

	// Upload data
	ImDrawVert* vtxDst = (ImDrawVert*)_vertexBuffer.mapped;
	ImDrawIdx* idxDst = (ImDrawIdx*)_indexBuffer.mapped;

	for (int n = 0; n < imDrawData->CmdListsCount; n++) {
		const ImDrawList* cmd_list = imDrawData->CmdLists[n];
		memcpy(vtxDst, cmd_list->VtxBuffer.Data, cmd_list->VtxBuffer.Size * sizeof(ImDrawVert));
		memcpy(idxDst, cmd_list->IdxBuffer.Data, cmd_list->IdxBuffer.Size * sizeof(ImDrawIdx));
		vtxDst += cmd_list->VtxBuffer.Size;
		idxDst += cmd_list->IdxBuffer.Size;
	}

	// Flush to make writes visible to GPU
	_vertexBuffer.flush(*_device);
	_indexBuffer.flush(*_device);
}

void GUI::drawFrame(VkCommandBuffer commandBuffer)
{
	ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer, _pipeline);

	//ImGuiIO& io = ImGui::GetIO();
	//vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _pipelineLayout, 0, 1, &_descriptorSet.Get(), 0, nullptr);
	//vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _pipeline);
	//
	//VkViewport viewport{};
	//viewport.x = 0.0f;
	//viewport.y = 0.0f;
	//viewport.width = ImGui::GetIO().DisplaySize.x;
	//viewport.height = ImGui::GetIO().DisplaySize.y;
	//viewport.minDepth = 0.0f;
	//viewport.maxDepth = 1.0f;
	//vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

	//// UI scale and translate via push constants
	//pushConstBlock.scale = glm::vec2(2.0f / io.DisplaySize.x, 2.0f / io.DisplaySize.y);
	//pushConstBlock.translate = glm::vec2(-1.0f);
	//vkCmdPushConstants(commandBuffer, _pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(PushConstBlock), &pushConstBlock);

	//// Render commands
	//ImDrawData* imDrawData = ImGui::GetDrawData();
	//int32_t vertexOffset = 0;
	//int32_t indexOffset = 0;
	//ImVec2 clip_off = imDrawData->DisplayPos;         // (0,0) unless using multi-viewports
	//ImVec2 clip_scale = imDrawData->FramebufferScale;
	//int fb_width = (int)(imDrawData->DisplaySize.x * imDrawData->FramebufferScale.x);
	//int fb_height = (int)(imDrawData->DisplaySize.y * imDrawData->FramebufferScale.y);
	//if (fb_width <= 0 || fb_height <= 0)
	//	return;
	//if (imDrawData->CmdListsCount > 0) {

	//	VkDeviceSize offsets[1] = { 0 };
	//	vkCmdBindVertexBuffers(commandBuffer, 0, 1, &_vertexBuffer.Get(), offsets);
	//	vkCmdBindIndexBuffer(commandBuffer, _indexBuffer.Get(), 0, VK_INDEX_TYPE_UINT16);

	//	for (int32_t i = 0; i < imDrawData->CmdListsCount; i++)
	//	{
	//		const ImDrawList* draw_list = imDrawData->CmdLists[i];
	//		for (int cmd_i = 0; cmd_i < draw_list->CmdBuffer.Size; cmd_i++)
	//		{
	//			const ImDrawCmd* pcmd = &draw_list->CmdBuffer[cmd_i];
	//			if (pcmd->UserCallback != nullptr)
	//			{
	//				// User callback, registered via ImDrawList::AddCallback()
	//				// (ImDrawCallback_ResetRenderState is a special callback value used by the user to request the renderer to reset render state.)
	//				//if (pcmd->UserCallback == ImDrawCallback_ResetRenderState)
	//				//	ImGui_ImplVulkan_SetupRenderState(draw_data, pipeline, command_buffer, rb, fb_width, fb_height);
	//				//else
	//				pcmd->UserCallback(draw_list, pcmd);
	//			}
	//			else
	//			{
	//				// Project scissor/clipping rectangles into framebuffer space
	//				ImVec2 clip_min((pcmd->ClipRect.x - clip_off.x) * clip_scale.x, (pcmd->ClipRect.y - clip_off.y) * clip_scale.y);
	//				ImVec2 clip_max((pcmd->ClipRect.z - clip_off.x) * clip_scale.x, (pcmd->ClipRect.w - clip_off.y) * clip_scale.y);

	//				// Clamp to viewport as vkCmdSetScissor() won't accept values that are off bounds
	//				if (clip_min.x < 0.0f) { clip_min.x = 0.0f; }
	//				if (clip_min.y < 0.0f) { clip_min.y = 0.0f; }
	//				if (clip_max.x > fb_width) { clip_max.x = (float)fb_width; }
	//				if (clip_max.y > fb_height) { clip_max.y = (float)fb_height; }
	//				if (clip_max.x <= clip_min.x || clip_max.y <= clip_min.y)
	//					continue;

	//				// Apply scissor/clipping rectangle
	//				VkRect2D scissor;
	//				scissor.offset.x = (int32_t)(clip_min.x);
	//				scissor.offset.y = (int32_t)(clip_min.y);
	//				scissor.extent.width = (uint32_t)(clip_max.x - clip_min.x);
	//				scissor.extent.height = (uint32_t)(clip_max.y - clip_min.y);
	//				vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
	//				vkCmdDrawIndexed(commandBuffer, pcmd->ElemCount, 1, indexOffset, vertexOffset, 0);
	//				indexOffset += pcmd->ElemCount;
	//			}
	//			vertexOffset += draw_list->VtxBuffer.Size;
	//		}

	//	}
	//}
}
void GUI::init(float width, float height)
{
	vulkanStyle = ImGui::GetStyle();
	vulkanStyle.Colors[ImGuiCol_TitleBg] = ImVec4(1.0f, 0.0f, 0.0f, 0.6f);
	vulkanStyle.Colors[ImGuiCol_TitleBgActive] = ImVec4(1.0f, 0.0f, 0.0f, 0.8f);
	vulkanStyle.Colors[ImGuiCol_MenuBarBg] = ImVec4(1.0f, 0.0f, 0.0f, 0.4f);
	vulkanStyle.Colors[ImGuiCol_Header] = ImVec4(1.0f, 0.0f, 0.0f, 0.4f);
	vulkanStyle.Colors[ImGuiCol_CheckMark] = ImVec4(0.0f, 1.0f, 0.0f, 1.0f);

	setStyle(0);

	// Dimensions
	ImGuiIO& io = ImGui::GetIO();
	io.DisplaySize = ImVec2(width, height);
	io.DisplayFramebufferScale = ImVec2(1.0f, 1.0f);
	//#if defined(_WIN32)
	//		// If we directly work with os specific key codes, we need to map special key types like tab
	//		io.KeyMap[ImGuiKey_Tab] = VK_TAB;
	//		io.KeyMap[ImGuiKey_LeftArrow] = VK_LEFT;
	//		io.KeyMap[ImGuiKey_RightArrow] = VK_RIGHT;
	//		io.KeyMap[ImGuiKey_UpArrow] = VK_UP;
	//		io.KeyMap[ImGuiKey_DownArrow] = VK_DOWN;
	//		io.KeyMap[ImGuiKey_Backspace] = VK_BACK;
	//		io.KeyMap[ImGuiKey_Enter] = VK_RETURN;
	//		io.KeyMap[ImGuiKey_Space] = VK_SPACE;
	//		io.KeyMap[ImGuiKey_Delete] = VK_DELETE;
	//#endif
}

void GUI::initDescriptorSet()
{
	VkDescriptorImageInfo imageInfo{};
	imageInfo.sampler = Sampler::Get(SamplerMipMapType::Low);
	imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	imageInfo.imageView = _fontImage;

	VkWriteDescriptorSet descriptorWrite{};
	descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrite.dstSet = _descriptorSet;
	descriptorWrite.dstBinding = 0;
	descriptorWrite.dstArrayElement = 0;
	descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptorWrite.descriptorCount = 1;
	descriptorWrite.pImageInfo = &imageInfo;

	vkUpdateDescriptorSets(*_device, 1, &descriptorWrite, 0, nullptr);


}
