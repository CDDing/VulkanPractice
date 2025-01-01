#pragma once
enum QueueType {
    GRAPHICS,
    PRESENT,
    END
};
class Device
{
public:
    Device();
    Device(Instance& instance, Surface& surface);
    ~Device();
    VkDevice& Get() { return _device; }
    VkPhysicalDevice& GetPhysical() { return _physicalDevice; }
    VkQueue& GetQueue(QueueType type) { return _queues[type]; }
private:
    void pickPhysicalDevice(Instance& instance,Surface& surface);
    void createLogicalDevice(Surface& surface);
    bool isDeviceSuitable(VkPhysicalDevice device,Surface& surface);
    bool checkDeviceExtensionSupport(VkPhysicalDevice device) {
        return true;
    }

    VkPhysicalDevice _physicalDevice = VK_NULL_HANDLE;
    VkDevice _device;
    std::vector<VkQueue> _queues;

};

static QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface) {
    QueueFamilyIndices indices;

    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

    int i = 0;
    for (const auto& queueFamily : queueFamilies) {
        if (queueFamily.queueFlags && VK_QUEUE_GRAPHICS_BIT) {
            indices.graphicsFamily = i;
        }
        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
        if (presentSupport) {
            indices.presentFamily = i;
        }
        if (indices.isComplete()) {
            break;
        }
        i++;
    }
    return indices;
}