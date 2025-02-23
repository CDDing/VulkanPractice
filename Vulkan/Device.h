#pragma once
class Device
{
public:
    enum QueueType {
        GRAPHICS,
        PRESENT,
        END
    };
    Device(Instance& instance, vk::raii::SurfaceKHR& surface);

    operator vk::raii::Device& () {
        return logical;
    }
    operator vk::raii::PhysicalDevice& () {
        return physical;
    }
    vk::Queue& GetQueue(int type) { return _queues[type]; }
    vk::raii::PhysicalDevice physical = VK_NULL_HANDLE;
    vk::raii::Device logical;
private:
    void pickPhysicalDevice(Instance& instance, vk::raii::SurfaceKHR& surface);
    void createLogicalDevice(vk::raii::SurfaceKHR& surface);
    bool isDeviceSuitable(vk::raii::PhysicalDevice device, vk::raii::SurfaceKHR& surface);
    bool checkDeviceExtensionSupport(vk::PhysicalDevice device) {
        return true;
    }

    std::vector<vk::Queue> _queues;

};

static QueueFamilyIndices findQueueFamilies(vk::raii::PhysicalDevice& device, vk::SurfaceKHR surface) {
    QueueFamilyIndices indices;
    
    std::vector<vk::QueueFamilyProperties> queueFamilies =
        device.getQueueFamilyProperties();;
    
    int i = 0;
    for (const auto& queueFamily : queueFamilies) {
        if (queueFamily.queueFlags && VK_QUEUE_GRAPHICS_BIT) {
            indices.graphicsFamily = i;
        }
        VkBool32 presentSupport = device.getSurfaceSupportKHR(i, surface);
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