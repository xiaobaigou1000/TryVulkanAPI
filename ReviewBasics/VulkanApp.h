#pragma once
#include"NativeWindow.h"
#include"VulkanContext.h"
#include"EasyUseSwapChain.h"
#include"SimpleShaderPipeline.h"

class VulkanApp
{
public:
    void run();
    void init();
    void cleanup();

private:
    void userInit();
    void userLoopFunc();
    void userDestroy();

    NativeWindow window;
    VulkanContext context;
    EasyUseSwapChain swapChain;
    std::vector<vk::Image> swapChainImages;
    std::vector<vk::ImageView> swapChainImageViews;
    vk::Device device;
    vk::Queue graphicsQueue;//be able to present image

    //code here
    SimpleShaderPipeline shader;
    std::vector<vk::Framebuffer> swapChainColorOnlyFramebuffers;
    vk::CommandPool commandPool;
    std::vector<vk::CommandBuffer> commandBuffers;

    void mainLoop();
};