#pragma once
enum QueueType {
    GRAPHICS,
    PRESENT,
    END
};
class Queue
{
public:
    Queue();
    Queue(Device& device);
    VkQueue& Get(QueueType type);
    static QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface) {
        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

        std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

        int i = 0;
        for (const auto& queueFamily : queueFamilies) {
            if (queueFamily.queueFlags && VK_QUEUE_GRAPHICS_BIT) {
                _indices.graphicsFamily = i;
            }
            VkBool32 presentSupport = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
            if (presentSupport) {
                _indices.presentFamily = i;
            }
            if (_indices.isComplete()) {
                break;
            }
            i++;
        }
        return _indices;
    }

private:
    static QueueFamilyIndices _indices;

    std::vector<VkQueue> _queues;
};

