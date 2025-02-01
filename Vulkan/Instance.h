#pragma once
class Instance
{
public:
	Instance();
	Instance(const char* ApplicationName);
	VkDebugUtilsMessengerEXT& GetDebugMessenger() { return _debugMessenger; }

	operator VkInstance& () {
		return _instance;
	}
private:
	VkInstance _instance;
	VkDebugUtilsMessengerEXT _debugMessenger;
};


std::vector<const char*> getRequiredExtensions();