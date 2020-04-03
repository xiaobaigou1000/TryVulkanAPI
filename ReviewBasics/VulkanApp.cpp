#include"VulkanApp.h"

void VulkanApp::run()
{
    mainLoop();
}

void VulkanApp::init()
{
    window.init();
    context.init(window);
    device = context.createLogicalDevice({VK_KHR_SWAPCHAIN_EXTENSION_NAME}, {});
    graphicsQueue = device.getQueue(context.getQueueFamilyIndex(), 0);
}

void VulkanApp::cleanup()
{
    device.destroy();
    context.destroy();
    window.destroy();
}

void VulkanApp::mainLoop()
{
    while (!window.shouldClose())
    {
        window.pollEvents();
    }
}
