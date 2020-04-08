#include"VulkanApp.h"

void VulkanApp::run()
{
    mainLoop();
}

void VulkanApp::init()
{
    window.init();
    context.init(window);
    device = context.createLogicalDevice({ VK_KHR_SWAPCHAIN_EXTENSION_NAME }, {});
    graphicsQueue = device.getQueue(context.getQueueFamilyIndex(), 0);
    swapChain.init(context, device, window.extent());
    std::tie(swapChainImages, swapChainImageViews) = swapChain.getSwapChainImages();

    userInit();
}

void VulkanApp::cleanup()
{
    userDestroy();
    swapChain.destroy();
    device.destroy();
    context.destroy();
    window.destroy();
}

void VulkanApp::userInit()
{
    //code here

    //create shader pipeline
    shader.init(device, swapChain.extent(), swapChain.imageFormat());
    vk::PipelineVertexInputStateCreateInfo vertexInputInfo({}, 0, nullptr, 0, nullptr);
    vk::PipelineLayoutCreateInfo pipelineLayoutInfo({}, 0, nullptr, 0, nullptr);
    shader.createColorOnlyRenderPass();
    shader.createDefaultVFShader("./shaders/simpleTriangleVert.spv", "./shaders/simpleTriangleFrag.spv", vertexInputInfo, pipelineLayoutInfo);

    //create framebuffers
    swapChainColorOnlyFramebuffers.resize(swapChainImageViews.size());
    for (uint32_t i = 0; i < swapChainImageViews.size(); ++i)
    {
        vk::FramebufferCreateInfo framebufferInfo({}, shader.getRenderPass(), 1, &swapChainImageViews[i], swapChain.extent().width, swapChain.extent().height, 1);
        swapChainColorOnlyFramebuffers[i] = device.createFramebuffer(framebufferInfo);
    }

    //create command pool and allocate command buffers
    vk::CommandPoolCreateInfo commandPoolInfo({}, context.getQueueFamilyIndex());
    commandPool = device.createCommandPool(commandPoolInfo);
    vk::CommandBufferAllocateInfo allocInfo(commandPool,
        vk::CommandBufferLevel::ePrimary, static_cast<uint32_t>(swapChainColorOnlyFramebuffers.size()));
    commandBuffers = device.allocateCommandBuffers(allocInfo);

    //record command buffers
}

void VulkanApp::userLoopFunc()
{
    //code here
}

void VulkanApp::userDestroy()
{
    //code here
    device.destroyCommandPool(commandPool);
    for (const auto i : swapChainColorOnlyFramebuffers)
    {
        device.destroyFramebuffer(i);
    }
    shader.destroy();
}

void VulkanApp::mainLoop()
{
    while (!window.shouldClose())
    {
        window.pollEvents();
        userLoopFunc();
    }
}
