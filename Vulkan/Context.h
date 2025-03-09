#pragma once
class DContext
{
private:
	//For Initalize order
	std::vector<vk::Queue> queues;
public:
	enum QueueType {
		GRAPHICS,
		PRESENT,
		END
	};

	DContext(GLFWwindow* window);
	~DContext();

	vk::Queue& GetQueue(QueueType type) { return queues[static_cast<int>(type)]; }
	vk::Queue& GetQueue(int type) { return queues[type]; }


	GLFWwindow* window;
	vk::raii::Context context;
	vk::raii::Instance instance;
	vk::raii::DebugUtilsMessengerEXT debug;
	vk::raii::SurfaceKHR surface;
	vk::raii::PhysicalDevice physical;
	vk::raii::Device logical;
private:

	//Init functions
	vk::raii::Instance createInstance();
	vk::raii::DebugUtilsMessengerEXT createDebugMessenger();
	vk::raii::SurfaceKHR createSurface();
	vk::raii::PhysicalDevice createPhysicalDevice();
	vk::raii::Device createLogicalDevice();
	void initializeGlobalVariables();


	bool isDeviceSuitable(vk::raii::PhysicalDevice device, vk::raii::SurfaceKHR& surface);
	
	
};

static QueueFamilyIndices findQueueFamilies(vk::raii::PhysicalDevice& device, vk::raii::SurfaceKHR& surface) {
	QueueFamilyIndices indices;

	std::vector<vk::QueueFamilyProperties> queueFamilies =
		device.getQueueFamilyProperties();;

	int i = 0;
	for (const auto& queueFamily : queueFamilies) {
		if (queueFamily.queueFlags && VK_QUEUE_GRAPHICS_BIT) {
			indices.graphicsFamily = i;
		}
		VkBool32 presentSupport = device.getSurfaceSupportKHR(i, *surface);
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