#pragma once
#define GLFW_INCLUDE_VULKAN
#include<GLFW/glfw3.h>
#include<vulkan/vulkan.hpp>
#include<glm.hpp>
#include<iostream>
#include<vector>
#include<functional>
#include<array>

class HelloTriangleApplication
{
public:
    void run();
private:
    struct QueueFamilyIndices
    {
        uint32_t graphicsFamily;
        uint32_t presentFamily;
    };

    struct SwapChainSupportDetails
    {
        vk::SurfaceCapabilitiesKHR capabilities;
        std::vector<vk::SurfaceFormatKHR> formats;
        std::vector<vk::PresentModeKHR> presentModes;
    };

    GLFWwindow* window;
    vk::Instance instance;
    vk::DebugUtilsMessengerEXT debugMessenger;
    vk::DebugUtilsMessengerCreateInfoEXT debugMessengerCreateInfo;
    vk::PhysicalDevice physicalDevice;
    vk::Device device;
    vk::Queue graphicsQueue;
    vk::Queue presentQueue;

    vk::SurfaceKHR surface;

    int width = 800;
    int height = 600;

    std::vector<const char*> physicalDeviceExtensions{ VK_KHR_SWAPCHAIN_EXTENSION_NAME };
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
    void checkPhysicalDeviceExtensionSupport();
    void setupDebugMessenger();
    void pickPhysicalDevice();
    void createLogicalDevice();
    void createSurface();

    SwapChainSupportDetails querySwapChainSupport();
    vk::DebugUtilsMessengerCreateInfoEXT getDebugMessengerCreateInfo();
    std::vector<const char*> getRequiredExtensions();
    QueueFamilyIndices findQueueFamilies();

    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageType,
        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void* pUserData);
};
