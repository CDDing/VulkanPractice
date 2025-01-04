#include "pch.h"
#include "Device.h"
Device::Device() {

}

//Setup with Vulkan Physical and Logical Device
Device::Device(Instance& instance, Surface& surface)
{


    pickPhysicalDevice(instance,surface);
    createLogicalDevice(surface);

}

void Device::pickPhysicalDevice(Instance& instance,Surface& surface)
{
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(instance.Get(), &deviceCount, nullptr);

    if (deviceCount == 0) {
        throw std::runtime_error("failed to find GPUs with Vulkan support!");
    }

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(instance.Get(), &deviceCount, devices.data());

    for (const auto& device : devices) {
        if (isDeviceSuitable(device,surface)) {
            _physicalDevice = device;
            break;
        }
    }

    if (_physicalDevice == VK_NULL_HANDLE) {
        throw std::runtime_error("failed to find a suitable GPU!");
    }
}

void Device::createLogicalDevice(Surface& surface)
{
    QueueFamilyIndices indices = findQueueFamilies(_physicalDevice,surface.Get());

    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::set<uint32_t> uniqueQueueFamilies = { indices.graphicsFamily.value(),indices.presentFamily.value() };

    float queuePriority = 1.0f;
    for (uint32_t queueFamiliy : uniqueQueueFamilies) {
        VkDeviceQueueCreateInfo queueCreateInfo{};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.queueFamilyIndex = queueFamiliy;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(queueCreateInfo);
    }

    VkPhysicalDeviceFeatures deviceFeatures{};
    deviceFeatures.samplerAnisotropy = VK_TRUE;

    VkDeviceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    createInfo.pQueueCreateInfos = queueCreateInfos.data();
    createInfo.pEnabledFeatures = &deviceFeatures;

    //현대 코드에선 필요없지만 구식 코드와의 호환을 위해
    createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
    createInfo.ppEnabledExtensionNames = deviceExtensions.data();

    if (enableValidationLayers) {
        createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        createInfo.ppEnabledLayerNames = validationLayers.data();
    }
    else {
        createInfo.enabledLayerCount = 0;
    }

    if (vkCreateDevice(_physicalDevice, &createInfo, nullptr, &_device) != VK_SUCCESS) {
        throw std::runtime_error("failed to create logical device!");
    }


    for (int i = 0; i < QueueType::END; i++) {
        VkQueue q;
        _queues.push_back(q);
    }
    vkGetDeviceQueue(_device, indices.graphicsFamily.value(), 0, &GetQueue(QueueType::GRAPHICS));
    vkGetDeviceQueue(_device, indices.presentFamily.value(), 0, &GetQueue(QueueType::PRESENT));


}

bool Device::isDeviceSuitable(VkPhysicalDevice device,Surface& surface)
{
    QueueFamilyIndices indices = findQueueFamilies(device,surface.Get());

    bool extensionsSupported = checkDeviceExtensionSupport(device);

    bool swapChainAdequate = false;
    if (extensionsSupported) {
       SwapChainSupportDetails swapChainSupport = SwapChain::querySwapChainSupport(device,surface.Get());
        swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
    }

    VkPhysicalDeviceFeatures supportedFeatures;
    vkGetPhysicalDeviceFeatures(device, &supportedFeatures);
    return indices.isComplete() && extensionsSupported && swapChainAdequate && supportedFeatures.samplerAnisotropy;
    
    
}
Device::~Device() {

}
