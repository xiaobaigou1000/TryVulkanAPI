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
    createSwapChain();
    createImageViews();
    createRenderPass();
    createGraphicsPipeline();
    createFrameBuffers();
    createCommandPool();
    createCommandBuffers();
    createSyncObjects();
}

void HelloTriangleApplication::mainLoop()
{
    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
        drawFrame();
    }
    device.waitIdle();
}

void HelloTriangleApplication::cleanup()
{
    for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        device.destroySemaphore(imageAvailableSemaphore[i]);
        device.destroySemaphore(renderFinishedSemaphore[i]);
        device.destroyFence(inFlightFences[i]);
    }

    device.destroyCommandPool(commandPool);
    for (const auto& i : swapChainFrameBuffers)
    {
        device.destroyFramebuffer(i);
    }
    device.destroyPipeline(graphicsPipeline);
    device.destroyPipelineLayout(pipelineLayout);
    device.destroyRenderPass(renderPass);
    for (const auto& i : swapChainImageViews)
    {
        device.destroyImageView(i);
    }
    device.destroySwapchainKHR(swapChain);
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

void HelloTriangleApplication::checkPhysicalDeviceExtensionSupport()
{
    auto availableExtensions = physicalDevice.enumerateDeviceExtensionProperties();
    for (const auto& i : physicalDeviceExtensions)
    {
        bool checked = false;
        for (const auto& j : availableExtensions)
        {
            if (std::string(j.extensionName) == std::string(i))
            {
                checked = true;
            }
        }
        if (!checked)
        {
            throw std::runtime_error("physical device extension check failed :" + std::string(i));
        }
    }
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

    checkPhysicalDeviceExtensionSupport();
    auto swapChainSupportDetails = querySwapChainSupport();
    if (swapChainSupportDetails.formats.empty() || swapChainSupportDetails.presentModes.empty())
    {
        throw std::runtime_error("swap chain support inadequate.");
    }

    vk::DeviceCreateInfo createInfo({}, 2, queueCreateInfos.data(), 0, nullptr,
        static_cast<uint32_t>(physicalDeviceExtensions.size()), physicalDeviceExtensions.data());
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

void HelloTriangleApplication::createSwapChain()
{
    auto swapChainSupport = querySwapChainSupport();
    vk::SurfaceFormatKHR surfaceFormat = chooseSurfaceFormat(swapChainSupport.formats);
    vk::PresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
    vk::Extent2D extent = chooseSwapExtent(swapChainSupport.capabilities);
    uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
    if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount)
    {
        imageCount = swapChainSupport.capabilities.maxImageCount;
    }

    vk::SwapchainCreateInfoKHR createInfo({}, surface, imageCount, surfaceFormat.format, surfaceFormat.colorSpace, extent, 1, vk::ImageUsageFlagBits::eColorAttachment);
    QueueFamilyIndices indices = findQueueFamilies();
    uint32_t queueFamilyIndices[] = { indices.graphicsFamily,indices.presentFamily };
    if (indices.graphicsFamily != indices.presentFamily)
    {
        createInfo.imageSharingMode = vk::SharingMode::eConcurrent;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    }
    else
    {
        createInfo.imageSharingMode = vk::SharingMode::eExclusive;
        createInfo.queueFamilyIndexCount = 0;
        createInfo.pQueueFamilyIndices = nullptr;
    }

    createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
    createInfo.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;

    swapChain = device.createSwapchainKHR(createInfo);
    swapChainImages = device.getSwapchainImagesKHR(swapChain);
    swapChainImageFormat = surfaceFormat.format;
    swapChainExtent = extent;
}

void HelloTriangleApplication::createImageViews()
{
    swapChainImageViews.resize(swapChainImages.size());
    for (uint32_t i = 0; i < swapChainImageViews.size(); ++i)
    {
        vk::ImageSubresourceRange subresourceRange;
        vk::ImageViewCreateInfo createInfo({}, swapChainImages[i], vk::ImageViewType::e2D, swapChainImageFormat,
            { vk::ComponentSwizzle::eIdentity,vk::ComponentSwizzle::eIdentity ,vk::ComponentSwizzle::eIdentity ,vk::ComponentSwizzle::eIdentity },
            { vk::ImageAspectFlagBits::eColor,0,1,0,1 });
        swapChainImageViews[i] = device.createImageView(createInfo);
    }
}

void HelloTriangleApplication::createRenderPass()
{
    vk::AttachmentDescription colorAttachment(
        {}, swapChainImageFormat, vk::SampleCountFlagBits::e1,
        vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eStore,
        vk::AttachmentLoadOp::eDontCare, vk::AttachmentStoreOp::eDontCare,
        vk::ImageLayout::eUndefined, vk::ImageLayout::ePresentSrcKHR);
    vk::AttachmentReference colorAttachmentRef(0, vk::ImageLayout::eColorAttachmentOptimal);
    vk::SubpassDescription subpass({}, vk::PipelineBindPoint::eGraphics);
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;

    vk::SubpassDependency dependency(
        VK_SUBPASS_EXTERNAL, 0,
        vk::PipelineStageFlagBits::eColorAttachmentOutput, vk::PipelineStageFlagBits::eColorAttachmentOutput,
        {}, vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite,
        vk::DependencyFlagBits::eByRegion);

    vk::RenderPassCreateInfo renderPassInfo({}, 1, &colorAttachment, 1, &subpass, 1, &dependency);
    renderPass = device.createRenderPass(renderPassInfo);
}

