#include "pch.h"
vk::raii::CommandPool createCommandPool(Device& device, QueueFamilyIndices queueFamilyIndices) {

	//QueueFamilyIndices queueFamilyIndices = findQueueFamilies(device, surface);
	vk::CommandPoolCreateInfo poolInfo{ vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
	queueFamilyIndices.graphicsFamily.value() };

	return device.logical.createCommandPool(poolInfo);
}
Scene createScene(Device& device) {
	Scene scene(nullptr);
	scene.models.push_back(Model(device, {},
		BaseModel::Square,
		{},
		glm::translate(glm::mat4(30.0f), glm::vec3(0, -2.f / 30.f, 0))));
	scene.models.push_back(Model(device, { MaterialComponent::ALBEDO, MaterialComponent::NORMAL, MaterialComponent::ROUGHNESS, MaterialComponent::ao },
		"Resources/models/vk2vcdl/vk2vcdl.fbx",
		{ "Resources/models/vk2vcdl/vk2vcdl_4K_BaseColor.jpg",
		"Resources/models/vk2vcdl/vk2vcdl_4K_Normal.jpg",
		"Resources/models/vk2vcdl/vk2vcdl_4K_Roughness.jpg",
		"Resources/models/vk2vcdl/vk2vcdl_4K_AO.jpg" },
		glm::rotate(glm::translate(glm::mat4(1.0f), glm::vec3(1.5, 0, 2)), glm::radians(45.0f), glm::vec3(-2, 3, 1))));
	scene.models.push_back(Model(device, {},
		BaseModel::Sphere,
		{},
		glm::scale(glm::translate(glm::mat4(1.0f), glm::vec3(0, 0, 1)), glm::vec3(0.4f))));
	scene.skybox = Skybox(device);
	return scene;
}

