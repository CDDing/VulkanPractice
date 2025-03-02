#include "pch.h"
#include "GUI.h"
GUI::GUI(Device& device, GLFWwindow* window, VkInstance Instance, vk::raii::RenderPass& renderPass) :
	vertexBuffer(nullptr), indexBuffer(nullptr), fontImage(nullptr), pipeline(nullptr), pipelineLayout(nullptr), pipelineCache(nullptr), descriptorPool(nullptr), descriptorSet(nullptr), descriptorSetLayout(nullptr)
{
	ImGui::CreateContext();

	ImGuiIO& io = ImGui::GetIO();
	io.FontGlobalScale = 1;

	ImGuiStyle& style = ImGui::GetStyle();
	style.ScaleAllSizes(1);


	vulkanStyle = ImGui::GetStyle();
	vulkanStyle.Colors[ImGuiCol_TitleBg] = ImVec4(1.0f, 0.0f, 0.0f, 0.6f);
	vulkanStyle.Colors[ImGuiCol_TitleBgActive] = ImVec4(1.0f, 0.0f, 0.0f, 0.8f);
	vulkanStyle.Colors[ImGuiCol_MenuBarBg] = ImVec4(1.0f, 0.0f, 0.0f, 0.4f);
	vulkanStyle.Colors[ImGuiCol_Header] = ImVec4(1.0f, 0.0f, 0.0f, 0.4f);
	vulkanStyle.Colors[ImGuiCol_CheckMark] = ImVec4(0.0f, 1.0f, 0.0f, 1.0f);

	setStyle(0);

	// Dimensions
	io.DisplaySize = ImVec2(WIDTH, HEIGHT);
	io.DisplayFramebufferScale = ImVec2(1.0f, 1.0f);


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
	fontImage = DImage(device,1,vk::Format::eR8G8B8A8Unorm,
		vk::Extent2D(texWidth, texHeight),
		vk::ImageTiling::eOptimal,
		vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst,
		vk::ImageLayout::eUndefined,
		vk::MemoryPropertyFlagBits::eDeviceLocal,
		vk::ImageAspectFlagBits::eColor);

	DBuffer stagingBuffer(device, uploadSize, vk::BufferUsageFlagBits::eTransferSrc,
		vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);

	stagingBuffer.map(device, uploadSize, 0);
	memcpy(stagingBuffer.mapped, fontData, static_cast<size_t>(uploadSize));
	stagingBuffer.unmap(device);

	vk::raii::CommandBuffer commandBuffer = beginSingleTimeCommands(device);
	fontImage.setImageLayout(commandBuffer, vk::ImageLayout::eTransferDstOptimal);
	fontImage.copyFromBuffer(commandBuffer, stagingBuffer.buffer);
	fontImage.setImageLayout(commandBuffer, vk::ImageLayout::eShaderReadOnlyOptimal);
	endSingleTimeCommands(device, commandBuffer);



	vk::DescriptorPoolSize pool_sizes[] =
	{
		{ vk::DescriptorType::eCombinedImageSampler, 10 * IMGUI_IMPL_VULKAN_MINIMUM_IMAGE_SAMPLER_POOL_SIZE },
	};
	vk::DescriptorPoolCreateInfo pool_info = {};
	pool_info.setFlags(vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet);
	pool_info.setMaxSets(0);
	for (vk::DescriptorPoolSize& pool_size : pool_sizes)
		pool_info.maxSets += pool_size.descriptorCount;
	pool_info.setPoolSizes(pool_sizes);

	descriptorPool = vk::raii::DescriptorPool(device, pool_info);


	std::vector<vk::DescriptorSetLayoutBinding> bindings;
	auto components = DescriptorSetLayout::GetComponents(DescriptorType::ImGUI);
	bindings = DescriptorSetLayout::inputAttributeDescriptions(components);
	vk::DescriptorSetLayoutCreateInfo layoutInfo{ {},bindings };

	descriptorSetLayout = vk::raii::DescriptorSetLayout(device, layoutInfo);

	vk::DescriptorSetAllocateInfo alloc_info = {};
	alloc_info.setDescriptorPool(*descriptorPool);
	alloc_info.setSetLayouts(*descriptorSetLayout);
	descriptorSet = std::move(vk::raii::DescriptorSets(device, alloc_info).front());
	initDescriptorSet(device);

	//파이프라인
	vk::PipelineCacheCreateInfo pipelineCacheCreateInfo = {};
	pipelineCache = vk::raii::PipelineCache(device, pipelineCacheCreateInfo);

	// Pipeline layout
	// Push constants for UI rendering parameters
	vk::PushConstantRange pushConstantRange{};
	pushConstantRange.size = sizeof(PushConstBlock);
	pushConstantRange.offset = 0;
	pushConstantRange.stageFlags = vk::ShaderStageFlagBits::eVertex;
	vk::PipelineLayoutCreateInfo pipelineLayoutCreateInfo{ {},{*descriptorSetLayout},{pushConstantRange } };

	pipelineLayout = vk::raii::PipelineLayout(device, pipelineLayoutCreateInfo);

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
	blendAttachmentState.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
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
	pipelineCreateInfo.layout = pipelineLayout;
	pipelineCreateInfo.renderPass = renderPass;
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

	auto vertexShaderModule = createShader(device, "shaders/imgui.vert.spv");
	auto fragmentShaderModule = createShader(device, "shaders/imgui.frag.spv");
	shaderStages[0].stage = vk::ShaderStageFlagBits::eVertex;
	shaderStages[0].pName = "main";
	shaderStages[0].module = vertexShaderModule;

	shaderStages[1].stage = vk::ShaderStageFlagBits::eFragment;
	shaderStages[1].pName = "main";
	shaderStages[1].module = fragmentShaderModule;

	pipeline = vk::raii::Pipeline(device, pipelineCache, pipelineCreateInfo);

	ImGui_ImplGlfw_InitForVulkan(window, true);
	ImGui_ImplVulkan_InitInfo init_info = {};
	init_info.Instance = Instance;
	init_info.PhysicalDevice = *device.physical;
	init_info.Device = *device.logical;
	init_info.QueueFamily = ImGui_ImplVulkanH_SelectQueueFamilyIndex(*device.physical);
	VkQueue queue = device.GetQueue(init_info.QueueFamily);
	init_info.Queue = queue;
	init_info.PipelineCache = *pipelineCache;
	init_info.DescriptorPool = *descriptorPool;
	init_info.RenderPass = *renderPass;
	init_info.Subpass = 0;
	init_info.MinImageCount = 2;
	init_info.ImageCount = 2;
	init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
	init_info.Allocator = nullptr;
	init_info.CheckVkResultFn = check_vk_result;
	ImGui_ImplVulkan_Init(&init_info);
}

