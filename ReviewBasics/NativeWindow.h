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
    bool shouldClose()const;
    void pollEvents()const;
    GLFWwindow* handle()const;

    std::vector<const char*> extensionRequirements()const;
private:
    GLFWwindow* window;
    int width = 800, height = 600;
};