void HelloTriangleApplication::createGraphicsPipeline()
{
    auto vertexShaderCode = readShaderCode("./shaders/triangleWithAttribVert.spv");
    auto fragmentShaderCode = readShaderCode("./shaders/triangleWithAttribFrag.spv");
    vk::ShaderModule vertexShaderModule = createShaderModule(vertexShaderCode);
    vk::ShaderModule fragmentShaderModule = createShaderModule(fragmentShaderCode);

    vk::PipelineShaderStageCreateInfo vertexShaderStageCreateInfo({}, vk::ShaderStageFlagBits::eVertex, vertexShaderModule, "main");
    vk::PipelineShaderStageCreateInfo fragmentShaderStageCreateInfo({}, vk::ShaderStageFlagBits::eFragment, fragmentShaderModule, "main");
    vk::PipelineShaderStageCreateInfo shaderStageCreateInfos[] = { vertexShaderStageCreateInfo,fragmentShaderStageCreateInfo };

    auto bindingDescription = Vertex::getBindingDescription();
    auto attributeDescription = Vertex::getAttributeDescriptions();

    vk::PipelineVertexInputStateCreateInfo vertexInputInfo({}, 1, &bindingDescription, 2, attributeDescription.data());
    vk::PipelineInputAssemblyStateCreateInfo inputAssembly({}, vk::PrimitiveTopology::eTriangleList, VK_FALSE);
    vk::Viewport viewport(0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height), 0.0f, 1.0f);
    vk::Rect2D scissor({ 0,0 }, swapChainExtent);
    vk::PipelineViewportStateCreateInfo viewportState({}, 1, &viewport, 1, &scissor);
    vk::PipelineRasterizationStateCreateInfo rasterizer(
        {}, VK_FALSE, VK_FALSE, vk::PolygonMode::eFill, vk::CullModeFlagBits::eBack,
        vk::FrontFace::eClockwise, VK_FALSE, 0.0f, 0.0f, 0.0f, 1.0f);
    vk::PipelineMultisampleStateCreateInfo multisampling({}, vk::SampleCountFlagBits::e1, VK_FALSE, 1.0f, nullptr, VK_FALSE, VK_FALSE);
    vk::PipelineColorBlendAttachmentState colorBlendAttachment(
        VK_FALSE, vk::BlendFactor::eOne, vk::BlendFactor::eZero,
        vk::BlendOp::eAdd, vk::BlendFactor::eOne, vk::BlendFactor::eZero, vk::BlendOp::eAdd,
        vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
        vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA);
    vk::PipelineColorBlendStateCreateInfo colorBlending({}, VK_FALSE, vk::LogicOp::eClear, 1, &colorBlendAttachment, { 0.0f,0.0f,0.0f,0.0f });
    vk::DynamicState dynamicStates[] = {
        vk::DynamicState::eViewport,
        vk::DynamicState::eLineWidth
    };
    vk::PipelineDynamicStateCreateInfo dynamicState({}, 2, dynamicStates);
    vk::PipelineLayoutCreateInfo pipelineLayoutInfo({}, 0, nullptr, 0, nullptr);
    pipelineLayout = device.createPipelineLayout(pipelineLayoutInfo);
    vk::GraphicsPipelineCreateInfo createInfo(
        {}, 2, shaderStageCreateInfos,
        &vertexInputInfo, &inputAssembly, nullptr,
        &viewportState, &rasterizer, &multisampling,
        nullptr, &colorBlending, &dynamicState,
        pipelineLayout, renderPass, 0);
    graphicsPipeline = device.createGraphicsPipeline({}, createInfo);

    device.destroyShaderModule(vertexShaderModule);
    device.destroyShaderModule(fragmentShaderModule);
}

void HelloTriangleApplication::createFrameBuffers()
{
    swapChainFrameBuffers.resize(swapChainImageViews.size());
    for (uint32_t i = 0; i < swapChainImageViews.size(); i++)
    {
        vk::ImageView* attachments = &swapChainImageViews[i];
        vk::FramebufferCreateInfo framebufferInfo({}, renderPass, 1, attachments, swapChainExtent.width, swapChainExtent.height, 1);
        swapChainFrameBuffers[i] = device.createFramebuffer(framebufferInfo);
    }
}

void HelloTriangleApplication::createCommandPool()
{
    auto queueFamilyIndices = findQueueFamilies();
    vk::CommandPoolCreateInfo poolInfo({}, queueFamilyIndices.graphicsFamily);
    commandPool = device.createCommandPool(poolInfo);
}

