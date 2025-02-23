#pragma once
class Instance
{
public:
	Instance(const char* ApplicationName);
	vk::DebugUtilsMessengerEXT& GetDebugMessenger() { return _debugMessenger; }
	vk::raii::Instance& Get() { return _instance; }
	operator vk::raii::Instance& () {
		return _instance;
	}
	operator VkInstance () {
		return (VkInstance)*_instance;
	}
	~Instance();
private:
	vk::raii::Context context;
	vk::raii::Instance _instance;
	vk::DebugUtilsMessengerEXT _debugMessenger;
};

std::vector<const char*> getRequiredExtensions();