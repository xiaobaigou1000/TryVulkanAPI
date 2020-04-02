#include"VulkanApp.h"

void VulkanApp::run()
{
    mainLoop();
}

void VulkanApp::init()
{
    window.init();
    context.createInstance(window);
    context.setupDebugMessenger();
}

void VulkanApp::cleanup()
{
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
