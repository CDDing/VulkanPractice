#pragma once

#include <DirectXTex.h>
#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#include <vulkan/vulkan_raii.hpp>
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
#include <string>
#include <utility>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/ext/scalar_constants.hpp>


const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;
const int MAX_FRAMES_IN_FLIGHT = 2;

#include "Utils.h"
#include "Struct.h"
#include "Instance.h"
#include "Surface.h"
#include "Device.h"
#include "CommandPool.h"
#include "VL.h"
#include "Sampler.h"
#include "SwapChain.h"
#include "Buffer.h"
#include "Image.h"
#include "FileLoader.h"
#include "Pipeline.h"
#include "DescriptorSetLayout.h"
#include "Shader.h"
#include "Camera.h"
#include "Material.h"
#include "Model.h"
#include "Skybox.h"
#include "Mesh.h"
#include "GUI.h"
#include "RayTracing.h"
#include "Scene.h"
#include "Deferred.h"
#include "PostProcessing.h"
//TODO 상수값들 위치 수정
const std::vector<const char*> validationLayers = {
    "VK_LAYER_KHRONOS_validation",
};

const std::vector<const char*> deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME,
    VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,
    VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME,
    
    VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME,
    VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME,
    VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME,
    
    VK_KHR_SPIRV_1_4_EXTENSION_NAME,

    VK_KHR_SHADER_FLOAT_CONTROLS_EXTENSION_NAME,

};
