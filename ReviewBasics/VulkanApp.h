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
    vk::SwapchainKHR swapChainHandle;
    std::vector<vk::Image> swapChainImages;
    std::vector<vk::ImageView> swapChainImageViews;
    vk::Device device;
    vk::Queue graphicsQueue;//be able to present image

    //code here
    SimpleShaderPipeline shader;
    std::vector<vk::Framebuffer> swapChainColorOnlyFramebuffers;
    vk::CommandPool commandPool;
    std::vector<vk::CommandBuffer> commandBuffers;
    std::vector<vk::Semaphore> imageAvailableSemaphore;
    std::vector<vk::Semaphore> renderFinishedSemaphore;
    std::vector<vk::Fence> inFlightFences;
    uint32_t max_images_in_flight = 0;
    uint32_t current_frame = 0;

    void mainLoop();
};