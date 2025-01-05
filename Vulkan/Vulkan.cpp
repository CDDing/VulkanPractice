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
	Device device;
	SwapChain swapChain;
	CommandPool commandPool;
	Surface surface;
	std::vector<CommandBuffer> commandBuffers;
	Image depthImage;
	ImageView depthImageView;
	std::vector<Buffer> uniformBuffers;
	std::vector<void*> uniformBuffersMapped;
	std::vector<Model> models;

	DescriptorPool descriptorPool;
	std::vector<DescriptorSetLayout> descriptorSetLayouts;
	std::vector<std::vector<DescriptorSet>> descriptorSets;

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
			DescriptorSetLayout(device,ShaderType::DEFAULT),
			DescriptorSetLayout(device,ShaderType::SKYBOX),
		};
	}
	void createPipelines() {
		pipelines.resize(1);

		Pipeline defaultPipeline = Pipeline(device,
			swapChain.GetExtent(),
			descriptorSetLayouts[static_cast<int>(ShaderType::DEFAULT)].Get(),
			swapChain.GetRenderPass(),
			"shaders/shader.vert.spv",
			"shaders/shader.frag.spv");

		/*Pipeline skyboxPipeline = Pipeline(device,
			swapChain.GetExtent(),
			descriptorSetLayout.Get(),
			swapChain.GetRenderPass(),
			"shaders/skybox.vert.spv",
			"shaders/skybox.frag.spv");*/

		pipelines[Pipeline::DEFAULT] = (defaultPipeline);
		//pipelines[Pipeline::SKYBOX]=(skyboxPipeline);
	}
	void InsertModels() {
		Model model = makeSphere(device, 1.0f, "Resources/models/Bricks075A_1K-PNG/Bricks075A_1K-PNG_Color.png", "Resources/models/Bricks075A_1K-PNG/Bricks075A_1K-PNG_NormalDX.png");
		Model model2 = Model(device, MODEL_PATH.c_str(), TEXTURE_PATH.c_str(), NORMALMAP_PATH.c_str());

		models.push_back(model);
		models.push_back(model2);


	}

	void createDescriptorSets() {

		descriptorSets.resize(models.size());
		for (auto& descriptorSetVector : descriptorSets) {
			descriptorSetVector.resize(MAX_FRAMES_IN_FLIGHT);
			for (auto& descriptorSet : descriptorSetVector) {
				descriptorSet = DescriptorSet(device, descriptorPool, descriptorSetLayouts[static_cast<int>(ShaderType::DEFAULT)]);
			}
		}

		for (int i = 0; i < models.size(); i++) {
			for (size_t j = 0; j < MAX_FRAMES_IN_FLIGHT; j++) {
				std::vector<VkWriteDescriptorSet> descriptorWrites;
				std::vector<ShaderComponent> components = DescriptorSetLayout::GetComponents(ShaderType::DEFAULT);

				int imgCnt = 0;
				int bufCnt = 0;

				// bufferInfo와 imageInfo를 벡터에 저장하여 유효 범위 보장
				std::vector<VkDescriptorBufferInfo> bufferInfos(components.size());
				std::vector<VkDescriptorImageInfo> imageInfos(components.size());

				for (int binding = 0; binding < components.size(); binding++) {
					VkWriteDescriptorSet descriptorWrite{};
					switch (components[binding]) {
					case ShaderComponent::UNIFORM:
						bufferInfos[bufCnt].buffer = uniformBuffers[j].Get();
						bufferInfos[bufCnt].offset = 0;
						bufferInfos[bufCnt].range = sizeof(UniformBufferObject);

						descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
						descriptorWrite.dstSet = descriptorSets[i][j].Get();
						descriptorWrite.dstBinding = binding;
						descriptorWrite.dstArrayElement = 0;
						descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
						descriptorWrite.descriptorCount = 1;
						descriptorWrite.pBufferInfo = &bufferInfos[bufCnt++];
						descriptorWrites.push_back(descriptorWrite);
						break;

					case ShaderComponent::SAMPLER:
						imageInfos[imgCnt].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
						imageInfos[imgCnt].imageView = models[i].images[imgCnt].imageView.Get();
						imageInfos[imgCnt].sampler = models[i].images[imgCnt].sampler.Get();

						descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
						descriptorWrite.dstSet = descriptorSets[i][j].Get();
						descriptorWrite.dstBinding = binding;
						descriptorWrite.dstArrayElement = 0;
						descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
						descriptorWrite.descriptorCount = 1;
						descriptorWrite.pImageInfo = &imageInfos[imgCnt++];
						descriptorWrites.push_back(descriptorWrite);
						break;
					}
				}

				vkUpdateDescriptorSets(device.Get(), static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
			}
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

		vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
		//vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines[Pipeline::SKYBOX].Get());

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

		for (int i = 0; i < models.size(); i++) {
			for (auto& mesh : models[i].meshes) {
				vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines[Pipeline::DEFAULT].Get());

				VkBuffer vertexBuffers[] = { mesh->vertexBuffer.Get() };
				VkDeviceSize offsets[] = { 0 };
				vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
				vkCmdBindIndexBuffer(commandBuffer, mesh->indexBuffer.Get(), 0, VK_INDEX_TYPE_UINT32);



				vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines[Pipeline::DEFAULT].GetLayout(), 0, 1, &descriptorSets[i][currentFrame].Get(), 0, nullptr);

				vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(mesh->indices.size()), 1, 0, 0, 0);

			}
		}

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
		ubo.model = glm::rotate(glm::mat4(1.0f), 0.f, glm::vec3(0.0f, 0.0f, 1.0f));
		ubo.view = camera.GetView();
		ubo.proj = camera.GetProj(swapChain.GetExtent().width, swapChain.GetExtent().height);
		ubo.proj[1][1] *= -1;

		memcpy(uniformBuffersMapped[currentImage], &ubo, sizeof(ubo));
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

		vkDestroyDescriptorPool(device.Get(), descriptorPool.Get(), nullptr);
		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
			vkDestroyBuffer(device.Get(), uniformBuffers[i].Get(), nullptr);
			vkFreeMemory(device.Get(), uniformBuffers[i].GetMemory(), nullptr);
		}

		for (auto& descriptorSetLayout : descriptorSetLayouts) {
			vkDestroyDescriptorSetLayout(device.Get(), descriptorSetLayout.Get(), nullptr);
		}

		for (auto& model : models) {
			model.destroy(device);
		}

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
			vkDestroySemaphore(device.Get(), imageAvailableSemaphores[i], nullptr);
			vkDestroySemaphore(device.Get(), renderFinishedSemaphores[i], nullptr);
			vkDestroyFence(device.Get(), inFlightFences[i], nullptr);
		}
		vkDestroyCommandPool(device.Get(), commandPool.Get(), nullptr);
		vkDestroyCommandPool(device.Get(), CommandPool::TransientPool, nullptr);
		for (auto& pipeline : pipelines) {
			vkDestroyPipeline(device.Get(), pipeline.Get(), nullptr);
			vkDestroyPipelineLayout(device.Get(), pipeline.GetLayout(), nullptr);
		}
		vkDestroyRenderPass(device.Get(), swapChain.GetRenderPass().Get(), nullptr);
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