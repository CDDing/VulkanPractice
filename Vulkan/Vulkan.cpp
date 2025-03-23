#include "pch.h"
bool multithread = false;


vk::raii::CommandPool createCommandPool(DContext& context) {
	QueueFamilyIndices indices = findQueueFamilies(context.physical, context.surface);

	vk::CommandPoolCreateInfo poolInfo{ vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
	indices.graphicsFamily.value() };

	return context.logical.createCommandPool(poolInfo);
}
Scene createScene(DContext& context) {
	Scene scene(nullptr);
	scene.models.push_back(Model(context, {},
		BaseModel::Square,
		{},
		glm::translate(glm::mat4(30.0f), glm::vec3(0, -2.f / 30.f, 0))));
	scene.models.push_back(Model(context, { MaterialComponent::ALBEDO, MaterialComponent::NORMAL, MaterialComponent::ROUGHNESS, MaterialComponent::ao },
		"Resources/models/vk2vcdl/vk2vcdl.fbx",
		{ "Resources/models/vk2vcdl/vk2vcdl_4K_BaseColor.jpg",
		"Resources/models/vk2vcdl/vk2vcdl_4K_Normal.jpg",
		"Resources/models/vk2vcdl/vk2vcdl_4K_Roughness.jpg",
		"Resources/models/vk2vcdl/vk2vcdl_4K_AO.jpg" },
		glm::rotate(glm::translate(glm::mat4(1.0f), glm::vec3(1.5, 0, 2)), glm::radians(45.0f), glm::vec3(-2, 3, 1))));
	scene.models.push_back(Model(context, {},
		BaseModel::Sphere,
		{},
		glm::scale(glm::translate(glm::mat4(1.0f), glm::vec3(0, 0, 1)), glm::vec3(0.4f)))); 	
	scene.models.push_back(Model(context, {},
			"Resources/models/Exterior/exterior.obj",
			{},
			glm::mat4x4(1.f))); 
	scene.skybox = Skybox(context);
	return scene;
}

