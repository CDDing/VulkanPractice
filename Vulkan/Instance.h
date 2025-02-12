#pragma once
class Instance
{
public:
	Instance();
	Instance(const char* ApplicationName);
	vk::DebugUtilsMessengerEXT& GetDebugMessenger() { return _debugMessenger; }
	vk::Instance& Get() { return _instance; }
	operator vk::Instance& () {
		return _instance;
	}
	operator VkInstance () {
		return (VkInstance)_instance;
	}
	void destroy();
private:
	vk::Instance _instance;
	vk::DebugUtilsMessengerEXT _debugMessenger;
};

std::vector<const char*> getRequiredExtensions();