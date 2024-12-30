#pragma once
class Surface
{
public:
	Surface();
	Surface(VkInstance* instance, GLFWwindow* window);
	VkSurfaceKHR& Get() { return _surface; }
	GLFWwindow* GetWindow() { return _window; }

private:
	VkSurfaceKHR _surface;
	GLFWwindow* _window;
};

