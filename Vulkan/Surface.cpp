#include "pch.h"
#include "Surface.h"
vk::raii::SurfaceKHR createSurface(Instance& instance, GLFWwindow* window)
{
	vk::raii::SurfaceKHR surface(nullptr);
    if (glfwCreateWindowSurface(*instance.Get(), window, nullptr, (VkSurfaceKHR*)&surface) != VK_SUCCESS) {
        throw std::runtime_error("failed to create window surface!");
    }
    return surface;
}
