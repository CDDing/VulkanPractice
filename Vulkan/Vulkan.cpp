#include "pch.h"
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>
namespace std {
    template<> struct hash<Vertex> {
        size_t operator()(Vertex const& vertex) const {
            return ((hash<glm::vec3>()(vertex.pos) ^
                (hash<glm::vec3>()(vertex.color) << 1)) >> 1) ^
                (hash<glm::vec2>()(vertex.texCoord) << 1);
        }
    };
}
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
    Queue queue;
    SwapChain swapChain;
    RenderPass renderPass;
    CommandPool commandPool;
    Surface surface;
    std::vector<CommandBuffer> commandBuffers;
    Texture texture;
    Image depthImage;
    ImageView depthImageView;
    Buffer vertexBuffer;
    Buffer indexBuffer;
    std::vector<Buffer> uniformBuffers;
    std::vector<void*> uniformBuffersMapped;

    DescriptorPool descriptorPool;
    DescriptorSetLayout descriptorSetLayout;
    std::vector<DescriptorSet> descriptorSets;
    
    Pipeline graphicsPipeline;

    Camera camera;
    GLFWwindow* window;
    Instance instance;
    std::vector<VkSemaphore> imageAvailableSemaphores;
    std::vector<VkSemaphore> renderFinishedSemaphores;
    std::vector<VkFence> inFlightFences;
    uint32_t currentFrame = 0;
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    uint32_t mipLevels;

    bool keyPressed[256] = { false, };

    bool framebufferResized = false;

    static void keyboardInput(GLFWwindow* window, int key, int scancode, int action, int mods) {
        auto app = reinterpret_cast<VulkanApp*> (glfwGetWindowUserPointer(window));
        
        if (action == GLFW_PRESS) {
            app->keyPressed[key] = true;
        }

        if (action == GLFW_RELEASE) {
            app->keyPressed[key] = false;
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
        glfwSetKeyCallback(window, keyboardInput);
        glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
    }
    void initVulkan() {
        instance = Instance("DDing");
        surface = Surface(instance,window);
        device = Device(instance, surface);
        queue = Queue(device);
        swapChain = SwapChain(device, surface);
        renderPass = RenderPass(device,swapChain.GetImageFormat(), findDepthFormat());
        descriptorSetLayout = DescriptorSetLayout(device);
        descriptorPool = DescriptorPool(device);
        commandPool = CommandPool(device, surface);
        createDepthResources();
        createFramebuffers();
        createTexture();
        loadModel();
        createVertexBuffer();
        createIndexBuffer();
        createUniformBuffers();
        createDescriptorSets();
        graphicsPipeline = Pipeline(device, swapChain.GetExtent(), descriptorSetLayout.Get(), renderPass);
        for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            CommandBuffer cb = CommandBuffer(&device, &commandPool);
            commandBuffers.push_back(cb);
        }
        createSyncObjects();
    }
    void createTexture() {
        createTextureImage();
        texture.imageView = ImageView(device, texture.image.Get(), VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT, mipLevels);
        texture.sampler = Sampler(device, mipLevels);
    };
    void loadModel() {
        tinyobj::attrib_t attrib;
        std::vector<tinyobj::shape_t> shapes;
        std::vector<tinyobj::material_t> materials;
        std::string warn, err;

        if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, MODEL_PATH.c_str())) {
            throw std::runtime_error(warn + err);
        }

        std::unordered_map<Vertex, uint32_t> uniqueVertices{};
        for (const auto& shape : shapes) {
            for (const auto& index : shape.mesh.indices) {
                Vertex vertex{};

                vertex.pos = {
                    attrib.vertices[3 * index.vertex_index + 0],
                    attrib.vertices[3 * index.vertex_index + 1],
                    attrib.vertices[3 * index.vertex_index + 2]
                };

                vertex.texCoord = {
                    attrib.texcoords[2 * index.texcoord_index + 0],
                    1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
                };

                vertex.color = { 1.0f, 1.0f, 1.0f };

                if (uniqueVertices.count(vertex) == 0) {
                    uniqueVertices[vertex] = static_cast<uint32_t>(vertices.size());
                    vertices.push_back(vertex);
                }
                indices.push_back(uniqueVertices[vertex]);
            }
        }
    }
    void createDepthResources() {
        VkFormat depthFormat = findDepthFormat();
        depthImage = Image(device,swapChain.GetExtent().width, swapChain.GetExtent().height, 1, depthFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        depthImageView = ImageView(device,depthImage.Get(), depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT, 1);

        transitionImageLayout(depthImage.Get(), depthFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_IMAGE_ASPECT_DEPTH_BIT, 1);
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
    void createTextureImage() {
        int texWidth, texHeight, texChannels;
        stbi_uc* pixels = stbi_load(TEXTURE_PATH.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
        VkDeviceSize imageSize = texWidth * texHeight * 4;
        mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(texWidth, texHeight)))) + 1;
        if (!pixels) {
            throw std::runtime_error("failed to load texture image!");
        }

        Buffer stagingBuffer;
        
        stagingBuffer = Buffer(device, imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

        void* data;
        vkMapMemory(device.Get(), stagingBuffer.GetMemory(), 0, imageSize, 0, &data);
        memcpy(data, pixels, static_cast<size_t>(imageSize));
        vkUnmapMemory(device.Get(), stagingBuffer.GetMemory());

        stbi_image_free(pixels);
        texture.image = Image(device,texWidth, texHeight,mipLevels, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_SRC_BIT|VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
     
        transitionImageLayout(texture.image.Get(), VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT, mipLevels);
        copyBufferToImage(stagingBuffer.Get(), texture.image.Get(), static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));
        //transitionImageLayout(textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT, mipLevels);
        
        vkDestroyBuffer(device.Get(), stagingBuffer.Get(), nullptr);
        vkFreeMemory(device.Get(), stagingBuffer.GetMemory(), nullptr);
        
        generateMipmaps(texture.image.Get(), VK_FORMAT_R8G8B8A8_SRGB, texWidth, texHeight, mipLevels);

    }
    void generateMipmaps(VkImage image, VkFormat imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels) {

        VkFormatProperties formatProperties;
        vkGetPhysicalDeviceFormatProperties(device.GetPhysical(), imageFormat, &formatProperties);
        if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT)) {
            throw std::runtime_error("texture image format does not linear blitting!");
        }
        VkCommandBuffer commandBuffer = beginSingleTimeCommands(device.Get(), commandPool.Get());

        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.image = image;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;
        barrier.subresourceRange.levelCount = 1;


        int32_t mipWidth = texWidth;
        int32_t mipHeight = texHeight;

        for (uint32_t i = 1; i < mipLevels; i++) {
            barrier.subresourceRange.baseMipLevel = i - 1;
            barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

            vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

            VkImageBlit blit{};
            blit.srcOffsets[0] = { 0,0,0 };
            blit.srcOffsets[1] = { mipWidth,mipHeight,1 };
            blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            blit.srcSubresource.mipLevel = i - 1;
            blit.srcSubresource.baseArrayLayer = 0;
            blit.srcSubresource.layerCount = 1;
            blit.dstOffsets[0] = { 0,0,0 };
            blit.dstOffsets[1] = { mipWidth > 1 ? mipWidth / 2 : 1,mipHeight > 1 ? mipHeight / 2 : 1,1 };
            blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            blit.dstSubresource.mipLevel = i;
            blit.dstSubresource.baseArrayLayer = 0;
            blit.dstSubresource.layerCount = 1;

            vkCmdBlitImage(commandBuffer, image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit, VK_FILTER_LINEAR);

            barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

            vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

            if (mipWidth > 1) mipWidth /= 2;
            if (mipHeight > 1)mipHeight /= 2;
        }

        barrier.subresourceRange.baseMipLevel = mipLevels - 1;
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);
        endSingleTimeCommands(device.Get(),commandPool.Get(),queue.Get(QueueType::GRAPHICS),commandBuffer);
    }
    
    void createDescriptorSets() {
        
        descriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
        for (auto& descriptorSet : descriptorSets) {
            descriptorSet = DescriptorSet(device, descriptorPool, descriptorSetLayout);
        }
        
        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            VkDescriptorBufferInfo bufferInfo{};
            bufferInfo.buffer = uniformBuffers[i].Get();
            bufferInfo.offset = 0;
            bufferInfo.range = sizeof(UniformBufferObject);

            VkDescriptorImageInfo imageInfo{};
            imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            imageInfo.imageView = texture.imageView.Get();
            imageInfo.sampler = texture.sampler.Get();

            std::array<VkWriteDescriptorSet,2> descriptorWrites{};
            descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[0].dstSet = descriptorSets[i].Get();
            descriptorWrites[0].dstBinding = 0;
            descriptorWrites[0].dstArrayElement = 0;
            descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            descriptorWrites[0].descriptorCount = 1;
            descriptorWrites[0].pBufferInfo = &bufferInfo;

            descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[1].dstSet = descriptorSets[i].Get();
            descriptorWrites[1].dstBinding = 1;
            descriptorWrites[1].dstArrayElement = 0;
            descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            descriptorWrites[1].descriptorCount = 1;
            descriptorWrites[1].pImageInfo = &imageInfo;


            vkUpdateDescriptorSets(device.Get(), static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
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
    void createIndexBuffer() {
        VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size();

        Buffer stagingBuffer;
        stagingBuffer = Buffer(device, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

        void* data;
        vkMapMemory(device.Get(), stagingBuffer.GetMemory(), 0, bufferSize, 0, &data);
        memcpy(data, indices.data(), (size_t)bufferSize);
        vkUnmapMemory(device.Get(), stagingBuffer.GetMemory());

        indexBuffer = Buffer(device, bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

        copyBuffer(stagingBuffer.Get(), indexBuffer.Get(), bufferSize);

        vkDestroyBuffer(device.Get(), stagingBuffer.Get(), nullptr);
        vkFreeMemory(device.Get(), stagingBuffer.GetMemory(), nullptr);
    }
    void createVertexBuffer() {
        VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();
        vertexBuffer = Buffer(device, bufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

        Buffer stagingBuffer;
        stagingBuffer = Buffer(device, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);



        void* data;
        vkMapMemory(device.Get(), stagingBuffer.GetMemory(), 0, bufferSize, 0, &data);
        memcpy(data, vertices.data(), (size_t)bufferSize);
        vkUnmapMemory(device.Get(), stagingBuffer.GetMemory());

        copyBuffer(stagingBuffer.Get(), vertexBuffer.Get(), bufferSize);

        vkDestroyBuffer(device.Get(), stagingBuffer.Get(), nullptr);
        vkFreeMemory(device.Get(), stagingBuffer.GetMemory(), nullptr);
    }
    void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) {
        VkCommandBuffer commandBuffer = beginSingleTimeCommands(device.Get(), commandPool.Get());

        VkBufferCopy copyRegion{};
        copyRegion.size = size;
        vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);
        
        endSingleTimeCommands(device.Get(), commandPool.Get(), queue.Get(QueueType::GRAPHICS), commandBuffer);

    }
    void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, VkImageAspectFlags aspectFlags,uint32_t mipLevels) {
        VkCommandBuffer commandBuffer = beginSingleTimeCommands(device.Get(),commandPool.Get());
        
        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = oldLayout;
        barrier.newLayout = newLayout;

        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = image;
        barrier.subresourceRange.aspectMask = aspectFlags;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;
        barrier.subresourceRange.levelCount = mipLevels;
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = 0;

        VkPipelineStageFlags sourceStage;
        VkPipelineStageFlags destinationStage;

        if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
            barrier.srcAccessMask = 0;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

            sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        }
        else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

            sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
            destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        }
        else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
            barrier.srcAccessMask = 0;
            barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
            
            sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        }
        else {
            throw std::invalid_argument("unsupported layout transition!");
        }

        vkCmdPipelineBarrier(commandBuffer, sourceStage,destinationStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);

        endSingleTimeCommands(device.Get(), commandPool.Get(), queue.Get(QueueType::GRAPHICS), commandBuffer);
    }
    void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height) {
        VkCommandBuffer commandBuffer = beginSingleTimeCommands(device.Get(), commandPool.Get());

        VkBufferImageCopy region{};
        region.bufferOffset = 0;
        region.bufferRowLength = 0;
        region.bufferImageHeight = 0;

        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.mipLevel = 0;
        region.imageSubresource.baseArrayLayer = 0;
        region.imageSubresource.layerCount = 1;

        region.imageOffset = { 0,0,0 };
        region.imageExtent = {
            width,height,1 };

        vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
        endSingleTimeCommands(device.Get(), commandPool.Get(), queue.Get(QueueType::GRAPHICS), commandBuffer);
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
    void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex) {
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
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline.Get());

        VkBuffer vertexBuffers[] = { vertexBuffer.Get()};
        VkDeviceSize offsets[] = { 0 };
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
        vkCmdBindIndexBuffer(commandBuffer, indexBuffer.Get(), 0, VK_INDEX_TYPE_UINT32);

        

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
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline.GetLayout(), 0, 1, &descriptorSets[currentFrame].Get(), 0, nullptr);

        vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(indices.size()), 1, 0,0, 0);
 
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
            camera.Update(time / 1000.f, keyPressed);
            drawFrame();
        }

        vkDeviceWaitIdle(device.Get());
    }
    void updateUniformBuffer(uint32_t currentImage) {

        UniformBufferObject ubo{};
        ubo.model = glm::rotate(glm::mat4(1.0f), 0.f , glm::vec3(0.0f, 0.0f, 1.0f));
        ubo.view = glm::lookAt(camera.GetPos(), camera.GetPos() + camera.GetDir(), camera.GetUp());
        ubo.proj = glm::perspective(glm::radians(45.0f), swapChain.GetExtent().width / (float)swapChain.GetExtent().height, 0.1f, 10.0f);
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
        recordCommandBuffer(commandBuffers[currentFrame].Get(), imageIndex);
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

        if (vkQueueSubmit(queue.Get(QueueType::GRAPHICS), 1, &submitInfo,inFlightFences[currentFrame]) != VK_SUCCESS) {
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

        result = vkQueuePresentKHR(queue.Get(QueueType::PRESENT), &presentInfo);
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

        vkDestroySampler(device.Get(), texture.sampler.Get(), nullptr);
        vkDestroyImageView(device.Get(), texture.imageView.Get(), nullptr);
        
        vkDestroyDescriptorPool(device.Get(), descriptorPool.Get(), nullptr);
        vkDestroyImage(device.Get(), texture.image.Get(), nullptr);
        vkFreeMemory(device.Get(), texture.image.GetMemory(), nullptr);
        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            vkDestroyBuffer(device.Get(), uniformBuffers[i].Get(), nullptr);
            vkFreeMemory(device.Get(), uniformBuffers[i].GetMemory(), nullptr);
        }
        vkDestroyDescriptorSetLayout(device.Get(), descriptorSetLayout.Get(), nullptr);
        vkDestroyBuffer(device.Get(), vertexBuffer.Get(), nullptr);
        vkFreeMemory(device.Get(), vertexBuffer.GetMemory(), nullptr);

        vkDestroyBuffer(device.Get(), indexBuffer.Get(), nullptr);
        vkFreeMemory(device.Get(), indexBuffer.GetMemory(), nullptr);
        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            vkDestroySemaphore(device.Get(), imageAvailableSemaphores[i], nullptr);
            vkDestroySemaphore(device.Get(), renderFinishedSemaphores[i], nullptr);
            vkDestroyFence(device.Get(), inFlightFences[i], nullptr);
        }
        vkDestroyCommandPool(device.Get(), commandPool.Get(), nullptr);
        vkDestroyPipeline(device.Get(), graphicsPipeline.Get(), nullptr);
        vkDestroyPipelineLayout(device.Get(), graphicsPipeline.GetLayout(), nullptr);
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