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
		vk::Format::eR8G8B8A8Unorm,
		vk::ImageTiling::eOptimal,
		vk::ImageUsageFlagBits::eSampled| vk::ImageUsageFlagBits::eTransferDst,
		vk::MemoryPropertyFlagBits::eDeviceLocal,
		vk::ImageAspectFlagBits::eColor);

	
	_fontImage.image.fillImage(*_device, fontData, uploadSize);

	vk::CommandBuffer cmdBuf = beginSingleTimeCommands(*_device);
	_fontImage.image.transitionLayout(*_device, cmdBuf, vk::ImageLayout::eShaderReadOnlyOptimal, vk::ImageAspectFlagBits::eColor);
	endSingleTimeCommands(*_device, cmdBuf);

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

	_descriptorPool = std::make_shared<DescriptorPool>();
	_descriptorPool->_device = _device;
	_descriptorPool->_descriptorPool = _device->logical.createDescriptorPool(pool_info);
	_descriptorSetLayout = DescriptorSetLayout(*_device, DescriptorType::ImGUI);
	_descriptorSet = DescriptorSet(*_device, *_descriptorPool, _descriptorSetLayout);
	initDescriptorSet();

	//파이프라인
	vk::PipelineCacheCreateInfo pipelineCacheCreateInfo = {};
	_pipelineCache = _device->logical.createPipelineCache(pipelineCacheCreateInfo);

	// Pipeline layout
	// Push constants for UI rendering parameters
	vk::PushConstantRange pushConstantRange{};
	pushConstantRange.size = sizeof(PushConstBlock);
	pushConstantRange.offset = 0;
	pushConstantRange.stageFlags = vk::ShaderStageFlagBits::eVertex;
	vk::PipelineLayoutCreateInfo pipelineLayoutCreateInfo{ {},{_descriptorSetLayout},{pushConstantRange } };

	_pipelineLayout = _device->logical.createPipelineLayout(pipelineLayoutCreateInfo);

	// Setup graphics pipeline for UI rendering
	vk::PipelineInputAssemblyStateCreateInfo inputAssemblyState{};
	inputAssemblyState.topology = vk::PrimitiveTopology::eTriangleList;
	inputAssemblyState.primitiveRestartEnable = vk::False;
	inputAssemblyState.flags = (vk::PipelineInputAssemblyStateCreateFlagBits)0;

	vk::PipelineRasterizationStateCreateInfo rasterizationState{};
	rasterizationState.polygonMode = vk::PolygonMode::eFill;
	rasterizationState.cullMode = vk::CullModeFlagBits::eNone;
	rasterizationState.frontFace = vk::FrontFace::eCounterClockwise;
	rasterizationState.flags = (vk::PipelineRasterizationStateCreateFlagBits)0;
	rasterizationState.depthClampEnable = vk::False;
	rasterizationState.lineWidth = 1.0f;


	// Enable blending
	vk::PipelineColorBlendAttachmentState blendAttachmentState{};
	blendAttachmentState.blendEnable = vk::True;
	blendAttachmentState.colorWriteMask = vk::ColorComponentFlagBits::eR| vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
	blendAttachmentState.srcColorBlendFactor = vk::BlendFactor::eSrcAlpha;
	blendAttachmentState.dstColorBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha;
	blendAttachmentState.colorBlendOp = vk::BlendOp::eAdd;
	blendAttachmentState.srcAlphaBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha;
	blendAttachmentState.dstAlphaBlendFactor = vk::BlendFactor::eZero;
	blendAttachmentState.alphaBlendOp = vk::BlendOp::eAdd;

	vk::PipelineColorBlendStateCreateInfo colorBlendState{};
	colorBlendState.attachmentCount = 1;
	colorBlendState.pAttachments = &blendAttachmentState;

	vk::PipelineDepthStencilStateCreateInfo depthStencilState{};
	depthStencilState.depthTestEnable = vk::False;
	depthStencilState.depthWriteEnable = vk::False;
	depthStencilState.depthCompareOp = vk::CompareOp::eLessOrEqual;
	depthStencilState.back.compareOp = vk::CompareOp::eAlways;

	vk::PipelineViewportStateCreateInfo viewportState{};
	viewportState.viewportCount = 1;
	viewportState.scissorCount = 1;
	viewportState.flags = {};

	vk::PipelineMultisampleStateCreateInfo multisampleState{};
	
	std::vector<vk::DynamicState> dynamicStateEnables = {
		vk::DynamicState::eViewport,
		vk::DynamicState::eScissor
	};
	vk::PipelineDynamicStateCreateInfo dynamicState{ {},dynamicStateEnables };

	std::array<vk::PipelineShaderStageCreateInfo, 2> shaderStages{};

	vk::GraphicsPipelineCreateInfo pipelineCreateInfo{};
	pipelineCreateInfo.layout = _pipelineLayout;
	pipelineCreateInfo.renderPass = renderPass.operator vk::RenderPass &();
	pipelineCreateInfo.flags = (vk::PipelineCreateFlagBits)0;
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
	std::vector<vk::VertexInputBindingDescription> vertexInputBindings = {
		vk::VertexInputBindingDescription{0,sizeof(ImDrawVert),vk::VertexInputRate::eVertex},
	};

	vk::VertexInputAttributeDescription vInputAttribDescription1{};
	vInputAttribDescription1.location = 0;
	vInputAttribDescription1.binding = 0;
	vInputAttribDescription1.format = vk::Format::eR32G32Sfloat;
	vInputAttribDescription1.offset = offsetof(ImDrawVert, pos);
	vk::VertexInputAttributeDescription vInputAttribDescription2{};
	vInputAttribDescription2.location = 1;
	vInputAttribDescription2.binding = 0;
	vInputAttribDescription2.format = vk::Format::eR32G32Sfloat;
	vInputAttribDescription2.offset = offsetof(ImDrawVert, uv);
	vk::VertexInputAttributeDescription vInputAttribDescription3{};
	vInputAttribDescription3.location = 2;
	vInputAttribDescription3.binding = 0;
	vInputAttribDescription3.format = vk::Format::eR8G8B8A8Unorm;
	vInputAttribDescription3.offset = offsetof(ImDrawVert, col);
	std::vector<vk::VertexInputAttributeDescription> vertexInputAttributes = {
		vInputAttribDescription1,vInputAttribDescription2,vInputAttribDescription3
	};
	vk::PipelineVertexInputStateCreateInfo vertexInputState{ {},vertexInputBindings,vertexInputAttributes };
	
	pipelineCreateInfo.pVertexInputState = &vertexInputState;

	auto vertexShaderModule = Shader(*_device, "shaders/imgui.vert.spv");
	auto fragmentShaderModule = Shader(*_device, "shaders/imgui.frag.spv");
	shaderStages[0].stage = vk::ShaderStageFlagBits::eVertex;
	shaderStages[0].pName = "main";
	shaderStages[0].module = vertexShaderModule;

	shaderStages[1].stage = vk::ShaderStageFlagBits::eFragment;
	shaderStages[1].pName = "main";
	shaderStages[1].module = fragmentShaderModule;

	vk::Result result;
	std::tie(result,_pipeline) = _device->logical.createGraphicsPipeline(_pipelineCache, pipelineCreateInfo);

	vertexShaderModule.destroy(*_device);
	fragmentShaderModule.destroy(*_device);

	ImGui_ImplGlfw_InitForVulkan(window, true);
	ImGui_ImplVulkan_InitInfo init_info = {};
	init_info.Instance = Instance;
	init_info.PhysicalDevice = _device->physical;
	init_info.Device = _device->logical;
	init_info.QueueFamily = ImGui_ImplVulkanH_SelectQueueFamilyIndex(_device->physical);
	VkQueue queue = _device->GetQueue(init_info.QueueFamily);
	init_info.Queue = queue;
	init_info.PipelineCache = _pipelineCache;
	init_info.DescriptorPool = *_descriptorPool;
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
	_device->logical.destroyPipelineCache(_pipelineCache);
	_device->logical.destroyPipeline(_pipeline);
	_device->logical.destroyPipelineLayout(_pipelineLayout);
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
void GUI::AddText(const std::string text)
{
	ImGui::Text(text.c_str());
}
void GUI::AddText(const char* format, ...)
{
	va_list args;
	va_start(args, format);

	// 필요한 버퍼 크기 계산
	int size = std::vsnprintf(nullptr, 0, format, args) + 1;
	va_end(args);

	if (size <= 0) return;

	std::vector<char> buffer(size);
	va_start(args, format);
	std::vsnprintf(buffer.data(), size, format, args);
	va_end(args);

	// 최종 문자열 전달
	AddText(std::string(buffer.data()));
}
void GUI::End(){

	ImGui::Render();
}
void GUI::updateBuffers()
{
	ImDrawData* imDrawData = ImGui::GetDrawData();

	vk::DeviceSize vertexBufferSize = imDrawData->TotalVtxCount * sizeof(ImDrawVert);
	vk::DeviceSize indexBufferSize = imDrawData->TotalIdxCount * sizeof(ImDrawIdx);

	if ((vertexBufferSize == 0) || (indexBufferSize == 0)) {
		return;
	}
	
	// Vertex buffer
	if ((_vertexBuffer == VK_NULL_HANDLE) || (_vertexCount != imDrawData->TotalVtxCount)) {
		_vertexBuffer.unmap(*_device);
		_vertexBuffer.destroy(*_device);
		_vertexBuffer = Buffer(*_device, vertexBufferSize, vk::BufferUsageFlagBits::eVertexBuffer, vk::MemoryPropertyFlagBits::eHostVisible);
		_vertexCount = imDrawData->TotalVtxCount;
		_vertexBuffer.map(*_device);
	}

	// Index buffer
	if ((_indexBuffer == VK_NULL_HANDLE) || (_indexCount < imDrawData->TotalIdxCount)) {
		_indexBuffer.unmap(*_device);
		_indexBuffer.destroy(*_device);
		_indexBuffer = Buffer(*_device, indexBufferSize, vk::BufferUsageFlagBits::eIndexBuffer, vk::MemoryPropertyFlagBits::eHostVisible);
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
}

void GUI::initDescriptorSet()
{
	vk::DescriptorImageInfo imageInfo{};
	imageInfo.sampler = Sampler::Get(SamplerMipMapType::Low);
	imageInfo.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
	imageInfo.imageView = _fontImage;

	vk::WriteDescriptorSet descriptorWrite{};
	descriptorWrite.dstSet = _descriptorSet;
	descriptorWrite.dstBinding = 0;
	descriptorWrite.dstArrayElement = 0;
	descriptorWrite.descriptorType = vk::DescriptorType::eCombinedImageSampler;
	descriptorWrite.descriptorCount = 1;
	descriptorWrite.pImageInfo = &imageInfo;

	_device->logical.updateDescriptorSets({ descriptorWrite },nullptr);


}
