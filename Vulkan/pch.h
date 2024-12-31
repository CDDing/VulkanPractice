#pragma once
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>
#include <stdexcept>
#include <algorithm>
#include <vector>
#include <cstring>
#include <cstdlib>
#include <cstdint>
#include <limits>
#include <optional>
#include <set>
#include <fstream>
#include <array>
#include <unordered_map>
#include <chrono>
#include <memory>


#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "Utils.h"
#include "Struct.h"
#include "Surface.h"
#include "Device.h"
#include "Queue.h"
#include "CommandPool.h"
#include "CommandBuffer.h"
#include "VL.h"
#include "RenderPass.h"
#include "Sampler.h"
#include "SwapChain.h"
#include "Buffer.h"
#include "Image.h"
#include "ImageView.h"
#include "Fence.h"
#include "Semaphore.h"
#include "FileLoader.h"
#include "Geometry.h"
#include "Shader.h"
#include "Pipeline.h"
#include "DescriptorPool.h"
#include "DescriptorSetLayout.h"
#include "DescriptorSet.h"
#include "Camera.h"
//TODO 상수값들 위치 수정
const std::vector<const char*> validationLayers = {
    "VK_LAYER_KHRONOS_validation"
};

const std::vector<const char*> deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;
const int MAX_FRAMES_IN_FLIGHT = 2;

const std::string MODEL_PATH = "Resources/models/viking_room.obj";
const std::string TEXTURE_PATH = "Resources/textures/viking_room.png";


