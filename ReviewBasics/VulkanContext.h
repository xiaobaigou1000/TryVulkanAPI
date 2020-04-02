#pragma once
#include<vulkan/vulkan.hpp>
#include"NativeWindow.h"
class VulkanContext
{
public:
    void createInstance(const NativeWindow& window);
    void setupDebugMessenger();
    void destroy();

private:
    vk::Instance instance;
    vk::DebugUtilsMessengerCreateInfoEXT debugMessengerCreateInfo;
    vk::DebugUtilsMessengerEXT debugMessenger;

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