template<typename T>
std::vector<DBuffer> createDBuffer(DContext& context) {
	std::vector<DBuffer> result;

	VkDeviceSize bufferSize = sizeof(T);

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		result.push_back(DBuffer(context, bufferSize, vk::BufferUsageFlagBits::eUniformBuffer, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent));
		result[i].map(bufferSize, 0);

	}
	return result;
}
std::vector<vk::raii::Semaphore> createSemaphores(DContext& context) {
	std::vector<vk::raii::Semaphore> result;
	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		vk::SemaphoreCreateInfo semaphoreInfo{};
		result.push_back(context.logical.createSemaphore(semaphoreInfo));
	}
	return result;
}
std::vector<vk::raii::Fence> createFences(DContext& context) {
	std::vector<vk::raii::Fence> result;
	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		vk::FenceCreateInfo fenceInfo{};
		fenceInfo.flags = vk::FenceCreateFlagBits::eSignaled;
		result.push_back(context.logical.createFence(fenceInfo));
	}
	return result;
}
template<typename T>
std::vector<vk::raii::DescriptorSet> createSets(DContext& context, std::vector<DBuffer>& buffers) {
	std::vector<vk::raii::DescriptorSet> result;
	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		vk::DescriptorSetAllocateInfo allocInfo{ DescriptorPool::Pool ,*DescriptorSetLayout::Get(DescriptorType::VP) };
		result.push_back(std::move(vk::raii::DescriptorSets(context.logical, allocInfo).front()));

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

		context.logical.updateDescriptorSets(descriptorWrite, nullptr);

	}

	return result;
}
std::vector<Pipeline> createPipelines(DContext& context,SwapChain& swapChain, Deferred& deferred) {
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

	result.push_back(Pipeline(context,
		swapChain.extent,
		descriptorSetLayoutList,
		swapChain.renderPass,
		"shaders/shader.vert.spv",
		"shaders/shader.frag.spv",
		ShaderType::DEFAULT));

	result.push_back(Pipeline(context,
		swapChain.extent,
		descriptorSetLayoutList,
		swapChain.renderPass,
		"shaders/skybox.vert.spv",
		"shaders/skybox.frag.spv",
		ShaderType::SKYBOX));

	result.push_back(Pipeline(context,
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
		context(window),
		swapChain(context),
		deferred(context, swapChain),
		pp(context, swapChain),
		commandPool(createCommandPool(context)),
		imgui(context, swapChain.renderPass),
		scene(std::move(createScene(context))),
		uniformBuffers(std::move(createDBuffer<UniformBufferObject>(context))),
		GUIBuffers(std::move(createDBuffer<GUIControl>(context))),
		uboDescriptorSets(createSets<UniformBufferObject>(context, uniformBuffers)),
		GUIDescriptorSets(createSets<GUIControl>(context, GUIBuffers)),
		pipelines(createPipelines(context, swapChain, deferred)),
		rt(context, swapChain, scene, uniformBuffers, GUIBuffers),

		imageAvailableSemaphores(createSemaphores(context)),
		renderFinishedSemaphores(createSemaphores(context)),
		inFlightFences(createFences(context)),
		threadPool({std::make_shared<DThreadPool>(context,5),std::make_shared<DThreadPool>(context,5)})
	{

		glfwSetWindowUserPointer(window, this);
		glfwSetCursorPosCallback(window, mouseInput);
		glfwSetKeyCallback(window, keyboardInput);
		glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
		for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
			vk::CommandBufferAllocateInfo allocInfo{ commandPool,vk::CommandBufferLevel::ePrimary,1 };
			commandBuffers.push_back(std::move(vk::raii::CommandBuffers(context.logical, allocInfo).front()));
			
		}
		
	}
	void run() {
		mainLoop();
		cleanup();
	}
private:
	GLFWwindow* window;
	DContext context;
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

	std::vector<std::shared_ptr<DThreadPool>> threadPool;

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

			std::array<vk::ClearValue, 2> clearValues{};
			clearValues[1].depthStencil = vk::ClearDepthStencilValue(1.0f, 0);

			rt.recordCommandBuffer(context, commandBuffer, currentFrame, imageIndex);

			vk::RenderPassBeginInfo renderPassInfo{};
			renderPassInfo.setRenderPass(pp.renderPass);
			renderPassInfo.framebuffer = pp.framebuffers[imageIndex];
			renderPassInfo.renderArea.offset = vk::Offset2D(0,0);
			renderPassInfo.renderArea.extent = swapChain.extent;
			renderPassInfo.setClearValues(clearValues);

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

			std::array<vk::ClearValue, 5> deferredClearValues{};
			deferredClearValues[0].color = vk::ClearColorValue{0.0f,0.0f,0.0f,0.0f};
			deferredClearValues[1].color = vk::ClearColorValue{0.0f,0.0f,0.0f,0.0f};
			deferredClearValues[2].color = vk::ClearColorValue{0.0f,0.0f,0.0f,0.0f};
			deferredClearValues[3].color = vk::ClearColorValue{0.0f,0.0f,0.0f,0.0f};
			deferredClearValues[4].depthStencil = vk::ClearDepthStencilValue(1.0f, 0);
			deferredRenderPassInfo.setClearValues(deferredClearValues);

			vk::Viewport viewport{};
			viewport.x = 0.0f;
			viewport.y = 0.0f;
			viewport.width = static_cast<float>(swapChain.extent.width);
			viewport.height = static_cast<float>(swapChain.extent.height);
			viewport.minDepth = 0.0f;
			viewport.maxDepth = 1.0f;

			vk::Rect2D scissor{};
			scissor.offset = vk::Offset2D( 0,0 );
			scissor.extent = swapChain.extent;

			commandBuffer.setViewport(0, viewport);
			commandBuffer.setScissor(0, scissor);
			if (!multithread) {
				commandBuffer.beginRenderPass(deferredRenderPassInfo, vk::SubpassContents::eInline);

				//GBuffer Draw
				commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pipelines[Pipeline::DEFERRED]);
				for (auto& model : models) {
					for (auto& mesh : model.meshes) {
						vk::Buffer vertexBuffers[] = { mesh.vertexBuffer.buffer };
						vk::DeviceSize offsets[] = { 0 };
						commandBuffer.bindVertexBuffers(0, vertexBuffers, offsets);
						commandBuffer.bindIndexBuffer(mesh.indexBuffer.buffer, 0, vk::IndexType::eUint32);

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
			}
			else {
				auto totalMeshCnt = 0;
				for (int i = 0; i < scene.models.size(); i++) {
					totalMeshCnt += scene.models[i].meshes.size();
				}
				int meshCntPerThread = std::ceil(float(totalMeshCnt) / float(threadPool[currentFrame]->num_threads));


				commandBuffer.beginRenderPass(deferredRenderPassInfo, vk::SubpassContents::eSecondaryCommandBuffers);
				

				vk::CommandBufferInheritanceInfo inheritanceInfo{};
				inheritanceInfo.setRenderPass(deferred.renderPass);
				inheritanceInfo.setFramebuffer(deferred.framebuffers[imageIndex]);
				inheritanceInfo.setSubpass(0);
				vk::CommandBufferBeginInfo beginInfo{};
				beginInfo.setFlags(vk::CommandBufferUsageFlagBits::eRenderPassContinue);
				beginInfo.setPInheritanceInfo(&inheritanceInfo);

				threadPool[currentFrame]->begin(beginInfo);
				
				std::vector<std::future<DThread*>> futures;
				int cnt = 0;
				for (int i = 0; i < scene.models.size(); i++) {
					for (int j = 0; j < scene.models[i].meshes.size(); j++) {
						cnt++;
						if (cnt == meshCntPerThread) {
							cnt = 0;

							auto fut = threadPool[currentFrame]->EnqueueJob(
								[this](DThread& thread, auto viewport, auto scissor, int imageIndex, int startIndex, int endIndex, int someInt) {
									return this->threadRenderCode(thread, viewport, scissor, imageIndex, startIndex, endIndex, someInt);
								},
								viewport, scissor, imageIndex, i, j, meshCntPerThread
							);
							futures.push_back(std::move(fut));
						}
						else if (i == scene.models.size() - 1 && j == scene.models[i].meshes.size() - 1) {


							auto fut = threadPool[currentFrame]->EnqueueJob(
								[this](DThread& thread, auto viewport, auto scissor, int imageIndex, int startIndex, int endIndex, int someInt) {
									return this->threadRenderCode(thread, viewport, scissor, imageIndex, startIndex, endIndex, someInt);
								},
								viewport, scissor, imageIndex, i, j, cnt
							);
							futures.push_back(std::move(fut));
						}
						
					}
				}

				std::vector<vk::CommandBuffer> threadedBuffers;
				for (auto& fut : futures) {
					DThread* pDThread = std::move(fut.get());
					pDThread->buffer.end();
					threadedBuffers.push_back(*pDThread->buffer);
				}

				commandBuffer.executeCommands(threadedBuffers);
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
			renderPassInfo.setClearValues(clearValues);

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
	DThread* threadRenderCode(DThread& thread, vk::Viewport viewport, vk::Rect2D scissor,
		uint32_t imageIndex, int modelIdx, int meshIdx, int meshCnt) {
		auto& commandBuffer = thread.buffer;

		commandBuffer.setViewport(0, viewport);
		commandBuffer.setScissor(0, scissor);

		commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pipelines[Pipeline::DEFERRED]);
		
		while (meshCnt) {

			auto& mesh = scene.models[modelIdx].meshes[meshIdx];
			vk::Buffer vertexBuffers[] = { mesh.vertexBuffer.buffer };
			vk::DeviceSize offsets[] = { 0 };
			commandBuffer.bindVertexBuffers(0, vertexBuffers, offsets);
			commandBuffer.bindIndexBuffer(mesh.indexBuffer.buffer, 0, vk::IndexType::eUint32);

			int maxMaterialCnt = static_cast<int>(MaterialComponent::END);
			std::vector<VkBool32> data(5);
			for (int i = 0; i < maxMaterialCnt; i++) {
				data[i] = (scene.models[modelIdx].material.hasComponent(i));
			}
			commandBuffer.pushConstants<VkBool32>(pipelines[Pipeline::DEFERRED].GetLayout(), vk::ShaderStageFlagBits::eFragment, 0, data);


			std::vector<vk::DescriptorSet> descriptorSetListForModel = {
				*uboDescriptorSets[currentFrame],
				*scene.models[modelIdx].material.descriptorSets[currentFrame],
				*scene.models[modelIdx].descriptorSets[currentFrame],
				*GUIDescriptorSets[currentFrame]
			};
			commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
				pipelines[Pipeline::DEFERRED].GetLayout(),
				0, descriptorSetListForModel, {});

			commandBuffer.drawIndexed(static_cast<uint32_t>(mesh.indices.size()), 1, 0, 0, 0);


			meshCnt--;
			meshIdx--;
			if (meshIdx == -1) {
				modelIdx--;
				if(modelIdx>=0)	meshIdx = scene.models[modelIdx].meshes.size() - 1;
			}
		}


		return &thread;
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
		context.logical.waitIdle();

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
		//이전 프레임 대기
		context.logical.waitForFences({ *inFlightFences[currentFrame] }, vk::True, UINT64_MAX);

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
		context.logical.resetFences({ *inFlightFences[currentFrame] });
		updateUniformBuffer(currentFrame);
		commandBuffers[currentFrame].reset({});
		recordCommandBuffer(commandBuffers[currentFrame], imageIndex, scene.models);
		vk::SubmitInfo submitInfo{};
		
		vk::Semaphore waitSemaphores[] = { *imageAvailableSemaphores[currentFrame] };
		vk::PipelineStageFlags waitStages[1] = { vk::PipelineStageFlagBits::eColorAttachmentOutput};
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.setWaitSemaphores(waitSemaphores);
		submitInfo.setPWaitDstStageMask(waitStages);
		submitInfo.setCommandBuffers(*commandBuffers[currentFrame]);
		submitInfo.setSignalSemaphores(*renderFinishedSemaphores[currentFrame]);

		context.GetQueue(DContext::QueueType::GRAPHICS).submit(submitInfo, *inFlightFences[currentFrame]);

		vk::PresentInfoKHR presentInfo{};
		presentInfo.setWaitSemaphores(*renderFinishedSemaphores[currentFrame]);
		presentInfo.setSwapchains(*swapChain.Get());
		presentInfo.pImageIndices = &imageIndex;
		presentInfo.pResults = nullptr;

		result = context.GetQueue(DContext::QueueType::PRESENT).presentKHR(presentInfo);
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