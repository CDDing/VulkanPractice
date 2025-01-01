#pragma once
class Instance
{
public:
	Instance();
	Instance(const char* ApplicationName);
	VkInstance& Get() { return _instance; }
	VkDebugUtilsMessengerEXT& GetDebugMessenger() { return _debugMessenger; }

private:
	VkInstance _instance;
	VkDebugUtilsMessengerEXT _debugMessenger;
};


std::vector<const char*> getRequiredExtensions();