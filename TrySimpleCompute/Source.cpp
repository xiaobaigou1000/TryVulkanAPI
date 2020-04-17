#include<iostream>
#include<vulkan/vulkan.hpp>
#include<fstream>
#include<iomanip>
#include<opencv2/core.hpp>
#include<armadillo>
#include<chrono>
#include<thread>

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
};

std::tuple<vk::Buffer, vk::DeviceMemory> createHostBuffer(const SimpleComputeContext& context, vk::Device device, vk::DeviceSize size, vk::BufferUsageFlags usage)
{
    vk::BufferCreateInfo bufferInfo(
        {},
        size,
        vk::BufferUsageFlagBits::eStorageBuffer,
        vk::SharingMode::eExclusive,
        1,
        &context.queueFamilyIndex);
    vk::Buffer buffer = device.createBuffer(bufferInfo);
    vk::MemoryRequirements requirements = device.getBufferMemoryRequirements(buffer);
    vk::MemoryAllocateInfo allocInfo(
        requirements.size,
        context.findMemoryType(
            requirements.memoryTypeBits,
            vk::MemoryPropertyFlagBits::eHostCoherent | vk::MemoryPropertyFlagBits::eHostVisible));
    vk::DeviceMemory memory = device.allocateMemory(allocInfo);
    device.bindBufferMemory(buffer, memory, 0);
    return { buffer,memory };
}

void destroyBufferAndFreeMemory(vk::Device device, vk::Buffer buffer, vk::DeviceMemory memory)
{
    device.destroyBuffer(buffer);
    device.freeMemory(memory);
}

