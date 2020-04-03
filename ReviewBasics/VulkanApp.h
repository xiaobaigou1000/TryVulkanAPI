#pragma once
#include"NativeWindow.h"
#include"VulkanContext.h"
#include"EasyUseSwapChain.h"

class VulkanApp
{
public:
    void run();
    void init();
    void cleanup();

private:
    NativeWindow window;
    VulkanContext context;
    EasyUseSwapChain swapChain;

    vk::Device device;
    vk::Queue graphicsQueue;//be able to present image

    void mainLoop();
};