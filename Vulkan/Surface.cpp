#include "pch.h"
#include "Surface.h"
vk::raii::SurfaceKHR createSurface(Instance& instance, GLFWwindow* window)
{
    if (!glfwInit())
        throw std::runtime_error("GLFW initialization failed!");
    VkSurfaceKHR surface;
    VkResult result = glfwCreateWindowSurface(static_cast<VkInstance>(*instance.Get()), window, nullptr, &surface);
    if (result != VK_SUCCESS) {
        throw std::runtime_error("failed to create window surface!");
    }
    
    return vk::raii::SurfaceKHR(instance,surface);
}
