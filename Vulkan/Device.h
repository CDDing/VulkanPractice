#pragma once
class Device
{
public:
    Device();
    Device(VkInstance* instance, VkSurfaceKHR* surface);
    ~Device();
    VkDevice& Get() { return *_device; }
    VkPhysicalDevice& GetPhysical() { return *_physicalDevice; }
private:
    void pickPhysicalDevice();
    void createLogicalDevice();
    bool isDeviceSuitable(VkPhysicalDevice device);
    bool checkDeviceExtensionSupport(VkPhysicalDevice device) {
        return true;
    }

    VkPhysicalDevice* _physicalDevice = VK_NULL_HANDLE;
    VkDevice* _device;
    VkInstance* _instance;
    VkSurfaceKHR* _surface;

};

