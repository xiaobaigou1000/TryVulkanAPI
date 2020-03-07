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
    createSurface();
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
    instance.destroySurfaceKHR(surface);
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
    auto queueFamilyIndex = findQueueFamilies();
    float queuePriority = 1.0f;
    std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;
    std::array<uint32_t, 2> queueFamilies = { queueFamilyIndex.graphicsFamily,queueFamilyIndex.presentFamily };
    for (uint32_t i : queueFamilies)
    {
        vk::DeviceQueueCreateInfo deviceQueueCreateInfo({}, i, 1, &queuePriority);
        queueCreateInfos.push_back(deviceQueueCreateInfo);
    }

    vk::DeviceCreateInfo createInfo({}, 2, queueCreateInfos.data());
    if (enableValidationLayers)
    {
        createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        createInfo.ppEnabledLayerNames = validationLayers.data();
    }
    device = physicalDevice.createDevice(createInfo);
    graphicsQueue = device.getQueue(queueFamilyIndex.graphicsFamily, 0);
    presentQueue = device.getQueue(queueFamilyIndex.presentFamily, 0);
}

void HelloTriangleApplication::createSurface()
{
    if (glfwCreateWindowSurface(instance, window, nullptr, reinterpret_cast<VkSurfaceKHR*>(&surface)) != VK_SUCCESS)
    {
        throw std::runtime_error("glfw create window surface failed.");
    }
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

HelloTriangleApplication::QueueFamilyIndices HelloTriangleApplication::findQueueFamilies()
{
    auto queueFamilies = physicalDevice.getQueueFamilyProperties();
    std::cout << "total queue family num :" << queueFamilies.size() << '\n';

    uint32_t graphicsFamily = -1;
    uint32_t presentFamily = -1;
    for (uint32_t i = 0; i < queueFamilies.size(); ++i)
    {
        if (graphicsFamily == -1 && (queueFamilies[i].queueFlags & vk::QueueFlagBits::eGraphics))
        {
            graphicsFamily = i;
        }
        if (presentFamily == -1 && physicalDevice.getSurfaceSupportKHR(i, surface))
        {
            presentFamily = i;
        }
    }

    if (graphicsFamily < 0)
    {
        throw std::runtime_error("no queue family support" + vk::to_string(vk::QueueFlagBits::eGraphics));
    }
    if (presentFamily < 0)
    {
        throw std::runtime_error("queue family unsupport win32 surface khr");
    }

    std::cout << "selected queue family :" << graphicsFamily << '\n';
    std::cout << vk::to_string(queueFamilies[graphicsFamily].queueFlags) << '\n';
    std::cout << "queue count :" << queueFamilies[graphicsFamily].queueCount << '\n';

    return { graphicsFamily,presentFamily };
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
