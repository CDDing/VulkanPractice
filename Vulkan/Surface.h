#pragma once
class Surface
{
public:
	Surface();
	Surface(std::shared_ptr<Instance> instance, GLFWwindow* window);
	~Surface();
	operator vk::SurfaceKHR& () {
		return _surface;
	}
	GLFWwindow* GetWindow() { return _window; }
	
private:
	vk::SurfaceKHR _surface;
	vk::Instance* _instance;
	GLFWwindow* _window;
};

