#include "pch.h"
#include "Instance.h"

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE

Instance::Instance()
{
}


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

VKAPI_ATTR VkResult VKAPI_CALL vkCreateDebugUtilsMessengerEXT(VkInstance                                 instance,
    const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkDebugUtilsMessengerEXT* pMessenger) {
    return pfnVkCreateDebugUtilsMessengerEXT(instance, pCreateInfo, pAllocator, pMessenger);
}
void setupDebugMessenger(vk::Instance& instance, vk::DebugUtilsMessengerEXT& debugMessenger) {
    if (!enableValidationLayers) return;

    vk::DebugUtilsMessengerCreateInfoEXT createInfo;
    populateDebugMessengerCreateInfo(createInfo);

    pfnVkCreateDebugUtilsMessengerEXT = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(instance.getProcAddr("vkCreateDebugUtilsMessengerEXT"));;
    debugMessenger = instance.createDebugUtilsMessengerEXT(createInfo);
}
Instance::Instance(const char* ApplicationName)
{
    if (enableValidationLayers && !checkValidationLayerSupport()) {
        throw std::runtime_error("validation layers requested, but not available!");
    }

    vk::detail::DynamicLoader dl;
    auto vkGetInstanceProcAddr = dl.getProcAddress<PFN_vkGetInstanceProcAddr>("vkGetInstanceProcAddr");
    VULKAN_HPP_DEFAULT_DISPATCHER.init(vkGetInstanceProcAddr);

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

    vk::DebugUtilsMessengerCreateInfoEXT debugCreateInfo( {},severityFlags,
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
    createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    createInfo.ppEnabledExtensionNames = extensions.data();

    _instance = vk::createInstance(createInfo,nullptr);
    VULKAN_HPP_DEFAULT_DISPATCHER.init(_instance);

    setupDebugMessenger(_instance,_debugMessenger);
}

void Instance::destroy()
{
    _instance.destroy();
}
