#include "HelloTriangle.h"

void HelloTriangleApplication::run()
{
    initWindow();
    initVulkan();
    mainLoop();
    cleanup();
}

void HelloTriangleApplication::initWindow()
{
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    window = glfwCreateWindow(width, height, "Vulkan", nullptr, nullptr);
}

void HelloTriangleApplication::initVulkan()
{
    createInstance();
    setupDebugMessenger();
    pickPhysicalDevice();
    createLogicalDevice();
}

void HelloTriangleApplication::mainLoop()
{
    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
    }
}

void HelloTriangleApplication::cleanup()
{
    device.destroy();
    auto debugMessengerDestroyFunc = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if (debugMessengerDestroyFunc != nullptr)
    {
        debugMessengerDestroyFunc(instance, debugMessenger, nullptr);
    }
    instance.destroy();
    glfwDestroyWindow(window);
    glfwTerminate();
}

void HelloTriangleApplication::createInstance()
{
    if (enableValidationLayers && !checkValidationLayerSupport())
    {
        throw std::runtime_error("validation layers requested, but not available!");
    }

    auto extensionsSupported = vk::enumerateInstanceExtensionProperties();
    std::cout << "supported extensions:\n";
    for (const auto& i : extensionsSupported)
    {
        std::cout << i.extensionName << '\n';
    }
    std::cout << '\n';

    vk::ApplicationInfo appInfo("hello triangle", VK_MAKE_VERSION(1, 1, 0), "no engine", VK_MAKE_VERSION(1, 0, 0), VK_API_VERSION_1_0);

    auto extensions = getRequiredExtensions();
    vk::InstanceCreateInfo createInfo({}, &appInfo, 0, nullptr, static_cast<uint32_t>(extensions.size()), extensions.data());
    if (enableValidationLayers)
    {
        createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        createInfo.ppEnabledLayerNames = validationLayers.data();
        debugMessengerCreateInfo = getDebugMessengerCreateInfo();
        createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugMessengerCreateInfo;
    }

    instance = vk::createInstance(createInfo);
}

bool HelloTriangleApplication::checkValidationLayerSupport()
{
    auto availableLayers = vk::enumerateInstanceLayerProperties();
    for (const auto& layer : validationLayers)
    {
        bool layerFound = false;
        for (const auto& layerProperties : availableLayers)
        {
            if (std::string(layerProperties.layerName) == std::string(layer))
            {
                layerFound = true;
                break;
            }
        }
        if (!layerFound)
            return false;
    }
    return true;
}

void HelloTriangleApplication::setupDebugMessenger()
{
    if (!enableValidationLayers)
        return;

    auto debugCreateFunc = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if (debugCreateFunc != nullptr)
    {
        VkResult result = debugCreateFunc(instance, reinterpret_cast<VkDebugUtilsMessengerCreateInfoEXT*>(&debugMessengerCreateInfo), nullptr,reinterpret_cast<VkDebugUtilsMessengerEXT*>(&debugMessenger));
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

void HelloTriangleApplication::pickPhysicalDevice()
{
    auto physicalDevices = instance.enumeratePhysicalDevices();
    if (physicalDevices.size() == 0)
    {
        throw std::runtime_error("no physical devices supported vulkan!");
    }
    std::cout << "there are " << physicalDevices.size() << " devices supported for vulkan.\n";
    for (const auto& i : physicalDevices)
    {
        vk::PhysicalDeviceProperties deviceProperties = i.getProperties();
        vk::PhysicalDeviceFeatures deviceFeatures = i.getFeatures();
        bool suitable = deviceProperties.deviceType == vk::PhysicalDeviceType::eDiscreteGpu && deviceFeatures.geometryShader;
        if (suitable)
        {
            std::cout << "select physicalDevice : " << deviceProperties.deviceName << '\n';
            physicalDevice = i;
        }
    }
}

void HelloTriangleApplication::createLogicalDevice()
{
    uint32_t queueFamilyIndex = findQueueFamilies();
    float queuePriority = 1.0f;
    vk::DeviceQueueCreateInfo deviceQueueCreateInfo({}, queueFamilyIndex, 1, &queuePriority);
    vk::PhysicalDeviceFeatures physicalDeviceFeatures;
    vk::DeviceCreateInfo createInfo({},1,&deviceQueueCreateInfo);
    if (enableValidationLayers)
    {
        createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        createInfo.ppEnabledLayerNames = validationLayers.data();
    }
    device = physicalDevice.createDevice(createInfo);
}

vk::DebugUtilsMessengerCreateInfoEXT HelloTriangleApplication::getDebugMessengerCreateInfo()
{
    vk::DebugUtilsMessageSeverityFlagsEXT severity =
        vk::DebugUtilsMessageSeverityFlagBitsEXT::eError |
        vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose |
        vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning;

    vk::DebugUtilsMessageTypeFlagsEXT typeFlag =
        vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
        vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation |
        vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance;

    return vk::DebugUtilsMessengerCreateInfoEXT({}, severity, typeFlag, debugCallback, {});
}

std::vector<const char*> HelloTriangleApplication::getRequiredExtensions()
{
    uint32_t glfwExtensionCount = 0;
    auto glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
    std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
    if (enableValidationLayers)
    {
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    return extensions;
}

uint32_t HelloTriangleApplication::findQueueFamilies()
{
    auto queueFamilies = physicalDevice.getQueueFamilyProperties();
    std::cout << "total queue family num :" << queueFamilies.size() << '\n';
    
    uint32_t queueFamilyIndex = -1;
    for (uint32_t i = 0; i < queueFamilies.size(); ++i)
    {
        if (queueFamilies[i].queueFlags & vk::QueueFlagBits::eGraphics)
        {
            queueFamilyIndex = i;
        }
    }

    if (queueFamilyIndex < 0)
    {
        throw std::runtime_error("no queue family support" + vk::to_string(vk::QueueFlagBits::eGraphics));
    }

    std::cout << "selected queue family :" << queueFamilyIndex << '\n';
    std::cout << vk::to_string(queueFamilies[queueFamilyIndex].queueFlags) << '\n';
    std::cout << "queue count :" << queueFamilies[queueFamilyIndex].queueCount << '\n';

    return queueFamilyIndex;
}

VKAPI_ATTR VkBool32 VKAPI_CALL HelloTriangleApplication::debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData)
{
    std::cerr << "validation layer: " << pCallbackData->pMessage << '\n';
    return VK_FALSE;
}
