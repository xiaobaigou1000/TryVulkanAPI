#include<vulkan/vulkan.hpp>
#define GLFW_INCLUDE_VULKAN
#include<GLFW/glfw3.h>
#include<iostream>
#include<glm.hpp>

int main()
{
    glfwInit();
    GLFWwindow* window = glfwCreateWindow(800, 600, "vulkan window", nullptr, nullptr);
    unsigned int extensionCount = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
    std::cout << extensionCount << " extensions supported" << '\n';

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
    }
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}