#pragma once
#include<vulkan/vulkan.hpp>
#define GLFW_INCLUDE_VULKAN
#include<GLFW/glfw3.h>
#include<glm.hpp>
#include<iostream>
#include<vector>
#include<functional>

class HelloTriangleApplication
{
public:
    void run();
private:
    GLFWwindow* window;
    vk::Instance instance;
    vk::DebugUtilsMessengerEXT debugMessenger;
    vk::DebugUtilsMessengerCreateInfoEXT debugMessengerCreateInfo;

    int width = 800;
    int height = 600;

    std::vector<const char*> validationLayers{ "VK_LAYER_KHRONOS_validation" };
#ifdef NDEBUG
    const bool enableValidationLayers = false;
#else
    const bool enableValidationLayers = true;
#endif

    void initWindow();
    void initVulkan();
    void mainLoop();
    void cleanup();

    void createInstance();
    bool checkValidationLayerSupport();
    void setupDebugMessenger();

    vk::DebugUtilsMessengerCreateInfoEXT getDebugMessengerCreateInfo();
    std::vector<const char*> getRequiredExtensions();
    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageType,
        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void* pUserData);
};
