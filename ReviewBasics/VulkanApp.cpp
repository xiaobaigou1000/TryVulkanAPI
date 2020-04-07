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
    shader.init(device, swapChain.extent(), swapChain.imageFormat());
    vk::PipelineVertexInputStateCreateInfo vertexInputInfo({}, 0, nullptr, 0, nullptr);
    vk::PipelineLayoutCreateInfo pipelineLayoutInfo({}, 0, nullptr, 0, nullptr);
    shader.createColorOnlyRenderPass();
    shader.createDefaultVFShader("./shaders/simpleTriangleVert.spv", "./shaders/simpleTriangleFrag.spv", vertexInputInfo, pipelineLayoutInfo);

}

void VulkanApp::userLoopFunc()
{
    //code here
}

void VulkanApp::userDestroy()
{
    //code here
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
