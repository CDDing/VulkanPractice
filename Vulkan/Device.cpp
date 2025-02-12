#include "pch.h"
#include "Device.h"
Device::Device() {

}

//Setup with Vulkan Physical and Logical Device
Device::Device(vk::Instance& instance, vk::SurfaceKHR& surface)
{


    pickPhysicalDevice(instance,surface);
    createLogicalDevice(surface);

}

void Device::pickPhysicalDevice(vk::Instance& instance,vk::SurfaceKHR& surface)
{
    uint32_t deviceCount = 0;
    auto devices = instance.enumeratePhysicalDevices();
    
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

void Device::createLogicalDevice(vk::SurfaceKHR& surface)
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
    VULKAN_HPP_DEFAULT_DISPATCHER.init(logical);
    
    for (int i = 0; i < QueueType::END; i++) {
        vk::Queue q;
        _queues.push_back(q);
    }

    GetQueue(QueueType::GRAPHICS) = logical.getQueue(indices.graphicsFamily.value(), 0);
    GetQueue(QueueType::PRESENT) = logical.getQueue(indices.presentFamily.value(), 0);
}

bool Device::isDeviceSuitable(vk::PhysicalDevice device,vk::SurfaceKHR& surface)
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
Device::~Device() {

}
