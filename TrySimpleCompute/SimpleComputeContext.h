#pragma once
#include<vulkan/vulkan.hpp>
#include<iostream>

class SimpleComputeContext
{
public:
    uint32_t queueFamilyIndex = 0;

    vk::Instance instance;
    vk::PhysicalDevice physicalDevice;
    vk::Device device;
    vk::Queue queue;
    vk::CommandPool commandPool;
    vk::DebugUtilsMessengerCreateInfoEXT debugCreateInfo;
    vk::DebugUtilsMessengerEXT debugMessenger;

    inline vk::Device getDevice()const { return device; }
    inline vk::Queue getQueue()const { return queue; }
    inline vk::CommandPool getCommandPool()const { return commandPool; }

    SimpleComputeContext()
    {
        fillDebugCreateInfo();
        createInstance();
        createDebugCallBack();
        selectPhysicalDevice();
        createLogicalDevice();
        initQueue();
        createCommandPool();
    }

    ~SimpleComputeContext()
    {
        device.destroyCommandPool(commandPool);
        device.destroy();
        destroyDebugCallBack();
        instance.destroy();
    }

    void fillDebugCreateInfo()
    {
        vk::DebugUtilsMessengerCreateInfoEXT createInfo(
            {},
            {
                vk::DebugUtilsMessageSeverityFlagBitsEXT::eError |
                vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose |
                vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning
            },
            {
                vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
                vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance |
                vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation
            },
            debugCallback,
            nullptr);
        debugCreateInfo = createInfo;
    }

    void createInstance()
    {
        std::vector<const char*> extensions;
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        std::vector<const char*> layers;
        layers.push_back("VK_LAYER_KHRONOS_validation");

        vk::ApplicationInfo appInfo("hello compute", VK_MAKE_VERSION(0, 1, 0), "simple compute", VK_MAKE_VERSION(0, 1, 0), VK_API_VERSION_1_0);
        vk::InstanceCreateInfo instanceInfo({}, &appInfo, layers.size(), layers.data(), extensions.size(), extensions.data());
        instanceInfo.pNext = &debugCreateInfo;
        instance = vk::createInstance(instanceInfo);
    }

    void createDebugCallBack()
    {
        auto debugCreateFunc = (PFN_vkCreateDebugUtilsMessengerEXT)instance.getProcAddr("vkCreateDebugUtilsMessengerEXT");
        if (debugCreateFunc != nullptr)
        {
            VkResult result = debugCreateFunc(instance, reinterpret_cast<VkDebugUtilsMessengerCreateInfoEXT*>(&debugCreateInfo), nullptr, reinterpret_cast<VkDebugUtilsMessengerEXT*>(&debugMessenger));
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

    void destroyDebugCallBack()
    {
        auto debugMessengerDestroyFunc = (PFN_vkDestroyDebugUtilsMessengerEXT)instance.getProcAddr("vkDestroyDebugUtilsMessengerEXT");
        if (debugMessengerDestroyFunc != nullptr)
        {
            debugMessengerDestroyFunc(instance, debugMessenger, nullptr);
        }
    }

    void selectPhysicalDevice()
    {
        physicalDevice = instance.enumeratePhysicalDevices().front();
    }

    void createLogicalDevice()
    {
        float queuePriority = 1.0f;
        vk::DeviceQueueCreateInfo queueInfo({}, queueFamilyIndex, 1, &queuePriority);
        vk::DeviceCreateInfo deviceInfo({}, 1, &queueInfo, 0, nullptr, 0, nullptr, nullptr);
        device = physicalDevice.createDevice(deviceInfo);
    }

    void initQueue()
    {
        queue = device.getQueue(queueFamilyIndex, 0);
    }

    void createCommandPool()
    {
        vk::CommandPoolCreateInfo commandPoolInfo({}, queueFamilyIndex);
        commandPool = device.createCommandPool(commandPoolInfo);
    }

    uint32_t findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties)const
    {
        vk::PhysicalDeviceMemoryProperties memoryProperties = physicalDevice.getMemoryProperties();
        for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; ++i)
        {
            bool memoryTypeSupport = typeFilter & (1 << i);
            bool memoryPropertySupport = (memoryProperties.memoryTypes[i].propertyFlags & properties) == properties;
            if (memoryTypeSupport && memoryPropertySupport)
            {
                return i;
            }
        }
        throw std::runtime_error("no memory type supported.");
    }

    std::tuple<vk::Buffer, vk::DeviceMemory> createHostBuffer(vk::DeviceSize size, vk::BufferUsageFlags usage)
    {
        vk::BufferCreateInfo bufferInfo(
            {},
            size,
            vk::BufferUsageFlagBits::eStorageBuffer,
            vk::SharingMode::eExclusive,
            1,
            &queueFamilyIndex);
        vk::Buffer buffer = device.createBuffer(bufferInfo);
        vk::MemoryRequirements requirements = device.getBufferMemoryRequirements(buffer);
        vk::MemoryAllocateInfo allocInfo(
            requirements.size,
            findMemoryType(
                requirements.memoryTypeBits,
                vk::MemoryPropertyFlagBits::eHostCoherent | vk::MemoryPropertyFlagBits::eHostVisible));
        vk::DeviceMemory memory = device.allocateMemory(allocInfo);
        device.bindBufferMemory(buffer, memory, 0);
        return { buffer,memory };
    }

    void destroyBufferAndFreeMemory(vk::Buffer buffer, vk::DeviceMemory memory)
    {
        device.destroyBuffer(buffer);
        device.freeMemory(memory);
    }

    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT           messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT                  messageTypes,
        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void* pUserData)
    {
        std::cerr << "validation layer :" << pCallbackData->pMessage << std::endl;
        return VK_FALSE;
    }
};