int main()
{
    SimpleComputeContext context;
    vk::Device device = context.getDevice();
    vk::Queue queue = context.getQueue();
    vk::CommandPool commandPool = context.getCommandPool();

    arma::Mat<float> inputMat(2, 2);
    inputMat(0, 0) = 16;
    inputMat(0, 1) = 3;
    inputMat(1, 0) = 7;
    inputMat(1, 1) = -11;

    std::cout << "origin matrix A :\n" << inputMat << std::endl;

    float b[2];
    b[0] = 11;
    b[1] = 13;

    std::cout << "origin vector b :\n" << b[0] << '\n' << b[1] << '\n';

    float x0[2];
    x0[0] = 1.0;
    x0[1] = 1.0;

    std::cout << "choosen vector x0 :\n" << x0[0] << '\n' << x0[1] << '\n';

    arma::Mat<float> L = arma::trimatl(inputMat);
    for (int i = 0; i < L.n_rows; i++)
        L(i, i) = 0;
    arma::Mat<float> U = arma::trimatu(inputMat);
    for (int i = 0; i < U.n_rows; i++)
        U(i, i) = 0;
    arma::Mat<float> D = arma::diagmat(inputMat);
    arma::Mat<float> DLInv = (D + L).i();

    vk::Buffer dlInvBuffer;
    vk::DeviceMemory dlInvMemory;
    std::tie(dlInvBuffer, dlInvMemory) = createHostBuffer(
        context, device,
        DLInv.n_elem * sizeof(float),
        vk::BufferUsageFlagBits::eStorageBuffer);

    void* data = device.mapMemory(dlInvMemory, 0, VK_WHOLE_SIZE);
    memcpy(data, DLInv.memptr(), DLInv.n_elem * sizeof(float));
    device.unmapMemory(dlInvMemory);

    vk::Buffer uMat;
    vk::DeviceMemory uMatMemory;
    std::tie(uMat, uMatMemory) = createHostBuffer(
        context, device,
        U.n_elem * sizeof(float),
        vk::BufferUsageFlagBits::eStorageBuffer);
    data = device.mapMemory(uMatMemory, 0, VK_WHOLE_SIZE);
    memcpy(data, U.memptr(), U.n_elem * sizeof(float));
    device.unmapMemory(uMatMemory);

    vk::Buffer xkab;
    vk::DeviceMemory xkabMemory;
    std::tie(xkab, xkabMemory) = createHostBuffer(
        context, device,
        sizeof(x0) + sizeof(b),
        vk::BufferUsageFlagBits::eStorageBuffer);
    data = device.mapMemory(xkabMemory, 0, VK_WHOLE_SIZE);
    void* bPtr = (float*)data + sizeof(x0) / sizeof(float);
    memcpy(data, x0, sizeof(x0));
    memcpy(bPtr, b, sizeof(b));
    device.unmapMemory(xkabMemory);

    vk::Buffer resultBuffer;
    vk::DeviceMemory resultBuffermemory;
    std::tie(resultBuffer, resultBuffermemory) = createHostBuffer(
        context, device,
        sizeof(x0),
        vk::BufferUsageFlagBits::eStorageBuffer);

    vk::DescriptorSetLayoutBinding dlInvBinding(0, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute, nullptr);
    vk::DescriptorSetLayoutBinding utMatBinding(1, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute);
    vk::DescriptorSetLayoutBinding xkabBinding(2, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute);
    vk::DescriptorSetLayoutBinding resultBinding(3, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute);
    std::vector<vk::DescriptorSetLayoutBinding> uniformBindings{ dlInvBinding,utMatBinding,xkabBinding,resultBinding };
    vk::DescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo({}, uniformBindings.size(), uniformBindings.data());
    vk::DescriptorSetLayout descriptorSetLayout = device.createDescriptorSetLayout(descriptorSetLayoutCreateInfo);
    vk::PipelineLayoutCreateInfo pipelineLayoutCreateInfo({}, 1, &descriptorSetLayout, 0, nullptr);
    vk::PipelineLayout pipelineLayout = device.createPipelineLayout(pipelineLayoutCreateInfo);

    //load compute shader stage
    std::ifstream file("./Gauss-Seidel.spv", std::ios::binary);
    std::vector<char> fileStr{ std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>() };
    file.close();
    vk::ShaderModuleCreateInfo createInfo({}, fileStr.size(), reinterpret_cast<const uint32_t*>(fileStr.data()));
    vk::ShaderModule computeShaderModule = device.createShaderModule(createInfo);
    vk::PipelineShaderStageCreateInfo computeStageInfo({}, vk::ShaderStageFlagBits::eCompute, computeShaderModule, "main");

    vk::ComputePipelineCreateInfo computePipelineInfo({}, computeStageInfo, pipelineLayout);
    vk::Pipeline computePipeline = device.createComputePipeline({}, computePipelineInfo);
    device.destroyShaderModule(computeShaderModule);

    //create descriptor pool and update descriptor set
    vk::DescriptorPoolSize poolSize(vk::DescriptorType::eStorageBuffer, 4);
    vk::DescriptorPoolCreateInfo descriptorPoolInfo({}, 1, 1, &poolSize);
    vk::DescriptorPool descriptorPool = device.createDescriptorPool(descriptorPoolInfo);
    vk::DescriptorSetAllocateInfo descriptorSetAllocInfo(descriptorPool, 1, &descriptorSetLayout);
    vk::DescriptorSet descriptorSet = device.allocateDescriptorSets(descriptorSetAllocInfo).front();

    vk::DescriptorBufferInfo dlInvDescriptorInfo(dlInvBuffer, 0, VK_WHOLE_SIZE);
    vk::DescriptorBufferInfo uMatDescriptorInfo(uMat, 0, VK_WHOLE_SIZE);
    vk::DescriptorBufferInfo xkabDescriptorInfo(xkab, 0, VK_WHOLE_SIZE);
    vk::DescriptorBufferInfo resultDescriptorInfo(resultBuffer, 0, VK_WHOLE_SIZE);
    vk::WriteDescriptorSet dlInvWrite(
        descriptorSet,
        0,
        0,
        1,
        vk::DescriptorType::eStorageBuffer,
        nullptr,
        &dlInvDescriptorInfo,
        nullptr);

    vk::WriteDescriptorSet uMatWrite(
        descriptorSet,
        1,
        0,
        1,
        vk::DescriptorType::eStorageBuffer,
        nullptr,
        &uMatDescriptorInfo,
        nullptr);

    vk::WriteDescriptorSet xkabWrite(
        descriptorSet,
        2,
        0,
        1,
        vk::DescriptorType::eStorageBuffer,
        nullptr,
        &xkabDescriptorInfo,
        nullptr);

    vk::WriteDescriptorSet resultWrite(
        descriptorSet,
        3,
        0,
        1,
        vk::DescriptorType::eStorageBuffer,
        nullptr,
        &resultDescriptorInfo,
        nullptr);

    std::vector<vk::WriteDescriptorSet> descriptorWrites{ dlInvWrite,uMatWrite,xkabWrite,resultWrite };

    device.updateDescriptorSets(descriptorWrites, {});

    vk::CommandBufferAllocateInfo commandInfo(commandPool, vk::CommandBufferLevel::ePrimary, 1);
    vk::CommandBuffer command = device.allocateCommandBuffers(commandInfo).front();
    vk::CommandBufferBeginInfo commandBeginInfo({}, nullptr);
    command.begin(commandBeginInfo);
    command.bindPipeline(vk::PipelineBindPoint::eCompute, computePipeline);
    command.bindDescriptorSets(vk::PipelineBindPoint::eCompute, pipelineLayout, 0, descriptorSet, {});
    command.dispatch(sizeof(b) / sizeof(float), 1, 1);
    command.end();

    vk::Fence fence = device.createFence({});

    vk::SubmitInfo submitInfo(0, nullptr, nullptr, 1, &command, 0, nullptr);

    for (int i = 0; i < 5 * sizeof(x0) / sizeof(float); i++)
    {
        queue.submit(submitInfo, fence);
        device.waitForFences(fence, VK_TRUE, 0xFFFFFFFF);
        device.resetFences(fence);

        void* resultPtr = device.mapMemory(resultBuffermemory, 0, VK_WHOLE_SIZE);
        void* xkPtr = device.mapMemory(xkabMemory, 0, VK_WHOLE_SIZE);
        memcpy(xkPtr, resultPtr, sizeof(b));
        device.unmapMemory(resultBuffermemory);
        device.unmapMemory(xkabMemory);
    }
    queue.waitIdle();
    void* result = device.mapMemory(resultBuffermemory, 0, VK_WHOLE_SIZE);
    float trueResult[2];
    memcpy(trueResult, result, sizeof(b));

    std::cout << "result vector :\n" << trueResult[0] << '\n' << trueResult[1] << '\n';

    //clean up
    destroyBufferAndFreeMemory(device, dlInvBuffer, dlInvMemory);
    destroyBufferAndFreeMemory(device, uMat, uMatMemory);
    destroyBufferAndFreeMemory(device, xkab, xkabMemory);
    destroyBufferAndFreeMemory(device, resultBuffer, resultBuffermemory);
    device.destroyFence(fence);
    device.destroyPipeline(computePipeline);
    device.destroyDescriptorPool(descriptorPool);
    device.destroyPipelineLayout(pipelineLayout);
    device.destroyDescriptorSetLayout(descriptorSetLayout);
}