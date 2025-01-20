#pragma once
#include <DirectXTex.h>
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
#include <string>
#include <utility>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/ext/scalar_constants.hpp>
#include "Utils.h"
#include "Struct.h"
#include "Instance.h"
#include "Surface.h"
#include "Device.h"
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
#include "Shader.h"
#include "Pipeline.h"
#include "DescriptorPool.h"
#include "DescriptorSet.h"
#include "DescriptorSetLayout.h"
#include "Camera.h"
#include "Material.h"
#include "Model.h"
#include "Mesh.h"
#include "GUI.h"
//TODO ������� ��ġ ����
const std::vector<const char*> validationLayers = {
    "VK_LAYER_KHRONOS_validation"
};

const std::vector<const char*> deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;
const int MAX_FRAMES_IN_FLIGHT = 2;