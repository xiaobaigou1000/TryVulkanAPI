#include "NativeWindow.h"

void NativeWindow::init()
{
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    window = glfwCreateWindow(width, height, "review", nullptr, nullptr);
}

void NativeWindow::destroy()
{
    glfwDestroyWindow(window);
    glfwTerminate();
}

bool NativeWindow::shouldClose()const
{
    return glfwWindowShouldClose(window);
}

void NativeWindow::pollEvents()const
{
    glfwPollEvents();
}

GLFWwindow* NativeWindow::handle()const
{
    return window;
}

std::vector<const char*> NativeWindow::extensionRequirements()const
{
    uint32_t glfwExtensionCount;
    const char** glfwExtensions;
    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
    std::vector<const char*> extensionRequirements(glfwExtensions, glfwExtensions + glfwExtensionCount);
    return extensionRequirements;
}