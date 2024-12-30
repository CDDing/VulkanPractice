#pragma once
class Device
{
public:
    Device();
    Device(VkInstance* instance, VkSurfaceKHR* surface, VkQueue& graphics, VkQueue& present);
    ~Device();
    VkDevice& Get() { return *_device; }
    VkPhysicalDevice& GetPhysical() { return *_physicalDevice; }
private:
    void pickPhysicalDevice();
    void createLogicalDevice(VkQueue& graphics, VkQueue& present);
    bool isDeviceSuitable(VkPhysicalDevice device);
    bool checkDeviceExtensionSupport(VkPhysicalDevice device) {
        return true;
    }

    VkPhysicalDevice* _physicalDevice = VK_NULL_HANDLE;
    VkDevice* _device;
    VkInstance* _instance;
    VkSurfaceKHR* _surface;

};

