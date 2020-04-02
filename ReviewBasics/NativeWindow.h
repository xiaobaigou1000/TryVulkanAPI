#pragma once
#define GLFW_INCLUDE_VULKAN
#include<GLFW/glfw3.h>
#include<tuple>
#include<vector>

class NativeWindow
{
public:
    void init();
    void destroy();
    inline bool shouldClose()const { return glfwWindowShouldClose(window); };
    inline void pollEvents()const { glfwPollEvents(); };
    inline GLFWwindow* handle()const { return window; };

    std::vector<const char*> extensionRequirements()const;
private:
    GLFWwindow* window;
    int width = 800, height = 600;
};
