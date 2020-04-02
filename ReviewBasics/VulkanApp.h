#pragma once
#include"NativeWindow.h"
#include"VulkanContext.h"

class VulkanApp
{
public:
    void run();
    void init();
    void cleanup();

private:
    NativeWindow window;
    VulkanContext context;

    void mainLoop();
};