#include "pch.h"
class VulkanApp {
public:
	void run() {
		initWindow();
		initVulkan();
		mainLoop();
		cleanup();
	}
private:
	GUI imgui;
	Device device;
	SwapChain swapChain;
	CommandPool commandPool;
	Surface surface;
	std::vector<CommandBuffer> commandBuffers;
	Image depthImage;
	ImageView depthImageView;
	std::vector<Buffer> uniformBuffers;
	std::vector<void*> uniformBuffersMapped;

	std::vector<Buffer> GUIBuffers;
	std::vector<void*> GUIBuffersMapped;
	GUIControl guiControl{};
	std::vector<Model> models;
	Model skybox;
	std::vector<DescriptorSet> uboDescriptorSets;
	std::vector<DescriptorSet> GUIDescriptorSets;

	DescriptorPool descriptorPool;
	std::vector<DescriptorSetLayout> descriptorSetLayouts;
	std::vector<std::vector<VkDescriptorSetLayout>> descriptorSetLayoutList;

	std::vector<Pipeline> pipelines;

	Camera camera;
	GLFWwindow* window;
	Instance instance;
	std::vector<VkSemaphore> imageAvailableSemaphores;
	std::vector<VkSemaphore> renderFinishedSemaphores;
	std::vector<VkFence> inFlightFences;
	uint32_t currentFrame = 0;
	bool keyPressed[256] = { false, };

	bool framebufferResized = false;
	bool enableInput = true;

	float previousX = 0, previousY = 0;
	float currentX = 0, currentY = 0;
	float mouse_dx = 0, mouse_dy = 0;

	static void keyboardInput(GLFWwindow* window, int key, int scancode, int action, int mods) {
		auto app = reinterpret_cast<VulkanApp*> (glfwGetWindowUserPointer(window));

		if (action == GLFW_PRESS) {
			app->keyPressed[key] = true;

			if (key == GLFW_KEY_F) app->enableInput = !app->enableInput;
		}

		if (action == GLFW_RELEASE) {
			app->keyPressed[key] = false;
		}
	}
	static void mouseInput(GLFWwindow* window, double xpos, double ypos) {
		auto app = reinterpret_cast<VulkanApp*> (glfwGetWindowUserPointer(window));
		if (app->enableInput) {
			app->currentX = xpos;
			app->currentY = ypos;
		}
	}

