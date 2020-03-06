#include<vulkan/vulkan.hpp>
#define GLFW_INCLUDE_VULKAN
#include<GLFW/glfw3.h>
#include<iostream>
#include<glm.hpp>

int main()
{
    glfwInit();
    GLFWwindow* window = glfwCreateWindow(800, 600, "vulkan window", nullptr, nullptr);
    auto extensions = vk::enumerateInstanceExtensionProperties();
    for (const auto& i : extensions)
    {
        std::cout << i.extensionName << '\n';
    }
    std::cout << extensions.size() << " extensions supported" << '\n';

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
    }
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}