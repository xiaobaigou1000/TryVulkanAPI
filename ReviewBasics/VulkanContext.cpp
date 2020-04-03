#include "VulkanContext.h"
#include<iostream>
#include<algorithm>

VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT           messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT                  messageTypes,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData)
{
    std::cerr << "validation layer :" << pCallbackData->pMessage << std::endl;
    return VK_FALSE;
}

void VulkanContext::destroy()
{
    instance.destroySurfaceKHR(surface);
    auto debugMessengerDestroyFunc = (PFN_vkDestroyDebugUtilsMessengerEXT)instance.getProcAddr("vkDestroyDebugUtilsMessengerEXT");
    if (debugMessengerDestroyFunc != nullptr)
    {
        debugMessengerDestroyFunc(instance, debugMessenger, nullptr);
    }
    instance.destroy();
}

void VulkanContext::fillDebugMessengerCreateInfo()
{
    vk::DebugUtilsMessageSeverityFlagsEXT severity =
        vk::DebugUtilsMessageSeverityFlagBitsEXT::eError |
        vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose |
        vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning;

    vk::DebugUtilsMessageTypeFlagsEXT messageType =
        vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
        vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation |
        vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance;

    PFN_vkDebugUtilsMessengerCallbackEXT pfnUserCallback = debugCallback;

    debugMessengerCreateInfo = vk::DebugUtilsMessengerCreateInfoEXT({}, severity, messageType, debugCallback);
}

void VulkanContext::init(const NativeWindow& window)
{
    createInstance(window);
    setupDebugMessenger();
    createWindowSurface(window);
    selectPhysicalDevice();
    selectQueueFamily();
}

void VulkanContext::createInstance(const NativeWindow& window)
{
    vk::ApplicationInfo appInfo("review", VK_MAKE_VERSION(0, 1, 0), "hello engine", VK_MAKE_VERSION(0, 1, 0), VK_API_VERSION_1_0);
    std::vector<const char*> extensions = window.extensionRequirements();

    fillDebugMessengerCreateInfo();
    vk::InstanceCreateInfo createInfo({}, &appInfo, 0, nullptr);
    if (enableValidationLayers)
    {
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        createInfo.ppEnabledLayerNames = validationLayers.data();
        createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugMessengerCreateInfo;
    }
    createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    createInfo.ppEnabledExtensionNames = extensions.data();
    instance = vk::createInstance(createInfo);
}

void VulkanContext::setupDebugMessenger()
{
    if (!enableValidationLayers)
    {
        return;
    }

    auto debugCreateFunc = (PFN_vkCreateDebugUtilsMessengerEXT)instance.getProcAddr("vkCreateDebugUtilsMessengerEXT");
    if (debugCreateFunc != nullptr)
    {
        VkResult result = debugCreateFunc(instance, reinterpret_cast<VkDebugUtilsMessengerCreateInfoEXT*>(&debugMessengerCreateInfo), nullptr, reinterpret_cast<VkDebugUtilsMessengerEXT*>(&debugMessenger));
        if (result != VK_SUCCESS)
        {
            throw std::runtime_error("failes to set up debug messenger");
        }
    }
    else
    {
        throw std::runtime_error("can't find vkCreateDebugUtilsMessengerEXT function.");
    }
}

void VulkanContext::selectPhysicalDevice()
{
    auto physicalDevices = instance.enumeratePhysicalDevices();
    for (const auto& i : physicalDevices)
    {
        auto features = i.getFeatures();
        auto properties = i.getProperties();
        if (properties.deviceType == vk::PhysicalDeviceType::eDiscreteGpu && features.geometryShader)
        {
            physicalDevice = i;
        }
    }
}

void VulkanContext::selectQueueFamily()
{
    auto queueFamilyProperties = physicalDevice.getQueueFamilyProperties();
    uint32_t index = 0;
    for (const auto& i : queueFamilyProperties)
    {
        if (i.queueFlags | vk::QueueFlagBits::eGraphics | vk::QueueFlagBits::eCompute | vk::QueueFlagBits::eTransfer)
        {
            if (physicalDevice.getSurfaceSupportKHR(index, surface))
            {
                queueFamilyIndex = index;
            }
        }
        ++index;
    }
    if (queueFamilyIndex == -1)
    {
        throw std::runtime_error("physical device has no queue family suitable");
    }
}

vk::Device VulkanContext::createLogicalDevice(const std::vector<const char*>& deviceExtensions, vk::PhysicalDeviceFeatures physicalDeviceFeatures)
{
    float queuePriority = 1.0f;
    vk::DeviceQueueCreateInfo queueCreateInfo({}, queueFamilyIndex, 1, &queuePriority);
    vk::DeviceCreateInfo createInfo({}, 1, &queueCreateInfo, 0, nullptr, static_cast<uint32_t>(deviceExtensions.size()), deviceExtensions.data(), &physicalDeviceFeatures);
    vk::Device device = physicalDevice.createDevice(createInfo);
    return device;
}

void VulkanContext::createWindowSurface(const NativeWindow& window)
{
    glfwCreateWindowSurface(instance, window.handle(), nullptr, reinterpret_cast<VkSurfaceKHR*>(&surface));
}
