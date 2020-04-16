#include<iostream>
#include<vulkan/vulkan.hpp>
#include<fstream>
#include<iomanip>

VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT           messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT                  messageTypes,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData)
{
    std::cerr << "validation layer :" << pCallbackData->pMessage << std::endl;
    return VK_FALSE;
}

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

    SimpleComputeContext()
    {
        fillDebugCreateInfo();
        createInstance();
        createDebugCallBack();
        selectPhysicalDevice();
        createLogicalDevice();
        getQueue();
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

    void getQueue()
    {
        queue = device.getQueue(queueFamilyIndex, 0);
    }

    void createCommandPool()
    {
        vk::CommandPoolCreateInfo commandPoolInfo({}, queueFamilyIndex);
        commandPool = device.createCommandPool(commandPoolInfo);
    }

    uint32_t findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties)
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
};

int main()
{
    SimpleComputeContext context;
    vk::Device device = context.device;
    float input[50];
    for (uint32_t i = 0; i < 50; i++)
    {
        input[i] = i;
    }

    std::cout << "input :\n";
    for (int i = 0; i < 5; i++)
    {
        for (int j = 0; j < 10; j++)
        {
            std::cout << std::setw(5) << input[10 * i + j];
        }
        std::cout << '\n';
    }
    std::cout << "\n\n\n";
    vk::BufferCreateInfo bufferInfo(
        {},
        sizeof(input),
        vk::BufferUsageFlagBits::eStorageBuffer,
        vk::SharingMode::eExclusive,
        1,
        &context.queueFamilyIndex);
    vk::Buffer inputBuffer = device.createBuffer(bufferInfo);
    vk::MemoryRequirements inputBufferMemoryRequirement = device.getBufferMemoryRequirements(inputBuffer);
    vk::MemoryAllocateInfo allocInfo(
        inputBufferMemoryRequirement.size,
        context.findMemoryType(
            inputBufferMemoryRequirement.memoryTypeBits,
            vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent));
    vk::DeviceMemory inputBufferMemory = device.allocateMemory(allocInfo);
    device.bindBufferMemory(inputBuffer, inputBufferMemory, 0);

    void* data = device.mapMemory(inputBufferMemory, 0, sizeof(input));
    memcpy(data, input, sizeof(input));
    device.unmapMemory(inputBufferMemory);

    vk::DescriptorSetLayoutBinding uniformBinding(0, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute, nullptr);
    vk::DescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo({}, 1, &uniformBinding);
    vk::DescriptorSetLayout descriptorSetLayout = device.createDescriptorSetLayout(descriptorSetLayoutCreateInfo);
    vk::PipelineLayoutCreateInfo pipelineLayoutCreateInfo({}, 1, &descriptorSetLayout, 0, nullptr);
    vk::PipelineLayout pipelineLayout = device.createPipelineLayout(pipelineLayoutCreateInfo);

    //load compute shader stage
    std::ifstream file("./comp.spv", std::ios::binary);
    std::vector<char> fileStr{ std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>() };
    file.close();
    vk::ShaderModuleCreateInfo createInfo({}, fileStr.size(), reinterpret_cast<const uint32_t*>(fileStr.data()));
    vk::ShaderModule computeShaderModule = device.createShaderModule(createInfo);
    vk::PipelineShaderStageCreateInfo computeStageInfo({}, vk::ShaderStageFlagBits::eCompute, computeShaderModule, "main");

    vk::ComputePipelineCreateInfo computePipelineInfo({}, computeStageInfo, pipelineLayout);
    vk::Pipeline computePipeline = device.createComputePipeline({}, computePipelineInfo);
    device.destroyShaderModule(computeShaderModule);

    //create descriptor pool and update descriptor set
    vk::DescriptorPoolSize poolSize(vk::DescriptorType::eStorageBuffer, 1);
    vk::DescriptorPoolCreateInfo descriptorPoolInfo({}, 1, 1, &poolSize);
    vk::DescriptorPool descriptorPool = device.createDescriptorPool(descriptorPoolInfo);
    vk::DescriptorSetAllocateInfo descriptorSetAllocInfo(descriptorPool, 1, &descriptorSetLayout);
    vk::DescriptorSet descriptorSet = device.allocateDescriptorSets(descriptorSetAllocInfo).front();
    vk::DescriptorBufferInfo descriptorBufferInfo(inputBuffer, 0, sizeof(input));
    vk::WriteDescriptorSet writeDescriptorSet(
        descriptorSet,
        0,
        0,
        1,
        vk::DescriptorType::eStorageBuffer,
        nullptr,
        &descriptorBufferInfo,
        nullptr);
    device.updateDescriptorSets(writeDescriptorSet, {});

    vk::CommandBufferAllocateInfo commandInfo(context.commandPool, vk::CommandBufferLevel::ePrimary, 1);
    vk::CommandBuffer command = device.allocateCommandBuffers(commandInfo).front();
    vk::CommandBufferBeginInfo commandBeginInfo({}, nullptr);
    command.begin(commandBeginInfo);
    command.bindPipeline(vk::PipelineBindPoint::eCompute, computePipeline);
    command.bindDescriptorSets(vk::PipelineBindPoint::eCompute, pipelineLayout, 0, descriptorSet, {});
    command.dispatch(50, 1, 1);
    command.end();

    vk::SubmitInfo submitInfo(0, nullptr, nullptr, 1, &command, 0, nullptr);
    context.queue.submit(submitInfo, {});
    context.queue.waitIdle();

    void* result = device.mapMemory(inputBufferMemory, 0, sizeof(input));
    float output[50];
    memcpy(output, result, sizeof(input));

    std::cout << "output :\n";
    for (int i = 0; i < 5; i++)
    {
        for (int j = 0; j < 10; j++)
        {
            std::cout << std::setw(5) << output[10 * i + j];
        }
        std::cout << '\n';
    }

    //clean up
    device.destroyPipeline(computePipeline);
    device.destroyDescriptorPool(descriptorPool);
    device.destroyPipelineLayout(pipelineLayout);
    device.destroyDescriptorSetLayout(descriptorSetLayout);
    device.destroyBuffer(inputBuffer);
    device.freeMemory(inputBufferMemory);
}