template<typename T>
std::vector<DBuffer> createDBuffer(Device& device) {
	std::vector<DBuffer> result;

	VkDeviceSize bufferSize = sizeof(T);

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		result.push_back(DBuffer(device, bufferSize, vk::BufferUsageFlagBits::eUniformBuffer, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent));
		result[i].map(device, bufferSize, 0);

	}
	return result;
}
std::vector<vk::raii::Semaphore> createSemaphores(Device& device) {
	std::vector<vk::raii::Semaphore> result;
	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		vk::SemaphoreCreateInfo semaphoreInfo{};
		result.push_back(device.logical.createSemaphore(semaphoreInfo));
	}
	return result;
}
std::vector<vk::raii::Fence> createFences(Device& device) {
	std::vector<vk::raii::Fence> result;
	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		vk::FenceCreateInfo fenceInfo{};
		fenceInfo.flags = vk::FenceCreateFlagBits::eSignaled;
		result.push_back(device.logical.createFence(fenceInfo));
	}
	return result;
}
template<typename T>
std::vector<vk::raii::DescriptorSet> createSets(Device& device, std::vector<DBuffer>& buffers) {
	std::vector<vk::raii::DescriptorSet> result;
	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		vk::DescriptorSetAllocateInfo allocInfo{ DescriptorPool::Pool ,*DescriptorSetLayout::Get(DescriptorType::VP) };
		result.push_back(std::move(vk::raii::DescriptorSets(device, allocInfo).front()));

		vk::DescriptorBufferInfo bufferInfo;
		bufferInfo.buffer = buffers[i].buffer;
		bufferInfo.offset = 0;
		bufferInfo.range = sizeof(T);

		vk::WriteDescriptorSet descriptorWrite{};
		descriptorWrite.dstSet = result[i];
		descriptorWrite.dstBinding = 0;
		descriptorWrite.dstArrayElement = 0;
		descriptorWrite.descriptorType = vk::DescriptorType::eUniformBuffer;
		descriptorWrite.descriptorCount = 1;
		descriptorWrite.pBufferInfo = &bufferInfo;

		device.logical.updateDescriptorSets(descriptorWrite, nullptr);

	}

	return result;
}
std::vector<Pipeline> createPipelines(Device& device,SwapChain& swapChain, Deferred& deferred) {
	std::vector<Pipeline> result;

	std::vector<std::vector<vk::DescriptorSetLayout>> descriptorSetLayoutList;
	descriptorSetLayoutList = {
		//기본 셰이더
		{
			*DescriptorSetLayout::Get(0),
			*DescriptorSetLayout::Get(1),
			*DescriptorSetLayout::Get(4),
			},
			//스카이박스 셰이더
			{
				*DescriptorSetLayout::Get(0),
			*DescriptorSetLayout::Get(1)
			},
		//디퍼드 셰이더
			{
				*DescriptorSetLayout::Get(0),
				*DescriptorSetLayout::Get(2),
			*DescriptorSetLayout::Get(3),
			*DescriptorSetLayout::Get(0)}
			};

	result.push_back(Pipeline(device,
		swapChain.extent,
		descriptorSetLayoutList,
		swapChain.renderPass,
		"shaders/shader.vert.spv",
		"shaders/shader.frag.spv",
		ShaderType::DEFAULT));

	result.push_back(Pipeline(device,
		swapChain.extent,
		descriptorSetLayoutList,
		swapChain.renderPass,
		"shaders/skybox.vert.spv",
		"shaders/skybox.frag.spv",
		ShaderType::SKYBOX));

	result.push_back(Pipeline(device,
		swapChain.extent,
		descriptorSetLayoutList,
		deferred.renderPass,
		"shaders/deferred.vert.spv",
		"shaders/deferred.frag.spv",
		ShaderType::DEFERRED));

	return result;
}
GLFWwindow* initWindow() {
	glfwInit();

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

	GLFWwindow* w = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
	return w;
}
class VulkanApp {
public:
	VulkanApp() :
		window(initWindow()),
		instance("DDing"), 
		surface(createSurface(instance, window)), 
		device(instance, surface),
		swapChain(device, surface), 
		deferred(device, swapChain), 
		pp(device, swapChain),
		commandPool(createCommandPool(device, findQueueFamilies(device, surface))),
		imgui(device, window, instance, swapChain.renderPass),
		scene(std::move(createScene(device))),
		uniformBuffers(std::move(createDBuffer<UniformBufferObject>(device))),
		GUIBuffers(std::move(createDBuffer<GUIControl>(device))),
		uboDescriptorSets(createSets<UniformBufferObject>(device,uniformBuffers)),
		GUIDescriptorSets(createSets<GUIControl>(device,GUIBuffers)),
		pipelines(createPipelines(device, swapChain, deferred)),
		//rt(device, swapChain, scene, uniformBuffers, GUIBuffers),
		//scene(nullptr),
		rt(nullptr),

		imageAvailableSemaphores(createSemaphores(device)),
		renderFinishedSemaphores(createSemaphores(device)),
		inFlightFences(createFences(device))

	{

		glfwSetWindowUserPointer(window, this);
		glfwSetCursorPosCallback(window, mouseInput);
		glfwSetKeyCallback(window, keyboardInput);
		glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
		for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
			vk::CommandBufferAllocateInfo allocInfo{ commandPool,vk::CommandBufferLevel::ePrimary,1 };
			commandBuffers.push_back(std::move(vk::raii::CommandBuffers(device, allocInfo).front()));
		}
	}
	void run() {
		mainLoop();
		cleanup();
	}
