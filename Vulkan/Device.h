#pragma once
class Device
{
public:
    Device();
    Device(VkInstance& instance, Surface& surface);
    ~Device();
    VkDevice& Get() { return _device; }
    VkPhysicalDevice& GetPhysical() { return _physicalDevice; }
private:
    void pickPhysicalDevice(VkInstance& instance,Surface& surface);
    void createLogicalDevice(Surface& surface);
    bool isDeviceSuitable(VkPhysicalDevice device,Surface& surface);
    bool checkDeviceExtensionSupport(VkPhysicalDevice device) {
        return true;
    }

    VkPhysicalDevice _physicalDevice = VK_NULL_HANDLE;
    VkDevice _device;

};