	static void framebufferResizeCallback(GLFWwindow* window, int width, int height) {
		auto app = reinterpret_cast<VulkanApp*> (glfwGetWindowUserPointer(window));
		app->framebufferResized = true;
	}
	void initWindow() {
		glfwInit();

		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

		window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
		glfwSetWindowUserPointer(window, this);
		glfwSetCursorPosCallback(window, mouseInput);
		glfwSetKeyCallback(window, keyboardInput);
		glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
	}
	void initVulkan() {
		instance = Instance("DDing");
		surface = Surface(instance, window);
		device = Device(instance, surface);
		descriptorPool = DescriptorPool(device);
		commandPool = CommandPool(device, surface);
		swapChain = SwapChain(device, surface);

		initGUI();
		createDescriptorSetLayouts();
		InsertModels();
		createUniformBuffers();
		createDescriptorSets();
		createPipelines();
		for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
			CommandBuffer cb = CommandBuffer(device, commandPool);
			commandBuffers.push_back(cb);
		}
		createSyncObjects();
	}
	void createDescriptorSetLayouts() {
		descriptorSetLayouts =
		{
			DescriptorSetLayout(device,DescriptorType::VP),
			DescriptorSetLayout(device,DescriptorType::Skybox),
			DescriptorSetLayout(device,DescriptorType::Material),
			DescriptorSetLayout(device,DescriptorType::Model),
			DescriptorSetLayout(device,DescriptorType::GBuffer),
		};
	}
	void initGUI() {

		imgui = GUI(device);
		imgui.init(static_cast<float>(WIDTH), static_cast<float>(HEIGHT));
		imgui.initResources(window,instance.Get(), swapChain.GetRenderPass());
	}
	void createPipelines() {
		pipelines.resize(3);
		descriptorSetLayoutList = {
			//기본 셰이더
			{descriptorSetLayouts[0].Get(),
			descriptorSetLayouts[1].Get(),
			descriptorSetLayouts[4].Get()},
			//스카이박스 셰이더
			{descriptorSetLayouts[0].Get(),
		descriptorSetLayouts[1].Get()},
		//디퍼드 셰이더
			{descriptorSetLayouts[0].Get(),
				descriptorSetLayouts[2].Get(),
		descriptorSetLayouts[3].Get(),
		descriptorSetLayouts[0].Get()}
		};
		Pipeline defaultPipeline = Pipeline(device,
			swapChain.GetExtent(),
			descriptorSetLayoutList,
			swapChain.GetRenderPass(),
			"shaders/shader.vert.spv",
			"shaders/shader.frag.spv",
			ShaderType::DEFAULT);

		Pipeline skyboxPipeline = Pipeline(device,
			swapChain.GetExtent(),
			descriptorSetLayoutList,
			swapChain.GetRenderPass(),
			"shaders/skybox.vert.spv",
			"shaders/skybox.frag.spv",
			ShaderType::SKYBOX);

		Pipeline deferredPipeline = Pipeline(device,
			swapChain.GetExtent(),
			descriptorSetLayoutList,
			swapChain.GetDeferredRenderPass(),
			"shaders/deferred.vert.spv",
			"shaders/deferred.frag.spv",
			ShaderType::DEFERRED);

		pipelines[Pipeline::DEFAULT] = (defaultPipeline);
		pipelines[Pipeline::SKYBOX] = (skyboxPipeline);
		pipelines[Pipeline::DEFERRED] = (deferredPipeline);
	}
	void InsertModels() {
		//Model model = makeBox(device, 1.0f, "Resources/models/Bricks075A_1K-PNG/Bricks075A_1K-PNG_Color.png", "Resources/models/Bricks075A_1K-PNG/Bricks075A_1K-PNG_NormalDX.png");
	Model model2 = Model(device, 1.f
			, { MaterialComponent::ALBEDO, MaterialComponent::NORMAL, MaterialComponent::ROUGHNESS, MaterialComponent::ao },
			"Resources/models/vk2vcdl/vk2vcdl.fbx",
			{ "Resources/models/vk2vcdl/vk2vcdl_4K_BaseColor.jpg",
			"Resources/models/vk2vcdl/vk2vcdl_4K_Normal.jpg",
			"Resources/models/vk2vcdl/vk2vcdl_4K_Roughness.jpg",
			"Resources/models/vk2vcdl/vk2vcdl_4K_AO.jpg" },
			glm::mat4(1.0f));
		Model model = makeSphere(device, glm::scale(glm::translate(glm::mat4(1.0f), glm::vec3(0, 0, 1)), glm::vec3(0.4f)) , { MaterialComponent::ALBEDO, MaterialComponent::NORMAL, MaterialComponent::ROUGHNESS, MaterialComponent::ao },
			{ "Resources/models/Bricks075A_1K-PNG/Bricks075A_1K-PNG_Color.png",
			"Resources/models/Bricks075A_1K-PNG/Bricks075A_1K-PNG_NormalDX.png",
			"Resources/models/Bricks075A_1K-PNG/Bricks075A_1K-PNG_Roughness.png",
			"Resources/models/Bricks075A_1K-PNG/Bricks075A_1K-PNG_AmbientOcclusion.png" });
		models.push_back(model);
		models.push_back(model2);

		skybox = makeSkyBox(device);
	}

	void createDescriptorSets() {
		Material::dummy = Material::GetDefaultMaterial(device);

		//GBuffer 디스크립터 셋
		swapChain.descriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
		for (auto& descriptorSet : swapChain.descriptorSets) {
			descriptorSet = DescriptorSet(device, descriptorPool, descriptorSetLayouts[static_cast<int>(DescriptorType::GBuffer)]);
		}
		
		//skybox용 디스크립터 셋
		skybox.material.descriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
		for (auto& descriptorSet : skybox.material.descriptorSets) {
			descriptorSet = DescriptorSet(device, descriptorPool, descriptorSetLayouts[static_cast<int>(DescriptorType::Skybox)]);
		}

		//머테리얼 디스크립터 셋
		for (auto& model : models) {
			model.material.descriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
			for (auto& descriptorSet : model.material.descriptorSets) {
				descriptorSet = DescriptorSet(device, descriptorPool, descriptorSetLayouts[static_cast<int>(DescriptorType::Material)]);
			}
		}
		//모델 행렬 디스크립터 셋
		for (auto& model : models) {
			model.descriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
			for (auto& descriptorSet : model.descriptorSets) {
				descriptorSet = DescriptorSet(device, descriptorPool, descriptorSetLayouts[static_cast<int>(DescriptorType::Model)]);
			}
		}
		//카메라 행렬 디스크립터 셋
		uboDescriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
		for (auto& descriptorSet : uboDescriptorSets) {
			descriptorSet = DescriptorSet(device, descriptorPool, descriptorSetLayouts[static_cast<int>(DescriptorType::VP)]);
		}
		//GUI 버퍼 디스크립터 셋
		GUIDescriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
		for (auto& descriptorSet : GUIDescriptorSets) {
			descriptorSet = DescriptorSet(device, descriptorPool, descriptorSetLayouts[static_cast<int>(DescriptorType::VP)]);
		}
		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
			for (auto& model : models) {
				model.InitDescriptorSet(device, model.material.descriptorSets[i]);
				model.InitDescriptorSetForModelMatrix(device, model.descriptorSets[i]);
			}
			
			//스카이박스
			skybox.InitDescriptorSetForSkybox(device, skybox.material.descriptorSets[i]);
			swapChain.InitDescriptorSetForGBuffer(device);
			//카메라 행렬 유니폼 버퍼
			VkDescriptorBufferInfo bufferInfo;
			bufferInfo.buffer = uniformBuffers[i].Get();
			bufferInfo.offset = 0;
			bufferInfo.range = sizeof(UniformBufferObject);

			VkWriteDescriptorSet descriptorWrite{};
			descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrite.dstSet = uboDescriptorSets[i].Get();
			descriptorWrite.dstBinding = 0;
			descriptorWrite.dstArrayElement = 0;
			descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			descriptorWrite.descriptorCount = 1;
			descriptorWrite.pBufferInfo = &bufferInfo;

			vkUpdateDescriptorSets(device.Get(), 1, &descriptorWrite, 0, nullptr);
		
			//GUI 행렬 유니폼 버퍼
			VkDescriptorBufferInfo guibufferInfo;
			guibufferInfo.buffer = GUIBuffers[i].Get();
			guibufferInfo.offset = 0;
			guibufferInfo.range = sizeof(GUIControl);

			VkWriteDescriptorSet guidescriptorWrite{};
			guidescriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			guidescriptorWrite.dstSet = GUIDescriptorSets[i].Get();
			guidescriptorWrite.dstBinding = 0;
			guidescriptorWrite.dstArrayElement = 0;
			guidescriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			guidescriptorWrite.descriptorCount = 1;
			guidescriptorWrite.pBufferInfo = &guibufferInfo;

			vkUpdateDescriptorSets(device.Get(), 1, &guidescriptorWrite, 0, nullptr);

		}
	}

	void createUniformBuffers() {
		VkDeviceSize bufferSize = sizeof(UniformBufferObject);

		uniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
		uniformBuffersMapped.resize(MAX_FRAMES_IN_FLIGHT);

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
			uniformBuffers[i] = Buffer(device, bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
			vkMapMemory(device.Get(), uniformBuffers[i].GetMemory(), 0, bufferSize, 0, &uniformBuffersMapped[i]);


		}
		VkDeviceSize guibufferSize = sizeof(GUIControl);

		GUIBuffers.resize(MAX_FRAMES_IN_FLIGHT);
		GUIBuffersMapped.resize(MAX_FRAMES_IN_FLIGHT);

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
			GUIBuffers[i] = Buffer(device, bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
			vkMapMemory(device.Get(), GUIBuffers[i].GetMemory(), 0, guibufferSize, 0, &GUIBuffersMapped[i]);


		}
	}


	void recreateSwapChain() {
		int width = 0, height = 0;
		glfwGetFramebufferSize(window, &width, &height);
		while (width == 0 || height == 0) {
			glfwGetFramebufferSize(window, &width, &height);
			glfwWaitEvents();
		}
		vkDeviceWaitIdle(device.Get());
		swapChain.destroy(device);
		swapChain.create(device);
		swapChain.InitDescriptorSetForGBuffer(device);
		//imgui.init(static_cast<float>(width), static_cast<float>(height));
		//imgui.initResources(swapChain.GetRenderPass());
	}

	void createSyncObjects() {
		imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
		renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
		inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

		VkSemaphoreCreateInfo semaphoreInfo{};
		semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		VkFenceCreateInfo fenceInfo{};
		fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {


			if (vkCreateSemaphore(device.Get(), &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]) != VK_SUCCESS ||
				vkCreateSemaphore(device.Get(), &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]) != VK_SUCCESS ||
				vkCreateFence(device.Get(), &fenceInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS) {
				throw std::runtime_error("failed to create semaphores!");
			}
		}
	}
	void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex, std::vector<Model>& models) {
		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = 0;
		beginInfo.pInheritanceInfo = nullptr;

		if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
			throw std::runtime_error("failed to begin recording command buffer!");
		}
		VkRenderPassBeginInfo deferredRenderPassInfo{};
		deferredRenderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		deferredRenderPassInfo.renderPass = swapChain.GetDeferredRenderPass().Get();
		deferredRenderPassInfo.framebuffer = swapChain.GetDeferredFrameBuffers()[imageIndex];
		deferredRenderPassInfo.renderArea.offset = { 0,0 };
		deferredRenderPassInfo.renderArea.extent = swapChain.GetExtent();

		std::array<VkClearValue, 7> deferredClearValues{};
		deferredClearValues[0].color = { {0.0f,0.0f,0.0f,0.0f} };
		deferredClearValues[1].color = { {0.0f,0.0f,0.0f,0.0f} };
		deferredClearValues[2].color = { {0.0f,0.0f,0.0f,0.0f} };
		deferredClearValues[3].color = { {0.0f,0.0f,0.0f,0.0f} };
		deferredClearValues[4].color = { {0.0f,0.0f,0.0f,0.0f} };
		deferredClearValues[5].color = { {0.0f,0.0f,0.0f,0.0f} };
		deferredClearValues[6].depthStencil = { 1.0f,0 };
		deferredRenderPassInfo.clearValueCount = static_cast<uint32_t>(deferredClearValues.size());
		deferredRenderPassInfo.pClearValues = deferredClearValues.data();

		VkViewport viewport{};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = static_cast<float>(swapChain.GetExtent().width);
		viewport.height = static_cast<float>(swapChain.GetExtent().height);
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;
		vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

		VkRect2D scissor{};
		scissor.offset = { 0,0 };
		scissor.extent = swapChain.GetExtent();
		vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
		vkCmdBeginRenderPass(commandBuffer, &deferredRenderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
		
		//GBuffer Draw
		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines[Pipeline::DEFERRED].Get());
		
		for (auto& model : models) {
			for (auto& mesh : model.meshes) {
				VkBuffer vertexBuffers[] = { mesh->vertexBuffer.Get() };
				VkDeviceSize offsets[] = { 0 };
				vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
				vkCmdBindIndexBuffer(commandBuffer, mesh->indexBuffer.Get(), 0, VK_INDEX_TYPE_UINT32);

				int maxMaterialCnt = static_cast<int>(MaterialComponent::END);
				VkBool32 data[5];
				for (int i = 0; i < maxMaterialCnt; i++) {
					data[i] = model.material.hasComponent(i);
				}
				vkCmdPushConstants(commandBuffer, pipelines[Pipeline::DEFERRED].GetLayout(), VK_SHADER_STAGE_FRAGMENT_BIT, 0, 20, data);


				std::vector<VkDescriptorSet> descriptorSetListForModel = {
					uboDescriptorSets[currentFrame].Get(),
					model.material.descriptorSets[currentFrame].Get(),
					model.descriptorSets[currentFrame].Get(),
					GUIDescriptorSets[currentFrame].Get()
				};
				vkCmdBindDescriptorSets(commandBuffer,
					VK_PIPELINE_BIND_POINT_GRAPHICS,
					pipelines[Pipeline::DEFERRED].GetLayout(),
					0, static_cast<uint32_t>(descriptorSetListForModel.size())
					, descriptorSetListForModel.data(),
					0, nullptr);

				vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(mesh->indices.size()), 1, 0, 0, 0);

			}
		}
		
		vkCmdEndRenderPass(commandBuffer);

		VkRenderPassBeginInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = swapChain.GetRenderPass().Get();
		renderPassInfo.framebuffer = swapChain.GetFrameBuffers()[imageIndex];
		renderPassInfo.renderArea.offset = { 0,0 };
		renderPassInfo.renderArea.extent = swapChain.GetExtent();

		std::array<VkClearValue, 2> clearValues{};
		clearValues[0].color = { {0.0f, 0.0f, 0.0f, 1.0f} };
		clearValues[1].depthStencil = { 1.0f, 0 };
		renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
		renderPassInfo.pClearValues = clearValues.data();

		imgui.newFrame();
		imgui.AddBoolGUI("EnableVerticalRotate", camera.enableVerticalRotate);
		imgui.AddBoolGUI("UseNormalMap", guiControl.useNormalMap);
		imgui.End();
		//imgui.updateBuffers();
		//SkyboxDraw
		vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);


		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines[Pipeline::SKYBOX].Get());


		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines[Pipeline::SKYBOX].Get());
		
		std::vector<VkDescriptorSet> descriptorSetListForSkybox = {
			uboDescriptorSets[currentFrame].Get(),
			skybox.material.descriptorSets[currentFrame].Get()
		};
		vkCmdBindDescriptorSets(commandBuffer,
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			pipelines[Pipeline::SKYBOX].GetLayout(),
			0, static_cast<uint32_t>(descriptorSetListForSkybox.size())
			, descriptorSetListForSkybox.data(),
			0, nullptr);

		VkBuffer vertexBuffers[] = { skybox.meshes[0]->vertexBuffer.Get() };
		VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
		vkCmdBindIndexBuffer(commandBuffer, skybox.meshes[0]->indexBuffer.Get(), 0, VK_INDEX_TYPE_UINT32);


		vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(skybox.meshes[0]->indices.size()), 1, 0, 0, 0);

		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines[Pipeline::DEFAULT].Get());
		std::vector<VkDescriptorSet> descriptorSetListForModel = {
					uboDescriptorSets[currentFrame].Get(),
					skybox.material.descriptorSets[currentFrame].Get(),
					swapChain.descriptorSets[currentFrame].Get()
		};
		vkCmdBindDescriptorSets(commandBuffer,
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			pipelines[Pipeline::DEFAULT].GetLayout(),
			0, static_cast<uint32_t>(descriptorSetListForModel.size())
			, descriptorSetListForModel.data(),
			0, nullptr);

		vkCmdDraw(commandBuffer, 3, 1, 0, 0);
		imgui.drawFrame(commandBuffer);
		/*for (auto& model : models) {
			for (auto& mesh : model.meshes) {
				vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines[Pipeline::DEFAULT].Get());

				VkBuffer vertexBuffers[] = { mesh->vertexBuffer.Get() };
				VkDeviceSize offsets[] = { 0 };
				vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
				vkCmdBindIndexBuffer(commandBuffer, mesh->indexBuffer.Get(), 0, VK_INDEX_TYPE_UINT32);

				int maxMaterialCnt = static_cast<int>(MaterialComponent::END);
				VkBool32 data[5];
				for (int i = 0; i < maxMaterialCnt; i++) {
					data[i] = model.material.hasComponent(i);
				}
				vkCmdPushConstants(commandBuffer, pipelines[Pipeline::DEFAULT].GetLayout(), VK_SHADER_STAGE_FRAGMENT_BIT, 0, 16, data);


				std::vector<VkDescriptorSet> descriptorSetListForModel = {
					uboDescriptorSets[currentFrame].Get(),
					skybox.material.descriptorSets[currentFrame].Get(),
					model.material.descriptorSets[currentFrame].Get(),
					model.descriptorSets[currentFrame].Get(),
				};
				vkCmdBindDescriptorSets(commandBuffer,
					VK_PIPELINE_BIND_POINT_GRAPHICS,
					pipelines[Pipeline::DEFAULT].GetLayout(),
					0, static_cast<uint32_t>(descriptorSetListForModel.size())
					, descriptorSetListForModel.data(),
					0, nullptr);

				vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(mesh->indices.size()), 1, 0, 0, 0);

			}
		}
		*/
		vkCmdEndRenderPass(commandBuffer);

		if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
			throw std::runtime_error("failed to record command buffer!");
		}

	}


	void mainLoop() {
		static auto previousTime = std::chrono::high_resolution_clock::now();

		while (!glfwWindowShouldClose(window)) {
			glfwPollEvents();

			auto currentTime = std::chrono::high_resolution_clock::now();
			float time = std::chrono::duration<float, std::chrono::milliseconds::period>(currentTime - previousTime).count();
			previousTime = currentTime;
			// FPS 계산
			float fps = 1000.0f / time;

			// 창 제목 업데이트
			char title[256];
			sprintf_s(title, "Vulkan %.2fms, %dFPS", time, static_cast<int>(fps));

			glfwSetWindowTitle(window, title);
			if (enableInput) {
				mouse_dx = currentX - previousX;
				mouse_dy = currentY - previousY;

				previousX = currentX;
				previousY = currentY;




				camera.Update(time / 1000.f, keyPressed, mouse_dx, mouse_dy);
			}
			drawFrame();
		}
		vkDeviceWaitIdle(device.Get());
	}
	void updateUniformBuffer(uint32_t currentImage) {
		UniformBufferObject ubo{};
		ubo.view = camera.GetView();
		ubo.proj = camera.GetProj(swapChain.GetExtent().width, swapChain.GetExtent().height);
		ubo.proj[1][1] *= -1;
		ubo.camPos = camera.GetPos();

		ubo.lights[0] = glm::vec4(0, 1, 0, 0);
		ubo.lights[1] = glm::vec4(1, 1, 1, 0);
		ubo.lights[2] = glm::vec4(1, 1, 1, 0);
		ubo.lights[3] = glm::vec4(1, 1, 1, 0);


		memcpy(uniformBuffersMapped[currentImage], &ubo, sizeof(ubo));

		
		memcpy(GUIBuffersMapped[currentImage], &guiControl, sizeof(GUIControl));
	}
	void drawFrame() {
		vkWaitForFences(device.Get(), 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

		uint32_t imageIndex;
		VkResult result = vkAcquireNextImageKHR(device.Get(), swapChain.Get(), UINT64_MAX, imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);
		if (result == VK_ERROR_OUT_OF_DATE_KHR) {
			recreateSwapChain();
			return;
		}
		else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
			throw std::runtime_error("failed to acquire swap chain image!");
		}
		vkResetFences(device.Get(), 1, &inFlightFences[currentFrame]);
		updateUniformBuffer(currentFrame);
		vkResetCommandBuffer(commandBuffers[currentFrame].Get(), 0);
		recordCommandBuffer(commandBuffers[currentFrame].Get(), imageIndex, models);
		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

		VkSemaphore waitSemaphores[] = { imageAvailableSemaphores[currentFrame] };
		VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = waitSemaphores;
		submitInfo.pWaitDstStageMask = waitStages;

		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffers[currentFrame].Get();

		VkSemaphore signalSemaphores[] = { renderFinishedSemaphores[currentFrame] };
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = signalSemaphores;

		if (vkQueueSubmit(device.GetQueue(Device::QueueType::GRAPHICS), 1, &submitInfo, inFlightFences[currentFrame]) != VK_SUCCESS) {
			throw std::runtime_error("failed to submit draw command buffer!");
		}

		VkPresentInfoKHR presentInfo{};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = signalSemaphores;

		VkSwapchainKHR swapChains[] = { swapChain.Get() };
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = swapChains;
		presentInfo.pImageIndices = &imageIndex;
		presentInfo.pResults = nullptr;

		result = vkQueuePresentKHR(device.GetQueue(Device::QueueType::PRESENT), &presentInfo);
		if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebufferResized) {
			framebufferResized = false;
			recreateSwapChain();
		}
		else if (result != VK_SUCCESS) {
			throw std::runtime_error("failed to present swap chain image!");
		}
		currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
	}

	void cleanup() {
		swapChain.destroy(device);
		imgui.destroy();
		vkDestroyImageView(device.Get(), Material::dummy.imageView.Get(), nullptr);
		vkDestroyImage(device.Get(), Material::dummy.image.Get(), nullptr);
		vkFreeMemory(device.Get(), Material::dummy.image.GetMemory(), nullptr);
		vkDestroySampler(device.Get(), Material::dummy.sampler.Get(), nullptr);
		vkDestroyDescriptorPool(device.Get(), descriptorPool.Get(), nullptr);
		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
			vkDestroyBuffer(device.Get(), uniformBuffers[i].Get(), nullptr);
			vkFreeMemory(device.Get(), uniformBuffers[i].GetMemory(), nullptr);
			vkDestroyBuffer(device.Get(), GUIBuffers[i].Get(), nullptr);
			vkFreeMemory(device.Get(), GUIBuffers[i].GetMemory(), nullptr);
		}

		for (auto& descriptorSetLayout : descriptorSetLayouts) {
			vkDestroyDescriptorSetLayout(device.Get(), descriptorSetLayout.Get(), nullptr);
		}

		for (auto& model : models) {
			model.destroy(device);
		}
		skybox.destroy(device);
		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
			vkDestroySemaphore(device.Get(), imageAvailableSemaphores[i], nullptr);
			vkDestroySemaphore(device.Get(), renderFinishedSemaphores[i], nullptr);
			vkDestroyFence(device.Get(), inFlightFences[i], nullptr);
		}
		vkDestroyCommandPool(device.Get(), commandPool.Get(), nullptr);
		vkDestroyCommandPool(device.Get(), CommandPool::TransientPool, nullptr);
		for (int i = 0; i < 3;i++) {
			auto& pipeline = pipelines[i];
			vkDestroyPipeline(device.Get(), pipeline.Get(), nullptr);
			vkDestroyPipelineLayout(device.Get(), pipeline.GetLayout(), nullptr);
		}
		vkDestroyDevice(device.Get(), nullptr);
		if (enableValidationLayers) {

			DestroyDebugUtilsMessengerEXT(instance.Get(), instance.GetDebugMessenger(), nullptr);
		}

		vkDestroySurfaceKHR(instance.Get(), surface.Get(), nullptr);
		vkDestroyInstance(instance.Get(), nullptr);

		glfwDestroyWindow(window);

		glfwTerminate();
	}

};

int main() {
	VulkanApp app;

	try {
		app.run();
	}
	catch (const std::exception& e) {
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}