#pragma once
#define GLFW_INCLUDE_VULKAN
#include<GLFW/glfw3.h>
#include<vulkan/vulkan.hpp>
#include<tuple>
#include<vector>

class NativeWindow
{
public:
    NativeWindow() {}
    NativeWindow(const NativeWindow&) = delete;
    NativeWindow& operator=(const NativeWindow&) = delete;
    NativeWindow(NativeWindow&&) = delete;

    void init();
    void destroy();
    inline bool shouldClose()const { return glfwWindowShouldClose(window); };
    inline void pollEvents()const { glfwPollEvents(); };
    inline GLFWwindow* handle()const { return window; };
    inline vk::Extent2D extent()const { return vk::Extent2D{ width,height }; }

    std::vector<const char*> extensionRequirements()const;
private:
    GLFWwindow* window = nullptr;
    uint32_t width = 800, height = 600;
};