void HelloTriangleApplication::createCommandBuffers()
{
    commandBuffers.resize(swapChainFrameBuffers.size());
    vk::CommandBufferAllocateInfo allocInfo(commandPool, vk::CommandBufferLevel::ePrimary, commandBuffers.size());
    commandBuffers = device.allocateCommandBuffers(allocInfo);

    for (uint32_t i = 0; i < commandBuffers.size(); i++)
    {
        vk::CommandBufferBeginInfo beginInfo({}, nullptr);
        commandBuffers[i].begin(beginInfo);
        vk::ClearValue clearColor(std::array<float, 4>{0.0f, 0.0f, 0.0f, 1.0f});
        vk::RenderPassBeginInfo renderPassInfo(renderPass, swapChainFrameBuffers[i], { {0,0},swapChainExtent }, 1, &clearColor);
        commandBuffers[i].beginRenderPass(renderPassInfo, vk::SubpassContents::eInline);
        commandBuffers[i].bindPipeline(vk::PipelineBindPoint::eGraphics, graphicsPipeline);
        commandBuffers[i].setViewport(0, { {0.0f,0.0f,static_cast<float>(width),static_cast<float>(height),0.0f,1.0f} });
        commandBuffers[i].draw(3, 1, 0, 0);
        commandBuffers[i].endRenderPass();
        commandBuffers[i].end();
    }
}

void HelloTriangleApplication::drawFrame()
{
    device.waitForFences({ inFlightFences[currentFrame] },VK_TRUE,0xFFFFFFFF);
    device.resetFences({ inFlightFences[currentFrame] });

    auto imageIndex = device.acquireNextImageKHR(swapChain, 0xFFFFFFFF, imageAvailableSemaphore[currentFrame], {});
    vk::PipelineStageFlags waitStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
    vk::SubmitInfo submitInfo(1, &imageAvailableSemaphore[currentFrame], &waitStageMask, 1, &commandBuffers[imageIndex.value], 1, &renderFinishedSemaphore[currentFrame]);
    graphicsQueue.submit({ submitInfo }, inFlightFences[currentFrame]);

    vk::PresentInfoKHR presentInfo(1,&renderFinishedSemaphore[currentFrame],1,&swapChain,&imageIndex.value,nullptr);
    presentQueue.presentKHR(presentInfo);
    currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void HelloTriangleApplication::createSyncObjects()
{
    vk::SemaphoreCreateInfo semaphoreInfo;
    vk::FenceCreateInfo fenceInfo(vk::FenceCreateFlagBits::eSignaled);
    for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        imageAvailableSemaphore.push_back(device.createSemaphore(semaphoreInfo));
        renderFinishedSemaphore.push_back(device.createSemaphore(semaphoreInfo));
        inFlightFences.push_back(device.createFence(fenceInfo));
    }
}

std::vector<char> HelloTriangleApplication::readShaderCode(const std::string& fileName)
{
    std::ifstream file(fileName, std::ios::ate | std::ios::binary);
    if (!file.is_open())
    {
        throw std::runtime_error("open file failed.");
    }
    size_t fileSize = static_cast<size_t>(file.tellg());
    std::vector<char> buffer;
    buffer.resize(fileSize);
    file.seekg(std::ios::beg);
    file.read(buffer.data(), fileSize);
    file.close();
    return buffer;
}

vk::SurfaceFormatKHR HelloTriangleApplication::chooseSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& availableFormats)
{
    for (const auto i : availableFormats)
    {
        if (i.format == vk::Format::eB8G8R8A8Unorm && i.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear)
        {
            return i;
        }
    }
    return availableFormats[0];
}

vk::PresentModeKHR HelloTriangleApplication::chooseSwapPresentMode(const std::vector<vk::PresentModeKHR>& availablePresentModes)
{
    for (const auto i : availablePresentModes)
    {
        if (i == vk::PresentModeKHR::eMailbox)
        {
            return i;
        }
    }
    return vk::PresentModeKHR::eFifo;
}

vk::ShaderModule HelloTriangleApplication::createShaderModule(const std::vector<char>& code)
{
    vk::ShaderModuleCreateInfo createInfo({}, code.size(), reinterpret_cast<const uint32_t*>(code.data()));
    return device.createShaderModule(createInfo);
}

vk::Extent2D HelloTriangleApplication::chooseSwapExtent(const vk::SurfaceCapabilitiesKHR& capabilities)
{
    if (capabilities.currentExtent.width != 0xFFFFFFFFui32)
    {
        return capabilities.currentExtent;
    }
    else
    {
        vk::Extent2D actualExtent = { width,height };
        actualExtent.width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, actualExtent.width));
        actualExtent.height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, actualExtent.height));
        return actualExtent;
    }
}

HelloTriangleApplication::SwapChainSupportDetails HelloTriangleApplication::querySwapChainSupport()
{
    SwapChainSupportDetails details;
    details.capabilities = physicalDevice.getSurfaceCapabilitiesKHR(surface);
    std::cout << "window capabilities : " << vk::to_string(details.capabilities.supportedUsageFlags) << '\n';
    details.formats = physicalDevice.getSurfaceFormatsKHR(surface);
    details.presentModes = physicalDevice.getSurfacePresentModesKHR(surface);
    return details;
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
        if (presentFamily == -1 && physicalDevice.getSurfaceSupportKHR(i, surface) && graphicsFamily != i)
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
