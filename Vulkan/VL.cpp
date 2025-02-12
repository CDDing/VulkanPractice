#include "pch.h"
#include "VL.h"

VKAPI_ATTR vk::Bool32 VKAPI_CALL debugCallback(vk::DebugUtilsMessageSeverityFlagBitsEXT       messageSeverity,
    vk::DebugUtilsMessageTypeFlagsEXT              messageTypes,
    vk::DebugUtilsMessengerCallbackDataEXT const* pCallbackData,
    void* /*pUserData*/)
{
    std::ostringstream message;

    message << vk::to_string(messageSeverity) << ": " << vk::to_string(messageTypes) << ":\n";
    message << std::string("\t") << "messageIDName   = <" << pCallbackData->pMessageIdName << ">\n";
    message << std::string("\t") << "messageIdNumber = " << pCallbackData->messageIdNumber << "\n";
    message << std::string("\t") << "message         = <" << pCallbackData->pMessage << ">\n";
    if (0 < pCallbackData->queueLabelCount)
    {
        message << std::string("\t") << "Queue Labels:\n";
        for (uint32_t i = 0; i < pCallbackData->queueLabelCount; i++)
        {
            message << std::string("\t\t") << "labelName = <" << pCallbackData->pQueueLabels[i].pLabelName << ">\n";
        }
    }
    if (0 < pCallbackData->cmdBufLabelCount)
    {
        message << std::string("\t") << "CommandBuffer Labels:\n";
        for (uint32_t i = 0; i < pCallbackData->cmdBufLabelCount; i++)
        {
            message << std::string("\t\t") << "labelName = <" << pCallbackData->pCmdBufLabels[i].pLabelName << ">\n";
        }
    }
    if (0 < pCallbackData->objectCount)
    {
        message << std::string("\t") << "Objects:\n";
        for (uint32_t i = 0; i < pCallbackData->objectCount; i++)
        {
            message << std::string("\t\t") << "Object " << i << "\n";
            message << std::string("\t\t\t") << "objectType   = " << vk::to_string(pCallbackData->pObjects[i].objectType) << "\n";
            message << std::string("\t\t\t") << "objectHandle = " << pCallbackData->pObjects[i].objectHandle << "\n";
            if (pCallbackData->pObjects[i].pObjectName)
            {
                message << std::string("\t\t\t") << "objectName   = <" << pCallbackData->pObjects[i].pObjectName << ">\n";
            }
        }
    }
    std::cout << message.str() << std::endl;
    return vk::False;
}
void populateDebugMessengerCreateInfo(vk::DebugUtilsMessengerCreateInfoEXT& createInfo) {
    createInfo.messageSeverity =
        vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose|
        vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning|
        vk::DebugUtilsMessageSeverityFlagBitsEXT::eError;
    createInfo.messageType =
        vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral|
        vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation|
        vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance;
    createInfo.pfnUserCallback = debugCallback;
}
bool checkValidationLayerSupport() {
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    for (const char* layerName : validationLayers) {
        bool layerFound = false;
        for (const auto& layerProperties : availableLayers) {
            if (strcmp(layerName, layerProperties.layerName) == 0) {
                layerFound = true;
                break;
            }
        }
        if (!layerFound) {
            return false;
        }
    }

    return true;
}


void DestroyDebugUtilsMessengerEXT(vk::Instance instance,
    vk::DebugUtilsMessengerEXT debugMessenger,
    const vk::AllocationCallbacks* pAllocator) {
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)instance.getProcAddr("vkDestroyDebugUtilsMessengerEXT");;
    if (func != nullptr) {
        func(instance, debugMessenger, reinterpret_cast<const VkAllocationCallbacks*>(pAllocator));
    }
}
