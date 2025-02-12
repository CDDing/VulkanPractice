#pragma once
class Surface
{
public:
	Surface();
	Surface(Instance& instance, GLFWwindow* window);
	operator vk::SurfaceKHR& () {
		return _surface;
	}
	GLFWwindow* GetWindow() { return _window; }
	void destroy();
private:
	vk::SurfaceKHR _surface;
	vk::Instance* _instance;
	GLFWwindow* _window;
};

