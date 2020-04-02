#include"VulkanApp.h"

void VulkanApp::run()
{
    mainLoop();
}

void VulkanApp::init()
{
    window.init();
    context.init(window);
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
