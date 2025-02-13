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
	std::shared_ptr<Instance> instance;
	std::shared_ptr<Surface> surface;
	std::shared_ptr<Device> device;
	std::shared_ptr<DescriptorPool> descriptorPool;
	std::shared_ptr<CommandPool> commandPool;
	std::vector<std::shared_ptr<Buffer>> uniformBuffers;
	std::vector<std::shared_ptr<Buffer>> GUIBuffers;

	GUI imgui;
	SwapChain swapChain;
	std::vector<vk::CommandBuffer> commandBuffers;
	GUIControl guiControl{};
	Scene scene;
	std::vector<DescriptorSet> uboDescriptorSets;
	std::vector<DescriptorSet> GUIDescriptorSets;

	std::vector<DescriptorSetLayout> descriptorSetLayouts;
	std::vector<std::vector<vk::DescriptorSetLayout>> descriptorSetLayoutList;

	std::vector<Pipeline> pipelines;

	Camera camera;
	GLFWwindow* window;
	std::vector<std::shared_ptr<Semaphore>> imageAvailableSemaphores;
	std::vector<std::shared_ptr<Semaphore>> renderFinishedSemaphores;
	std::vector<std::shared_ptr<Fence>> inFlightFences;
	uint32_t currentFrame = 0;

	RayTracing rt;
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
		instance = std::make_shared<Instance>("DDing");
		surface = std::make_shared<Surface>(instance, window);
		device = std::make_shared<Device>(instance, surface);
		descriptorPool = std::make_shared<DescriptorPool>(device);
		commandPool = std::make_shared<CommandPool>(device, findQueueFamilies(*device,*surface));
		initSamplers();
		swapChain = SwapChain(device, surface);
		
		initGUI();
		createDescriptorSetLayouts();
		insertModels();
		createUniformBuffers();
		createDescriptorSets();
		createPipelines();
		initRayTracing();
		for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
			vk::CommandBufferAllocateInfo allocInfo{ *commandPool,vk::CommandBufferLevel::ePrimary,1 };
			vk::CommandBuffer cb = device->logical.allocateCommandBuffers(allocInfo).front();
			commandBuffers.push_back(cb);
		}
		createSyncObjects();
	}
	void createDescriptorSetLayouts() {
		descriptorSetLayouts =
		{
			DescriptorSetLayout(*device,DescriptorType::VP),
			DescriptorSetLayout(*device,DescriptorType::Skybox),
			DescriptorSetLayout(*device,DescriptorType::Material),
			DescriptorSetLayout(*device,DescriptorType::Model),
			DescriptorSetLayout(*device,DescriptorType::GBuffer),
		};
	}
	void initSamplers() {
		Sampler::init(*device);
	}
	void initRayTracing() {

		rt = RayTracing();
		rt.init(device,uniformBuffers,swapChain,scene,GUIBuffers);
	}
	void initGUI() {

		imgui = GUI(device);
		imgui.init(static_cast<float>(WIDTH), static_cast<float>(HEIGHT));
		imgui.initResources(window,*instance, swapChain.GetRenderPass());
	}
	void createPipelines() {
		pipelines.resize(3);
		descriptorSetLayoutList = {
			//기본 셰이더
			{descriptorSetLayouts[0],
			descriptorSetLayouts[1],
			descriptorSetLayouts[4]},
			//스카이박스 셰이더
			{descriptorSetLayouts[0],
		descriptorSetLayouts[1]},
		//디퍼드 셰이더
			{descriptorSetLayouts[0],
				descriptorSetLayouts[2],
		descriptorSetLayouts[3],
		descriptorSetLayouts[0]}
		};
		Pipeline defaultPipeline = Pipeline(*device,
			swapChain.GetExtent(),
			descriptorSetLayoutList,
			swapChain.GetRenderPass(),
			"shaders/shader.vert.spv",
			"shaders/shader.frag.spv",
			ShaderType::DEFAULT);

		Pipeline skyboxPipeline = Pipeline(*device,
			swapChain.GetExtent(),
			descriptorSetLayoutList,
			swapChain.GetRenderPass(),
			"shaders/skybox.vert.spv",
			"shaders/skybox.frag.spv",
			ShaderType::SKYBOX);

		Pipeline deferredPipeline = Pipeline(*device,
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
	void insertModels() {
		//Model model = makeBox(*device, 1.0f, "Resources/models/Bricks075A_1K-PNG/Bricks075A_1K-PNG_Color.png", "Resources/models/Bricks075A_1K-PNG/Bricks075A_1K-PNG_NormalDX.png");
		scene.models.push_back(makeSqaure(device, glm::translate(glm::mat4(30.0f),glm::vec3(0,-2.f/30.f,0)),
			{}, 
			{}));
		scene.models.push_back(Model(device, 1.f
			, { MaterialComponent::ALBEDO, MaterialComponent::NORMAL, MaterialComponent::ROUGHNESS, MaterialComponent::ao },
			"Resources/models/vk2vcdl/vk2vcdl.fbx",
			{ "Resources/models/vk2vcdl/vk2vcdl_4K_BaseColor.jpg",
			"Resources/models/vk2vcdl/vk2vcdl_4K_Normal.jpg",
			"Resources/models/vk2vcdl/vk2vcdl_4K_Roughness.jpg",
			"Resources/models/vk2vcdl/vk2vcdl_4K_AO.jpg" },
			glm::rotate(glm::translate(glm::mat4(1.0f),glm::vec3(1.5,0,2)), glm::radians(45.0f), glm::vec3(-2, 3, 1))));

		scene.models.push_back(makeSphere(device, glm::scale(glm::translate(glm::mat4(1.0f), glm::vec3(0, 0, 1)), glm::vec3(0.4f)) ,
			{ },
			{ }));
		scene.skybox = makeSkyBox(device);
	}

	void createDescriptorSets() {
		Material::dummy = Material::GetDefaultMaterial(device);

		//GBuffer 디스크립터 셋
		swapChain.descriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
		for (auto& descriptorSet : swapChain.descriptorSets) {
			descriptorSet = DescriptorSet(*device, *descriptorPool, descriptorSetLayouts[static_cast<int>(DescriptorType::GBuffer)]);
		}
		
		//skybox용 디스크립터 셋
		scene.skybox.material.descriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
		for (auto& descriptorSet : scene.skybox.material.descriptorSets) {
			descriptorSet = DescriptorSet(*device, *descriptorPool, descriptorSetLayouts[static_cast<int>(DescriptorType::Skybox)]);
		}

		//머테리얼 디스크립터 셋
		for (auto& model : scene.models) {
			model.material.descriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
			for (auto& descriptorSet : model.material.descriptorSets) {
				descriptorSet = DescriptorSet(*device, *descriptorPool, descriptorSetLayouts[static_cast<int>(DescriptorType::Material)]);
			}
		}
		//모델 행렬 디스크립터 셋
		for (auto& model : scene.models) {
			model.descriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
			for (auto& descriptorSet : model.descriptorSets) {
				descriptorSet = DescriptorSet(*device, *descriptorPool, descriptorSetLayouts[static_cast<int>(DescriptorType::Model)]);
			}
		}
		//카메라 행렬 디스크립터 셋
		uboDescriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
		for (auto& descriptorSet : uboDescriptorSets) {
			descriptorSet = DescriptorSet(*device, *descriptorPool, descriptorSetLayouts[static_cast<int>(DescriptorType::VP)]);
		}
		//GUI 버퍼 디스크립터 셋
		GUIDescriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
		for (auto& descriptorSet : GUIDescriptorSets) {
			descriptorSet = DescriptorSet(*device, *descriptorPool, descriptorSetLayouts[static_cast<int>(DescriptorType::VP)]);
		}
		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
			for (auto& model : scene.models) {
				model.InitDescriptorSet(model.material.descriptorSets[i]);
				model.InitDescriptorSetForModelMatrix(model.descriptorSets[i]);
			}
			
			//스카이박스
			scene.skybox.InitDescriptorSetForSkybox(scene.skybox.material.descriptorSets[i]);
			swapChain.InitDescriptorSetForGBuffer();
			//카메라 행렬 유니폼 버퍼
			vk::DescriptorBufferInfo bufferInfo;
			bufferInfo.buffer = *uniformBuffers[i];
			bufferInfo.offset = 0;
			bufferInfo.range = sizeof(UniformBufferObject);

			vk::WriteDescriptorSet descriptorWrite{};
			descriptorWrite.dstSet = uboDescriptorSets[i];
			descriptorWrite.dstBinding = 0;
			descriptorWrite.dstArrayElement = 0;
			descriptorWrite.descriptorType = vk::DescriptorType::eUniformBuffer;
			descriptorWrite.descriptorCount = 1;
			descriptorWrite.pBufferInfo = &bufferInfo;

			device->logical.updateDescriptorSets(descriptorWrite, nullptr);
		
			//GUI 행렬 유니폼 버퍼
			vk::DescriptorBufferInfo guibufferInfo;
			guibufferInfo.buffer = *GUIBuffers[i];
			guibufferInfo.offset = 0;
			guibufferInfo.range = sizeof(GUIControl);

			vk::WriteDescriptorSet guidescriptorWrite{};
			guidescriptorWrite.dstSet = GUIDescriptorSets[i];
			guidescriptorWrite.dstBinding = 0;
			guidescriptorWrite.dstArrayElement = 0;
			guidescriptorWrite.descriptorType = vk::DescriptorType::eUniformBuffer;
			guidescriptorWrite.descriptorCount = 1;
			guidescriptorWrite.pBufferInfo = &guibufferInfo;

			device->logical.updateDescriptorSets(guidescriptorWrite, nullptr);

		}
	}

	void createUniformBuffers() {
		VkDeviceSize bufferSize = sizeof(UniformBufferObject);

		uniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
			uniformBuffers[i] = std::make_shared<Buffer>(device, bufferSize, vk::BufferUsageFlagBits::eUniformBuffer, vk::MemoryPropertyFlagBits::eHostVisible| vk::MemoryPropertyFlagBits::eHostCoherent);
			uniformBuffers[i]->map(bufferSize, 0);

		}
		VkDeviceSize guibufferSize = sizeof(GUIControl);

		GUIBuffers.resize(MAX_FRAMES_IN_FLIGHT);
		
		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
			GUIBuffers[i] = std::make_shared<Buffer>(device, bufferSize, vk::BufferUsageFlagBits::eUniformBuffer, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
			GUIBuffers[i]->map(guibufferSize, 0);

		}
	}


	void recreateSwapChain() {
		int width = 0, height = 0;
		glfwGetFramebufferSize(window, &width, &height);
		while (width == 0 || height == 0) {
			glfwGetFramebufferSize(window, &width, &height);
			glfwWaitEvents();
		}
		device->logical.waitIdle();
		swapChain.destroy();
		swapChain.create();
		swapChain.InitDescriptorSetForGBuffer();
		//imgui.init(static_cast<float>(width), static_cast<float>(height));
		//imgui.initResources(swapChain.GetRenderPass());
	}

	void createSyncObjects() {
		imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
		renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
		inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);



		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {

			imageAvailableSemaphores[i] = std::make_shared<Semaphore>(device);
			renderFinishedSemaphores[i] = std::make_shared<Semaphore>(device);
			inFlightFences[i] = std::make_shared<Fence>(device);		
		}
	}
	void recordCommandBuffer(vk::CommandBuffer commandBuffer, uint32_t imageIndex, std::vector<Model>& models) {
		vk::CommandBufferBeginInfo beginInfo{};

		imgui.newFrame();
		imgui.AddText("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
		imgui.AddBoolGUI("EnableVerticalRotate", camera.enableVerticalRotate);
		imgui.AddBoolGUI("UseNormalMap", guiControl.useNormalMap);
		imgui.AddBoolGUI("RayTracing", guiControl.RayTracing);
		imgui.AddFloatGUI("Roughness", guiControl.roughness, 0.0f, 1.0f);
		imgui.AddFloatGUI("metallic", guiControl.metallic, 0.0f, 1.0f);
		imgui.End();

		commandBuffer.begin(beginInfo);

		if (guiControl.RayTracing) {

			rt.recordCommandBuffer(commandBuffer, currentFrame, imageIndex);
			vk::RenderPassBeginInfo renderPassInfo{};
			renderPassInfo.renderPass = swapChain.GetPostRenderPass().operator vk::RenderPass &();
			renderPassInfo.framebuffer = swapChain.GetPostFrameBuffers()[imageIndex];
			renderPassInfo.renderArea.offset = vk::Offset2D(0,0);
			renderPassInfo.renderArea.extent = swapChain.GetExtent();

			std::array<vk::ClearValue, 2> clearValues{};
			clearValues[1].depthStencil = vk::ClearDepthStencilValue( 1.0f, 0 );
			renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
			renderPassInfo.pClearValues = clearValues.data();

			commandBuffer.beginRenderPass(renderPassInfo, vk::SubpassContents::eInline);
			imgui.drawFrame(commandBuffer);
			commandBuffer.endRenderPass();

		}
		else {

			vk::RenderPassBeginInfo deferredRenderPassInfo{};
			deferredRenderPassInfo.renderPass = swapChain.GetDeferredRenderPass().operator vk::RenderPass &();
			deferredRenderPassInfo.framebuffer = swapChain.GetDeferredFrameBuffers()[imageIndex];
			deferredRenderPassInfo.renderArea.offset = vk::Offset2D( 0,0 );
			deferredRenderPassInfo.renderArea.extent = swapChain.GetExtent();

			std::array<vk::ClearValue, 7> deferredClearValues{};
			deferredClearValues[0].color = vk::ClearColorValue{0.0f,0.0f,0.0f,0.0f};
			deferredClearValues[1].color = vk::ClearColorValue{0.0f,0.0f,0.0f,0.0f};
			deferredClearValues[2].color = vk::ClearColorValue{0.0f,0.0f,0.0f,0.0f};
			deferredClearValues[3].color = vk::ClearColorValue{0.0f,0.0f,0.0f,0.0f};
			deferredClearValues[4].color = vk::ClearColorValue{0.0f,0.0f,0.0f,0.0f};
			deferredClearValues[5].color = vk::ClearColorValue{0.0f,0.0f,0.0f,0.0f};
			deferredClearValues[6].depthStencil = vk::ClearDepthStencilValue(1.0f, 0);
			deferredRenderPassInfo.clearValueCount = static_cast<uint32_t>(deferredClearValues.size());
			deferredRenderPassInfo.pClearValues = deferredClearValues.data();

			vk::Viewport viewport{};
			viewport.x = 0.0f;
			viewport.y = 0.0f;
			viewport.width = static_cast<float>(swapChain.GetExtent().width);
			viewport.height = static_cast<float>(swapChain.GetExtent().height);
			viewport.minDepth = 0.0f;
			viewport.maxDepth = 1.0f;
			commandBuffer.setViewport(0,viewport);

			vk::Rect2D scissor{};
			scissor.offset = vk::Offset2D( 0,0 );
			scissor.extent = swapChain.GetExtent();
			
			commandBuffer.setScissor(0, scissor);
			commandBuffer.beginRenderPass(deferredRenderPassInfo, vk::SubpassContents::eInline);

			//GBuffer Draw
			commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pipelines[Pipeline::DEFERRED]);

			for (auto& model : models) {
				for (auto& mesh : model.meshes) {
					vk::Buffer vertexBuffers[] = { mesh->vertexBuffer };
					vk::DeviceSize offsets[] = { 0 };
					commandBuffer.bindVertexBuffers(0, vertexBuffers, offsets);
					commandBuffer.bindIndexBuffer(mesh->indexBuffer,0,vk::IndexType::eUint32);
					
					int maxMaterialCnt = static_cast<int>(MaterialComponent::END);
					std::vector<VkBool32> data(5);
					for (int i = 0; i < maxMaterialCnt; i++) {
						data[i] = model.material.hasComponent(i);
					}
					commandBuffer.pushConstants<VkBool32>(pipelines[Pipeline::DEFERRED].GetLayout(), vk::ShaderStageFlagBits::eFragment, 0, data);
					

					std::vector<vk::DescriptorSet> descriptorSetListForModel = {
						uboDescriptorSets[currentFrame],
						model.material.descriptorSets[currentFrame],
						model.descriptorSets[currentFrame],
						GUIDescriptorSets[currentFrame]
					};
					commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
						pipelines[Pipeline::DEFERRED].GetLayout(),
						0, descriptorSetListForModel, {});

					vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(mesh->indices.size()), 1, 0, 0, 0);

				}
			}

			vkCmdEndRenderPass(commandBuffer);

			vk::RenderPassBeginInfo renderPassInfo{};
			renderPassInfo.renderPass = swapChain.GetRenderPass().operator vk::RenderPass &();
			renderPassInfo.framebuffer = swapChain.GetFrameBuffers()[imageIndex];
			renderPassInfo.renderArea.offset = vk::Offset2D{ 0,0 };
			renderPassInfo.renderArea.extent = swapChain.GetExtent();

			std::array<vk::ClearValue, 2> clearValues{};
			clearValues[0].color = vk::ClearColorValue{0.0f, 0.0f, 0.0f, 1.0f};
			clearValues[1].depthStencil = vk::ClearDepthStencilValue{ 1.0f, 0 };
			renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
			renderPassInfo.pClearValues = clearValues.data();

			//imgui.updateBuffers();
			//SkyboxDraw
			commandBuffer.beginRenderPass(renderPassInfo, vk::SubpassContents::eInline);

			commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pipelines[Pipeline::SKYBOX]);

			std::vector<vk::DescriptorSet> descriptorSetListForSkybox = {
				uboDescriptorSets[currentFrame],
				scene.skybox.material.descriptorSets[currentFrame]
			};
			commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
				pipelines[Pipeline::SKYBOX].GetLayout(), 0, descriptorSetListForSkybox, {});

			vk::Buffer vertexBuffers[] = { scene.skybox.meshes[0]->vertexBuffer };
			vk::DeviceSize offsets[] = { 0 };
			
			
			commandBuffer.bindVertexBuffers(0, vertexBuffers, offsets);
			commandBuffer.bindIndexBuffer(scene.skybox.meshes[0]->indexBuffer, 0, vk::IndexType::eUint32);


			vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(scene.skybox.meshes[0]->indices.size()), 1, 0, 0, 0);

			commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pipelines[Pipeline::DEFAULT]);
			std::vector<vk::DescriptorSet> descriptorSetListForModel = {
						uboDescriptorSets[currentFrame],
						scene.skybox.material.descriptorSets[currentFrame],
						swapChain.descriptorSets[currentFrame]
			};
			commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
				pipelines[Pipeline::DEFAULT].GetLayout(), 0, descriptorSetListForModel, {});
			vkCmdDraw(commandBuffer, 3, 1, 0, 0);
			imgui.drawFrame(commandBuffer);
			vkCmdEndRenderPass(commandBuffer);
		}
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

			if (enableInput) {
				mouse_dx = currentX - previousX;
				mouse_dy = currentY - previousY;

				previousX = currentX;
				previousY = currentY;




				camera.Update(time / 1000.f, keyPressed, mouse_dx, mouse_dy);
			}
			drawFrame();
		}
		device->logical.waitIdle();
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


		memcpy(uniformBuffers[currentImage]->mapped, &ubo, sizeof(ubo));

		
		memcpy(GUIBuffers[currentImage]->mapped, &guiControl, sizeof(GUIControl));
	}
	void drawFrame() {
		device->logical.waitForFences({ *inFlightFences[currentFrame] }, vk::True, UINT64_MAX);

		uint32_t imageIndex;
		vk::Result result =device->logical.acquireNextImageKHR(swapChain, UINT64_MAX, *imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);
		if (result == vk::Result::eErrorOutOfDateKHR) {
			recreateSwapChain();
			return;
		}
		else if (result != vk::Result::eSuccess && result != vk::Result::eSuboptimalKHR) {
			throw std::runtime_error("failed to acquire swap chain image!");
		}
		device->logical.resetFences({ *inFlightFences[currentFrame] });
		updateUniformBuffer(currentFrame);
		commandBuffers[currentFrame].reset({});
		recordCommandBuffer(commandBuffers[currentFrame], imageIndex, scene.models);
		vk::SubmitInfo submitInfo{};
		
		vk::Semaphore waitSemaphores[] = { *imageAvailableSemaphores[currentFrame] };
		vk::PipelineStageFlags waitStages[1] = { vk::PipelineStageFlagBits::eColorAttachmentOutput};
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = waitSemaphores;
		submitInfo.pWaitDstStageMask = waitStages;


		std::vector<vk::CommandBuffer> commandBuffersToSubmit = { commandBuffers[currentFrame]};
		submitInfo.commandBufferCount = static_cast<uint32_t>(commandBuffersToSubmit.size());
		submitInfo.pCommandBuffers = commandBuffersToSubmit.data();

		vk::Semaphore signalSemaphores[1] = { *renderFinishedSemaphores[currentFrame] };
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = signalSemaphores;

		device->GetQueue(Device::QueueType::GRAPHICS).submit(submitInfo, *inFlightFences[currentFrame]);

		vk::PresentInfoKHR presentInfo{};
		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = signalSemaphores;

		vk::SwapchainKHR swapChains[1] = { swapChain };
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = swapChains;
		presentInfo.pImageIndices = &imageIndex;
		presentInfo.pResults = nullptr;

		result = device->GetQueue(Device::QueueType::PRESENT).presentKHR(presentInfo);
		if (result == vk::Result::eErrorOutOfDateKHR|| result == vk::Result::eSuboptimalKHR|| framebufferResized) {
			framebufferResized = false;
			recreateSwapChain();
		}
		else if (result != vk::Result::eSuccess) {
			throw std::runtime_error("failed to present swap chain image!");
		}
		currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
	}

	void cleanup() {
		swapChain.destroy();
		imgui.destroy();
		Material::dummy.~ImageSet();
		Sampler::destroySamplers(*device);
		CommandPool::TransientPool.~CommandPool();







		for (auto& descriptorSetLayout : descriptorSetLayouts) {
			descriptorSetLayout.destroy(*device);
		}

		for (int i = 0; i < 3;i++) {
			auto& pipeline = pipelines[i];
			pipeline.destroy(*device);
		}
		rt.destroy();

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