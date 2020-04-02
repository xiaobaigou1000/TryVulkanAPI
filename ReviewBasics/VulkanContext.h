#pragma once
#include<vulkan/vulkan.hpp>
#include"NativeWindow.h"
class VulkanContext
{
public:
    void init(const NativeWindow& window);
    void createInstance(const NativeWindow& window);
    void setupDebugMessenger();
    void selectPhysicalDevice();
    void selectQueueFamily();
    inline uint32_t getQueueFamilyIndex() { return queueFamilyIndex; };
    vk::Device createLogicalDevice(const std::vector<const char*>& deviceExtensions, vk::PhysicalDeviceFeatures physicalDeviceFeatures);
    void createWindowSurface(const NativeWindow& window);
    void destroy();

private:
    vk::Instance instance;
    vk::PhysicalDevice physicalDevice;
    uint32_t queueFamilyIndex = -1;
    vk::DebugUtilsMessengerCreateInfoEXT debugMessengerCreateInfo;
    vk::DebugUtilsMessengerEXT debugMessenger;
    vk::SurfaceKHR surface;

    void fillDebugMessengerCreateInfo();

#ifdef NDEBUG
    const bool enableValidationLayers = false;
#else
    const bool enableValidationLayers = true;
#endif

    const std::vector<const char*> validationLayers{
        "VK_LAYER_KHRONOS_validation"
    };
};