private:
	GLFWwindow* window;
	Instance instance;
	vk::raii::SurfaceKHR surface;
	Device device;
	SwapChain swapChain;
	Deferred deferred;
	PostProcessing pp;
	vk::raii::CommandPool commandPool;

	GUI imgui;
	Scene scene;
	std::vector<DBuffer> uniformBuffers;
	std::vector<DBuffer> GUIBuffers;
	std::vector<vk::raii::DescriptorSet> uboDescriptorSets;
	std::vector<vk::raii::DescriptorSet> GUIDescriptorSets;
	std::vector<Pipeline> pipelines;
	RayTracing rt;
	std::vector<vk::raii::Semaphore> imageAvailableSemaphores;
	std::vector<vk::raii::Semaphore> renderFinishedSemaphores;
	std::vector<vk::raii::Fence> inFlightFences;


	std::vector<vk::raii::CommandBuffer> commandBuffers;
	GUIControl guiControl{};



	Camera camera;
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
	
	

	void recreateSwapChain() {
		//TODO
		//int width = 0, height = 0;
		//glfwGetFramebufferSize(window, &width, &height);
		//while (width == 0 || height == 0) {
		//	glfwGetFramebufferSize(window, &width, &height);
		//	glfwWaitEvents();
		//}
		//device.logical.waitIdle();
		//swapChain.destroy(device);
		//swapChain.create(device);
		//deferred.updateDescriptorSets();
		//imgui.init(static_cast<float>(width), static_cast<float>(height));
		//imgui.initResources(swapChain.GetRenderPass());
	}
	void recordCommandBuffer(vk::raii::CommandBuffer& commandBuffer, uint32_t imageIndex, std::vector<Model>& models) {
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

			rt.recordCommandBuffer(device, commandBuffer, currentFrame, imageIndex);
			vk::RenderPassBeginInfo renderPassInfo{};
			renderPassInfo.setRenderPass(pp.renderPass);
			renderPassInfo.framebuffer = pp.framebuffers[imageIndex];
			renderPassInfo.renderArea.offset = vk::Offset2D(0,0);
			renderPassInfo.renderArea.extent = swapChain.extent;

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
			deferredRenderPassInfo.setRenderPass(deferred.renderPass);
			deferredRenderPassInfo.framebuffer = deferred.framebuffers[imageIndex];
			deferredRenderPassInfo.renderArea.offset = vk::Offset2D( 0,0 );
			deferredRenderPassInfo.renderArea.extent = swapChain.extent;

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
			viewport.width = static_cast<float>(swapChain.extent.width);
			viewport.height = static_cast<float>(swapChain.extent.height);
			viewport.minDepth = 0.0f;
			viewport.maxDepth = 1.0f;
			commandBuffer.setViewport(0,viewport);

			vk::Rect2D scissor{};
			scissor.offset = vk::Offset2D( 0,0 );
			scissor.extent = swapChain.extent;
			
			commandBuffer.setScissor(0, scissor);
			commandBuffer.beginRenderPass(deferredRenderPassInfo, vk::SubpassContents::eInline);

			//GBuffer Draw
			commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pipelines[Pipeline::DEFERRED]);

			for (auto& model : models) {
				for (auto& mesh : model.meshes) {
					vk::Buffer vertexBuffers[] = { mesh.vertexBuffer.buffer};
					vk::DeviceSize offsets[] = { 0 };
					commandBuffer.bindVertexBuffers(0, vertexBuffers, offsets);
					commandBuffer.bindIndexBuffer(mesh.indexBuffer.buffer,0,vk::IndexType::eUint32);
					
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

					commandBuffer.drawIndexed(static_cast<uint32_t>(mesh.indices.size()), 1, 0, 0, 0);

				}
			}

			commandBuffer.endRenderPass();

			vk::RenderPassBeginInfo renderPassInfo{};
			renderPassInfo.setRenderPass(swapChain.renderPass);
			renderPassInfo.framebuffer = swapChain.framebuffers[imageIndex];
			renderPassInfo.renderArea.offset = vk::Offset2D{ 0,0 };
			renderPassInfo.renderArea.extent = swapChain.extent;

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

			vk::Buffer vertexBuffers[] = { scene.skybox.meshes[0].vertexBuffer.buffer };
			vk::DeviceSize offsets[] = { 0 };
			
			
			commandBuffer.bindVertexBuffers(0, vertexBuffers, offsets);
			commandBuffer.bindIndexBuffer(scene.skybox.meshes[0].indexBuffer.buffer, 0, vk::IndexType::eUint32);

			commandBuffer.drawIndexed(static_cast<uint32_t>(scene.skybox.meshes[0].indices.size()), 1, 0, 0, 0);

			commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pipelines[Pipeline::DEFAULT]);
			std::vector<vk::DescriptorSet> descriptorSetListForModel = {
						uboDescriptorSets[currentFrame],
						scene.skybox.material.descriptorSets[currentFrame],
						deferred.descriptorSets[currentFrame]
			};
			commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
				pipelines[Pipeline::DEFAULT].GetLayout(), 0, descriptorSetListForModel, {});

			commandBuffer.draw(3, 1, 0, 0);
			imgui.drawFrame(commandBuffer);
			commandBuffer.endRenderPass();
		}

		commandBuffer.end();

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
		device.logical.waitIdle();
	}
	void updateUniformBuffer(uint32_t currentImage) {
		UniformBufferObject ubo{};
		ubo.view = camera.GetView();
		ubo.proj = camera.GetProj(swapChain.extent.width, swapChain.extent.height);
		ubo.proj[1][1] *= -1;
		ubo.camPos = camera.GetPos();

		ubo.lights[0] = glm::vec4(0, 1, 0, 0);
		ubo.lights[1] = glm::vec4(1, 1, 1, 0);
		ubo.lights[2] = glm::vec4(1, 1, 1, 0);
		ubo.lights[3] = glm::vec4(1, 1, 1, 0);


		memcpy(uniformBuffers[currentImage].mapped, &ubo, sizeof(ubo));

		
		memcpy(GUIBuffers[currentImage].mapped, &guiControl, sizeof(GUIControl));
	}
	void drawFrame() {
		device.logical.waitForFences({ *inFlightFences[currentFrame] }, vk::True, UINT64_MAX);

		auto acquireImage = swapChain.Get().acquireNextImage(UINT64_MAX, *imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE);
		uint32_t imageIndex = acquireImage.second;
		vk::Result result = acquireImage.first;
		if (result == vk::Result::eErrorOutOfDateKHR) {
			recreateSwapChain();
			return;
		}
		else if (result != vk::Result::eSuccess && result != vk::Result::eSuboptimalKHR) {
			throw std::runtime_error("failed to acquire swap chain image!");
		}
		device.logical.resetFences({ *inFlightFences[currentFrame] });
		updateUniformBuffer(currentFrame);
		commandBuffers[currentFrame].reset({});
		recordCommandBuffer(commandBuffers[currentFrame], imageIndex, scene.models);
		vk::SubmitInfo submitInfo{};
		
		vk::Semaphore waitSemaphores[] = { *imageAvailableSemaphores[currentFrame] };
		vk::PipelineStageFlags waitStages[1] = { vk::PipelineStageFlagBits::eColorAttachmentOutput};
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = waitSemaphores;
		submitInfo.pWaitDstStageMask = waitStages;

		std::vector<vk::CommandBuffer> commandBuffersToSubmit = { *commandBuffers[currentFrame]};
		submitInfo.commandBufferCount = static_cast<uint32_t>(commandBuffersToSubmit.size());
		submitInfo.pCommandBuffers = commandBuffersToSubmit.data();

		vk::Semaphore signalSemaphores[1] = { *renderFinishedSemaphores[currentFrame] };
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = signalSemaphores;

		device.GetQueue(Device::QueueType::GRAPHICS).submit(submitInfo, *inFlightFences[currentFrame]);

		vk::PresentInfoKHR presentInfo{};
		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = signalSemaphores;

		vk::SwapchainKHR swapChains[1] = { swapChain.Get()};
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = swapChains;
		presentInfo.pImageIndices = &imageIndex;
		presentInfo.pResults = nullptr;

		result = device.GetQueue(Device::QueueType::PRESENT).presentKHR(presentInfo);
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
		//swapChain.destroy(device);


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