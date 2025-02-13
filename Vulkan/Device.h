#pragma once
class Device
{
public:
    enum QueueType {
        GRAPHICS,
        PRESENT,
        END
    };
    Device();
    Device(std::shared_ptr<Instance> instance, std::shared_ptr<Surface> surface);
    ~Device();

    operator vk::Device& () {
        return logical;
    }
    operator vk::PhysicalDevice& () {
        return physical;
    }
    vk::Queue& GetQueue(int type) { return _queues[type]; }
    vk::PhysicalDevice physical = VK_NULL_HANDLE;
    vk::Device logical;
private:
    void pickPhysicalDevice(vk::Instance& instance, vk::SurfaceKHR& surface);
    void createLogicalDevice(vk::SurfaceKHR& surface);
    bool isDeviceSuitable(vk::PhysicalDevice device,vk::SurfaceKHR& surface);
    bool checkDeviceExtensionSupport(vk::PhysicalDevice device) {
        return true;
    }

    std::vector<vk::Queue> _queues;

};

static QueueFamilyIndices findQueueFamilies(vk::PhysicalDevice device, vk::SurfaceKHR& surface) {
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