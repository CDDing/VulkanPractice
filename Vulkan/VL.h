#pragma once
#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif


VKAPI_ATTR vk::Bool32 VKAPI_CALL debugCallback(vk::DebugUtilsMessageSeverityFlagBitsEXT       messageSeverity,
    vk::DebugUtilsMessageTypeFlagsEXT              messageTypes,
    vk::DebugUtilsMessengerCallbackDataEXT const* pCallbackData,
    void* /*pUserData*/);

bool checkValidationLayerSupport();
void populateDebugMessengerCreateInfo(vk::DebugUtilsMessengerCreateInfoEXT& createInfo);
void DestroyDebugUtilsMessengerEXT(vk::Instance instance,
    vk::DebugUtilsMessengerEXT debugMessenger,
    const vk::AllocationCallbacks* pAllocator);
