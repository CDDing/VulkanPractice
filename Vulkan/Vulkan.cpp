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
    RenderPass renderPass;
    CommandPool commandPool;
    Surface surface;
    std::vector<CommandBuffer> commandBuffers;
    Image depthImage;
    ImageView depthImageView;
    std::vector<Buffer> uniformBuffers;
    std::vector<void*> uniformBuffersMapped;
    std::vector<Model> models;

    DescriptorPool descriptorPool;
    DescriptorSetLayout descriptorSetLayout;
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
        surface = Surface(instance,window);
        device = Device(instance, surface);
        swapChain = SwapChain(device, surface);
        renderPass = RenderPass(device,swapChain.GetImageFormat(), findDepthFormat());
        descriptorSetLayout = DescriptorSetLayout(device);
        descriptorPool = DescriptorPool(device);
        commandPool = CommandPool(device, surface);
        InsertModels();
        loadCubeMap();//TODO
        createDepthResources();
        createFramebuffers();
        createUniformBuffers();
        createDescriptorSets();
        createPipelines();
        for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            CommandBuffer cb = CommandBuffer(device, commandPool);
            commandBuffers.push_back(cb);
        }
        createSyncObjects();
    }
    void createPipelines() {
        pipelines.resize(Pipeline::END);

        Pipeline defaultPipeline =Pipeline(device,
            swapChain.GetExtent(),
            descriptorSetLayout.Get(),
            renderPass,
            "shaders/shader.vert.spv",
            "shaders/shader.frag.spv");

        Pipeline skyboxPipeline = Pipeline(device,
            swapChain.GetExtent(),
            descriptorSetLayout.Get(),
            renderPass,
            "shaders/skybox.vert.spv",
            "shaders/skybox.frag.spv");

        pipelines[Pipeline::DEFAULT]=(defaultPipeline);
        pipelines[Pipeline::SKYBOX]=(skyboxPipeline);
    }
    void loadCubeMap() {
        //int width, height, channels;
        //stbi_uc* faceData[6];
        //faceData[0] = stbi_load("Resources/textures/cubemap/right.jpg", &width, &height, &channels, STBI_rgb_alpha);
        //faceData[1] = stbi_load("Resources/textures/Cubemap/left.jpg", &width, &height, &channels, STBI_rgb_alpha);
        //faceData[2] = stbi_load("Resources/textures/Cubemap/top.jpg", &width, &height, &channels, STBI_rgb_alpha);
        //faceData[3] = stbi_load("Resources/textures/Cubemap/bottom.jpg", &width, &height, &channels, STBI_rgb_alpha);
        //faceData[4] = stbi_load("Resources/textures/Cubemap/front.jpg", &width, &height, &channels, STBI_rgb_alpha);
        //faceData[5] = stbi_load("Resources/textures/Cubemap/back.jpg", &width, &height, &channels, STBI_rgb_alpha);
        //uint32_t mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(width, height)))) + 1;
        //
        //int size = width * height * channels * 6;


        //VkBuffer stagingBuffer;
        //VkDeviceMemory stagingMemory;

        //VkBufferCreateInfo bufferCreateInfo;
        //bufferCreateInfo.size = size;
        //// This buffer is used as a transfer source for the buffer copy
        //bufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        //bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        //vkCreateBuffer(device.Get(), &bufferCreateInfo, nullptr, &stagingBuffer);

        //VkMemoryRequirements memRequirements;
        //vkGetBufferMemoryRequirements(device.Get(), stagingBuffer, &memRequirements);

        //VkMemoryAllocateInfo allocInfo{};
        //allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        //allocInfo.allocationSize = memRequirements.size;
        //allocInfo.memoryTypeIndex = findMemoryType(device, memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

        //vkAllocateMemory(device.Get(), &allocInfo, nullptr, &stagingMemory);
        //vkBindBufferMemory(device.Get(), stagingBuffer, stagingMemory, 0);

        //// Copy texture data into staging buffer
        //uint8_t* data;
        //vkMapMemory(device.Get(), stagingMemory, 0, memRequirements.size, 0, (void**)&data);
        //memcpy(data, faceData, size);
        //vkUnmapMemory(device.Get(), stagingMemory);


        //VkImageCreateInfo cubeMapCreateInfo;
        //cubeMapCreateInfo.imageType = VK_IMAGE_TYPE_2D;
        //cubeMapCreateInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
        //cubeMapCreateInfo.mipLevels = mipLevels;//TODO
        //cubeMapCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        //cubeMapCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        //cubeMapCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        //cubeMapCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        //cubeMapCreateInfo.extent = { (uint32_t)width,(uint32_t)height,1 };
        //cubeMapCreateInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        //cubeMapCreateInfo.arrayLayers = 6;
        //cubeMapCreateInfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;

        //if (vkCreateImage(device.Get(), &cubeMapCreateInfo, nullptr, &cubeMap.image.Get())!=VK_SUCCESS) {
        //    throw std::runtime_error("failed to load cubemap texture!");
        //}


        //std::vector<VkBufferImageCopy> bufferCopyRegions;
        //uint32_t offset = 0; // 시작 오프셋
        //for (uint32_t face = 0; face < 6; face++) {
        //    uint32_t mipWidth = width;
        //    uint32_t mipHeight = height;

        //    for (uint32_t level = 0; level < mipLevels; level++) {
        //        VkBufferImageCopy bufferCopyRegion{};
        //        bufferCopyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        //        bufferCopyRegion.imageSubresource.mipLevel = level;
        //        bufferCopyRegion.imageSubresource.baseArrayLayer = face;
        //        bufferCopyRegion.imageSubresource.layerCount = 1;

        //        bufferCopyRegion.imageExtent.width = std::max(1u, mipWidth);
        //        bufferCopyRegion.imageExtent.height = std::max(1u, mipHeight);
        //        bufferCopyRegion.imageExtent.depth = 1;
        //        bufferCopyRegion.bufferOffset = offset;

        //        bufferCopyRegions.push_back(bufferCopyRegion);

        //        // 현재 Mip Level의 데이터 크기 계산 (최소 크기 1x1 보장)
        //        uint32_t mipSize = std::max(1u, mipWidth) * std::max(1u, mipHeight) * channels;

        //        // 다음 Mip Level 크기 준비
        //        mipWidth = mipWidth > 1 ? mipWidth >> 1 : 1;
        //        mipHeight = mipHeight > 1 ? mipHeight >> 1 : 1;

        //        // 오프셋 증가
        //        offset += mipSize;
        //    }
        //}
        //VkCommandBuffer copyCmd = beginSingleTimeCommands(device);
        //VkImageSubresourceRange subresourceRange = {};
        //subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        //subresourceRange.baseMipLevel = 0;
        //subresourceRange.levelCount = mipLevels;
        //subresourceRange.layerCount = 6;

        //transitionImageLayout(device, cubeMap.image.Get(), VK_FORMAT_R8G8B8A8_UNORM,
        //    VK_IMAGE_LAYOUT_UNDEFINED,
        //    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, mipLevels, subresourceRange);

        //// Copy the cube map faces from the staging buffer to the optimal tiled image
        //vkCmdCopyBufferToImage(
        //    copyCmd,
        //    stagingBuffer,
        //    cubeMap.image.Get(),
        //    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        //    static_cast<uint32_t>(bufferCopyRegions.size()),
        //    bufferCopyRegions.data()
        //);
        //endSingleTimeCommands(device, copyCmd);

        //// Change texture image layout to shader read after all faces have been copied
        //transitionImageLayout(device, cubeMap.image.Get(), VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        //    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, mipLevels, subresourceRange);


        //// Create sampler
        //VkSamplerCreateInfo sampler;
        //sampler.magFilter = VK_FILTER_LINEAR;
        //sampler.minFilter = VK_FILTER_LINEAR;
        //sampler.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        //sampler.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        //sampler.addressModeV = sampler.addressModeU;
        //sampler.addressModeW = sampler.addressModeU;
        //sampler.mipLodBias = 0.0f;
        //sampler.compareOp = VK_COMPARE_OP_NEVER;
        //sampler.minLod = 0.0f;
        //sampler.maxLod = static_cast<float>(mipLevels);
        //sampler.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
        //sampler.maxAnisotropy = 1.0f;
        //VkPhysicalDeviceProperties properties{};
        //vkGetPhysicalDeviceProperties(device.GetPhysical(), &properties);
        //sampler.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
        //sampler.anisotropyEnable = VK_TRUE;
        //
        //vkCreateSampler(device.Get(), &sampler, nullptr, &cubeMap.sampler.Get());

        //// Create image view
        //VkImageViewCreateInfo view;
        //// Cube map view type
        //view.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
        //view.format = VK_FORMAT_R8G8B8A8_UNORM;
        //view.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
        //// 6 array layers (faces)
        //view.subresourceRange.layerCount = 6;
        //// Set number of mip levels
        //view.subresourceRange.levelCount = mipLevels;
        //view.image = cubeMap.image.Get();
        //vkCreateImageView(device.Get(), &view, nullptr, &cubeMap.imageView.Get());

        //// Clean up staging resources
        //vkFreeMemory(device.Get(), stagingMemory, nullptr);
        //vkDestroyBuffer(device.Get(), stagingBuffer, nullptr);
        //STBI_FREE(faceData);
    }
    void InsertModels() {
        Model model = makeSphere(device, 1.0f, "Resources/models/Bricks075A_1K-PNG/Bricks075A_1K-PNG_Color.png","Resources/models/Bricks075A_1K-PNG/Bricks075A_1K-PNG_NormalDX.png");
        Model model2 = Model(device, MODEL_PATH.c_str(), TEXTURE_PATH.c_str(), NORMALMAP_PATH.c_str());
        
        models.push_back(model);
        models.push_back(model2);

        
    }
    
    void createDepthResources() {
        VkFormat depthFormat = findDepthFormat();
        depthImage = Image(device,swapChain.GetExtent().width, swapChain.GetExtent().height, 1, depthFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        depthImageView = ImageView(device,depthImage.Get(), depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT, 1);

        transitionImageLayout(device, depthImage.Get(), depthFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_IMAGE_ASPECT_DEPTH_BIT, 1);
    }
    VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features) {
        for (VkFormat format : candidates) {
            VkFormatProperties props;
            vkGetPhysicalDeviceFormatProperties(device.GetPhysical(), format, &props);

            if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
                return format;
            }
            else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
                return format;
            }
        }

        throw std::runtime_error("failed to find supported format!");
    }
    VkFormat findDepthFormat() {
        return findSupportedFormat({ VK_FORMAT_D32_SFLOAT,VK_FORMAT_D32_SFLOAT_S8_UINT,VK_FORMAT_D24_UNORM_S8_UINT }, VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
    }
    
    void createDescriptorSets() {
        
        descriptorSets.resize(models.size());
        for (auto& descriptorSetVector : descriptorSets) {
            descriptorSetVector.resize(MAX_FRAMES_IN_FLIGHT);
            for (auto& descriptorSet : descriptorSetVector) {
                descriptorSet = DescriptorSet(device, descriptorPool, descriptorSetLayout);
            }
        }

        for (int i = 0; i < models.size();i++) {
            for (size_t j = 0; j < MAX_FRAMES_IN_FLIGHT; j++) {
                VkDescriptorBufferInfo bufferInfo{};
                bufferInfo.buffer = uniformBuffers[j].Get();
                bufferInfo.offset = 0;
                bufferInfo.range = sizeof(UniformBufferObject);

                VkDescriptorImageInfo imageInfo{};
                imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                imageInfo.imageView = models[i].images[0].imageView.Get();
                imageInfo.sampler = models[i].images[0].sampler.Get();

                VkDescriptorImageInfo normalMapInfo{};
                normalMapInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                normalMapInfo.imageView = models[i].images[1].imageView.Get();
                normalMapInfo.sampler = models[i].images[1].sampler.Get();

                std::array<VkWriteDescriptorSet, 3> descriptorWrites{};
                descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                descriptorWrites[0].dstSet = descriptorSets[i][j].Get();
                descriptorWrites[0].dstBinding = 0;
                descriptorWrites[0].dstArrayElement = 0;
                descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                descriptorWrites[0].descriptorCount = 1;
                descriptorWrites[0].pBufferInfo = &bufferInfo;

                descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                descriptorWrites[1].dstSet = descriptorSets[i][j].Get();
                descriptorWrites[1].dstBinding = 1;
                descriptorWrites[1].dstArrayElement = 0;
                descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                descriptorWrites[1].descriptorCount = 1;
                descriptorWrites[1].pImageInfo = &imageInfo;

                descriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                descriptorWrites[2].dstSet = descriptorSets[i][j].Get();
                descriptorWrites[2].dstBinding = 2;
                descriptorWrites[2].dstArrayElement = 0;
                descriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                descriptorWrites[2].descriptorCount = 1;
                descriptorWrites[2].pImageInfo = &normalMapInfo;


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
    
    
    void cleanupSwapChain() {
        vkDestroyImageView(device.Get(), depthImageView.Get(), nullptr);
        vkDestroyImage(device.Get(), depthImage.Get(), nullptr);
        vkFreeMemory(device.Get(), depthImage.GetMemory(), nullptr);
        for (size_t i = 0; i < swapChain.GetFrameBuffers().size(); i++) {
            vkDestroyFramebuffer(device.Get(), swapChain.GetFrameBuffers()[i], nullptr);
        }

        for (size_t i = 0; i < swapChain.GetImageViews().size(); i++) {
            vkDestroyImageView(device.Get(), swapChain.GetImageViews()[i].Get(), nullptr);
        }
        vkDestroySwapchainKHR(device.Get(), swapChain.Get(), nullptr);
    }
    void recreateSwapChain() {
        int width = 0, height = 0;
        glfwGetFramebufferSize(window, &width, &height);
        while (width == 0 || height == 0) {
            glfwGetFramebufferSize(window, &width, &height);
            glfwWaitEvents();
        }
        vkDeviceWaitIdle(device.Get());
        cleanupSwapChain();
        swapChain.create();
        createDepthResources();
        createFramebuffers();
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
        renderPassInfo.renderPass = renderPass.Get();
        renderPassInfo.framebuffer = swapChain.GetFrameBuffers()[imageIndex];
        renderPassInfo.renderArea.offset = { 0,0 };
        renderPassInfo.renderArea.extent = swapChain.GetExtent();

        std::array<VkClearValue, 2> clearValues{};
        clearValues[0].color = { {0.0f, 0.0f, 0.0f, 1.0f} };
        clearValues[1].depthStencil = { 1.0f, 0 };
        renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
        renderPassInfo.pClearValues = clearValues.data();

        vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines[Pipeline::SKYBOX].Get());

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

        for (int i = 0; i < models.size();i++) {
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
    void createFramebuffers() {
        swapChain.GetFrameBuffers().resize(swapChain.GetImageViews().size());
        for (size_t i = 0; i < swapChain.GetImageViews().size(); i++) {
            std::array<VkImageView,2> attachments = {
                swapChain.GetImageViews()[i].Get(),
                depthImageView.Get()
            };
            VkFramebufferCreateInfo framebufferInfo{};
            framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebufferInfo.renderPass = renderPass.Get();
            framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
            framebufferInfo.pAttachments = attachments.data();
            framebufferInfo.width = swapChain.GetExtent().width;
            framebufferInfo.height = swapChain.GetExtent().height;
            framebufferInfo.layers = 1;

            if (vkCreateFramebuffer(device.Get(), &framebufferInfo, nullptr, &swapChain.GetFrameBuffers()[i]) != VK_SUCCESS) {
                throw std::runtime_error("failed to create framebuffer!");
            }
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




                camera.Update(time / 1000.f, keyPressed,mouse_dx,mouse_dy);
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
        recordCommandBuffer(commandBuffers[currentFrame].Get(), imageIndex,models);
        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

        VkSemaphore waitSemaphores[] = { imageAvailableSemaphores[currentFrame]};
        VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = waitSemaphores;
        submitInfo.pWaitDstStageMask = waitStages;

        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffers[currentFrame].Get();

        VkSemaphore signalSemaphores[] = { renderFinishedSemaphores[currentFrame]};
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = signalSemaphores;

        if (vkQueueSubmit(device.GetQueue(Device::QueueType::GRAPHICS), 1, &submitInfo,inFlightFences[currentFrame]) != VK_SUCCESS) {
            throw std::runtime_error("failed to submit draw command buffer!");
        }

        VkPresentInfoKHR presentInfo{};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = signalSemaphores;

        VkSwapchainKHR swapChains[] = { swapChain.Get()};
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
        cleanupSwapChain();

        vkDestroyDescriptorPool(device.Get(), descriptorPool.Get(), nullptr);
        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            vkDestroyBuffer(device.Get(), uniformBuffers[i].Get(), nullptr);
            vkFreeMemory(device.Get(), uniformBuffers[i].GetMemory(), nullptr);
        }
        vkDestroyDescriptorSetLayout(device.Get(), descriptorSetLayout.Get(), nullptr);

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
        vkDestroyRenderPass(device.Get(), renderPass.Get(), nullptr);
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