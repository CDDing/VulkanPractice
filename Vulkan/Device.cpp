#include "pch.h"
#include "Device.h"

//Setup with Vulkan Physical and Logical Device
Device::Device(Instance& instance, vk::raii::SurfaceKHR& surface) : logical(VK_NULL_HANDLE), physical(VK_NULL_HANDLE)
{


    pickPhysicalDevice(instance,surface);
    createLogicalDevice(surface);

    //Initialize Global
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

void Device::pickPhysicalDevice(Instance& instance, vk::raii::SurfaceKHR& surface)
{
    vk::raii::PhysicalDevices devices(instance);
    
    for (const auto& device : devices) {
        if (isDeviceSuitable(device,surface)) {
            physical = device;
            break;
        }
    }

    if (physical == VK_NULL_HANDLE) {
        throw std::runtime_error("failed to find a suitable GPU!");
    }
}

void Device::createLogicalDevice(vk::raii::SurfaceKHR& surface)
{
    QueueFamilyIndices indices = findQueueFamilies(physical,surface);

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
    vk::DeviceCreateInfo createInfo{ {},queueCreateInfos,validationLayers,deviceExtensions};
    createInfo.pNext = &deviceFeatures;

    logical = physical.createDevice(createInfo);
    //VULKAN_HPP_DEFAULT_DISPATCHER.init(logical);
    
    for (int i = 0; i < QueueType::END; i++) {
        vk::Queue q;
        _queues.push_back(q);
    }

    GetQueue(QueueType::GRAPHICS) = logical.getQueue(indices.graphicsFamily.value(), 0);
    GetQueue(QueueType::PRESENT) = logical.getQueue(indices.presentFamily.value(), 0);
}

bool Device::isDeviceSuitable(vk::raii::PhysicalDevice device, vk::raii::SurfaceKHR& surface)
{
    QueueFamilyIndices indices = findQueueFamilies(device,surface);

    bool extensionsSupported = checkDeviceExtensionSupport(device);

    bool swapChainAdequate = false;
    if (extensionsSupported) {
       SwapChainSupportDetails swapChainSupport = SwapChain::querySwapChainSupport(device,surface);
        swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
    }

    vk::PhysicalDeviceFeatures supportedFeatures = device.getFeatures();
    return indices.isComplete() && extensionsSupported && swapChainAdequate && supportedFeatures.samplerAnisotropy;
}