GUI::~GUI()
{

	ImGui_ImplVulkan_Shutdown();
	ImGui_ImplGlfw_Shutdown();
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
void GUI::updateBuffers(Device& device)
{
	ImDrawData* imDrawData = ImGui::GetDrawData();

	vk::DeviceSize vertexBufferSize = imDrawData->TotalVtxCount * sizeof(ImDrawVert);
	vk::DeviceSize indexBufferSize = imDrawData->TotalIdxCount * sizeof(ImDrawIdx);

	if ((vertexBufferSize == 0) || (indexBufferSize == 0)) {
		return;
	}
	
	// Vertex buffer
	if ((vertexBuffer.buffer == VK_NULL_HANDLE) || (vertexCount != imDrawData->TotalVtxCount)) {
		vertexBuffer.unmap(device);
		vertexBuffer = DBuffer(device, vertexBufferSize, vk::BufferUsageFlagBits::eVertexBuffer, vk::MemoryPropertyFlagBits::eHostVisible);
		vertexCount = imDrawData->TotalVtxCount;
		vertexBuffer.map(device);
	}

	// Index buffer
	if ((indexBuffer.buffer == VK_NULL_HANDLE) || (indexCount < imDrawData->TotalIdxCount)) {
		indexBuffer.unmap(device);
		indexBuffer = DBuffer(device, indexBufferSize, vk::BufferUsageFlagBits::eIndexBuffer, vk::MemoryPropertyFlagBits::eHostVisible);
		indexCount = imDrawData->TotalIdxCount;
		indexBuffer.map(device);
	}

	// Upload data
	ImDrawVert* vtxDst = (ImDrawVert*)vertexBuffer.mapped;
	ImDrawIdx* idxDst = (ImDrawIdx*)indexBuffer.mapped;

	for (int n = 0; n < imDrawData->CmdListsCount; n++) {
		const ImDrawList* cmd_list = imDrawData->CmdLists[n];
		memcpy(vtxDst, cmd_list->VtxBuffer.Data, cmd_list->VtxBuffer.Size * sizeof(ImDrawVert));
		memcpy(idxDst, cmd_list->IdxBuffer.Data, cmd_list->IdxBuffer.Size * sizeof(ImDrawIdx));
		vtxDst += cmd_list->VtxBuffer.Size;
		idxDst += cmd_list->IdxBuffer.Size;
	}

	// Flush to make writes visible to GPU
	vertexBuffer.flush(device);
	indexBuffer.flush(device);
}

void GUI::drawFrame(vk::raii::CommandBuffer& commandBuffer)
{
	ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), *commandBuffer, *pipeline);

}
void GUI::initDescriptorSet(Device& device)
{
	vk::DescriptorImageInfo imageInfo{};
	imageInfo.sampler = Sampler::Get(SamplerMipMapType::Low);
	imageInfo.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
	imageInfo.imageView = fontImage.view;

	vk::WriteDescriptorSet descriptorWrite{};
	descriptorWrite.dstSet = descriptorSet;
	descriptorWrite.dstBinding = 0;
	descriptorWrite.dstArrayElement = 0;
	descriptorWrite.descriptorType = vk::DescriptorType::eCombinedImageSampler;
	descriptorWrite.descriptorCount = 1;
	descriptorWrite.pImageInfo = &imageInfo;

	device.logical.updateDescriptorSets({ descriptorWrite },nullptr);


}
