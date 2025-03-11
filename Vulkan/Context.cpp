#include "pch.h"
#include "Context.h"

PFN_vkCreateDebugUtilsMessengerEXT  pfnVkCreateDebugUtilsMessengerEXT;
std::vector<const char*> getRequiredExtensions() {
    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions;
    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
    std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

    if (enableValidationLayers) {
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    return extensions;
}

DContext::DContext(GLFWwindow* window) : window(window),
    instance(createInstance()),
    debug(createDebugMessenger()),
    surface(createSurface()),
    physical(createPhysicalDevice()),
    logical(createLogicalDevice())
{
    initializeGlobalVariables();
}

DContext::~DContext()
{
    Sampler::destroy(*this);

    CommandPool::TransientPool.~CommandPool();
    DescriptorPool::Pool.~DescriptorPool();
    DescriptorSetLayout::destroy(*this);

    Material::dummy.~DImage();
}

vk::raii::Instance DContext::createInstance()
{
    if (enableValidationLayers && !checkValidationLayerSupport()) {
        throw std::runtime_error("validation layers requested, but not available!");
    }


    vk::ApplicationInfo appInfo{ "DDing",
    VK_MAKE_VERSION(1,0,0),
    "No Engine",
    VK_MAKE_VERSION(1,0,0),
    VK_API_VERSION_1_3 };

    vk::InstanceCreateInfo createInfo{ {},&appInfo };

    vk::DebugUtilsMessageSeverityFlagsEXT severityFlags(
        vk::DebugUtilsMessageSeverityFlagBitsEXT::eError |
        vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose |
        vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning);
    vk::DebugUtilsMessageTypeFlagsEXT messageTypeFlags(
        vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation |
        vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance |
        vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral);

    vk::DebugUtilsMessengerCreateInfoEXT debugCreateInfo({}, severityFlags,
        messageTypeFlags,
        &debugCallback);
    if (enableValidationLayers) {
        createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        createInfo.ppEnabledLayerNames = validationLayers.data();

        populateDebugMessengerCreateInfo(debugCreateInfo);
        createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
    }
    else {
        createInfo.enabledLayerCount = 0;
        createInfo.pNext = nullptr;
    }

    auto extensions = getRequiredExtensions();
    extensions.push_back(vk::KHRSurfaceExtensionName);
    createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    createInfo.ppEnabledExtensionNames = extensions.data();

    return vk::raii::Instance(context, createInfo);
}

vk::raii::DebugUtilsMessengerEXT DContext::createDebugMessenger()
{
    if (!enableValidationLayers) return VK_NULL_HANDLE;

    vk::DebugUtilsMessengerCreateInfoEXT createInfo;
    populateDebugMessengerCreateInfo(createInfo);

    pfnVkCreateDebugUtilsMessengerEXT = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(instance.getProcAddr("vkCreateDebugUtilsMessengerEXT"));
    
    return instance.createDebugUtilsMessengerEXT(createInfo);
}

vk::raii::SurfaceKHR DContext::createSurface()
{
    if (!glfwInit())
        throw std::runtime_error("GLFW initialization failed!");
    VkSurfaceKHR surface;
    VkResult result = glfwCreateWindowSurface(static_cast<VkInstance>(*instance), window, nullptr, &surface);
    if (result != VK_SUCCESS) {
        throw std::runtime_error("failed to create window surface!");
    }

    return vk::raii::SurfaceKHR(instance, surface);
}

vk::raii::PhysicalDevice DContext::createPhysicalDevice()
{
    vk::raii::PhysicalDevices devices(instance);
    for (const auto& device : devices) {
        if (isDeviceSuitable(device, surface)) {
            return device;
            break;
        }
    }

    if (physical == VK_NULL_HANDLE) {
        throw std::runtime_error("failed to find a suitable GPU!");
    }

    return vk::raii::PhysicalDevice(nullptr);
}

vk::raii::Device DContext::createLogicalDevice()
{
    QueueFamilyIndices indices = findQueueFamilies(physical, surface);

    std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;
    std::set<uint32_t> uniqueQueueFamilies = { indices.graphicsFamily.value(),indices.presentFamily.value() };

    float queuePriority = 1.f;
    for (uint32_t queueFamiliy : uniqueQueueFamilies) {
        vk::DeviceQueueCreateInfo queueCreateInfo{ {},queueFamiliy,1, &queuePriority };
        queueCreateInfos.push_back(queueCreateInfo);
    }
    //For RT
    vk::PhysicalDeviceDescriptorIndexingFeaturesEXT pddifEXT;
    pddifEXT.runtimeDescriptorArray = VK_TRUE;

    vk::PhysicalDeviceRayTracingPipelineFeaturesKHR rayTracingPipelineFeatures{};
    rayTracingPipelineFeatures.rayTracingPipeline = VK_TRUE;  // 기능 활성화
    rayTracingPipelineFeatures.pNext = &pddifEXT;

    VkPhysicalDeviceAccelerationStructureFeaturesKHR accelerationStructureFeatures = {};
    accelerationStructureFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR;
    accelerationStructureFeatures.accelerationStructure = VK_TRUE;
    VkPhysicalDeviceBufferDeviceAddressFeatures bufferDeviceAddressFeatures = {};
    bufferDeviceAddressFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES;
    bufferDeviceAddressFeatures.bufferDeviceAddress = VK_TRUE;
    bufferDeviceAddressFeatures.pNext = &rayTracingPipelineFeatures;

    accelerationStructureFeatures.pNext = &bufferDeviceAddressFeatures;

    VkPhysicalDeviceFeatures2 deviceFeatures{};
    deviceFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    deviceFeatures.features.samplerAnisotropy = VK_TRUE;
    deviceFeatures.features.shaderInt64 = VK_TRUE;
    deviceFeatures.pNext = &accelerationStructureFeatures;

    std::vector<const char*> validationlayers;
    if (enableValidationLayers) {
        validationlayers = validationLayers;
    }
    vk::DeviceCreateInfo createInfo{ {},queueCreateInfos,validationLayers,deviceExtensions };
    createInfo.pNext = &deviceFeatures;

    vk::raii::Device logicalDevice = physical.createDevice(createInfo);
    
    for (int i = 0; i < QueueType::END; i++) {
        vk::Queue q;
        queues.push_back(q);
    }

    GetQueue(QueueType::GRAPHICS) = logicalDevice.getQueue(indices.graphicsFamily.value(), 0);
    GetQueue(QueueType::PRESENT) = logicalDevice.getQueue(indices.presentFamily.value(), 0);
    return logicalDevice;
}

void DContext::initializeGlobalVariables()
{
    Sampler::init(*this);

    QueueFamilyIndices queueFamilyIndices = findQueueFamilies(physical, surface);
    vk::CommandPoolCreateInfo commandPoolInfo{ vk::CommandPoolCreateFlagBits::eTransient,
        queueFamilyIndices.graphicsFamily.value() };
    CommandPool::TransientPool = logical.createCommandPool(commandPoolInfo);

    std::array<vk::DescriptorPoolSize, 2> poolSizes{};
    poolSizes[0].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
    poolSizes[1].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

    vk::DescriptorPoolCreateInfo descriptorPoolInfo{ {vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet},
        static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT) * 100 ,poolSizes };

    DescriptorSetLayout::Init(*this);
    DescriptorPool::Pool = vk::raii::DescriptorPool(logical, descriptorPoolInfo);
    Material::dummy = Material::GetDefaultMaterial(*this);
}

bool DContext::isDeviceSuitable(vk::raii::PhysicalDevice device, vk::raii::SurfaceKHR& surface)
{
    QueueFamilyIndices indices = findQueueFamilies(device, surface);

    bool extensionsSupported = true;//checkDeviceExtensionSupport(device);

    bool swapChainAdequate = false;
    if (extensionsSupported) {
        SwapChainSupportDetails swapChainSupport = SwapChain::querySwapChainSupport(device, surface);
        swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
    }

    vk::PhysicalDeviceFeatures supportedFeatures = device.getFeatures();
    return indices.isComplete() && extensionsSupported && swapChainAdequate && supportedFeatures.samplerAnisotropy;
}

