#pragma once
#include<vulkan/vulkan.hpp>
#include"NativeWindow.h"
class VulkanContext
{
public:
    VulkanContext() {}
    VulkanContext(const VulkanContext&) = delete;
    VulkanContext(VulkanContext&&) = delete;
    VulkanContext& operator=(const VulkanContext&) = delete;

    void init(const NativeWindow& window);
    void createInstance(const NativeWindow& window);
    void setupDebugMessenger();
    void selectPhysicalDevice();
    void selectQueueFamily();
    inline const uint32_t getQueueFamilyIndex()const { return queueFamilyIndex; }
    inline const vk::PhysicalDevice getPhysicalDeviceHandle()const { return physicalDevice; }
    inline const vk::SurfaceKHR getSurface()const { return surface; }
    vk::Device createLogicalDevice(const std::vector<const char*>& deviceExtensions, vk::PhysicalDeviceFeatures physicalDeviceFeatures);
    void createWindowSurface(const NativeWindow& window);
    void destroy();

    uint32_t findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties);
    vk::Format findSupportedFormat(const std::vector<vk::Format>& candidates, vk::ImageTiling tiling, vk::FormatFeatureFlags features);
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
