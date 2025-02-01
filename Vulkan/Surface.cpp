#include "pch.h"
#include "Surface.h"

Surface::Surface()
{
}

Surface::Surface(Instance& instance,GLFWwindow* window)
{
    if (glfwCreateWindowSurface(instance, window, nullptr, &_surface) != VK_SUCCESS) {
        throw std::runtime_error("failed to create window surface!");
    }
}
