#pragma once
#include"NativeWindow.h"
#include"VulkanContext.h"
#include"EasyUseSwapChain.h"
#include"SimpleShaderPipeline.h"
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include<glm.hpp>
#include<gtc/matrix_transform.hpp>

class VulkanApp
{
public:
    void run();
    void init();
    void cleanup();

protected:
    VulkanContext context;
    vk::SwapchainKHR swapChainHandle;
    NativeWindow window;
    EasyUseSwapChain swapChain;

    std::vector<vk::Image> swapChainImages;
    std::vector<vk::ImageView> swapChainImageViews;
    vk::Device device;
    vk::Queue graphicsQueue;//be able to present image
    vk::CommandPool commandPool;

    vk::Format depthImageFormat;
    std::vector<vk::Image> depthImages;
    std::vector<vk::DeviceMemory> depthImageMemorys;
    std::vector<vk::ImageView> depthImageViews;

    void createDepthResources();

    std::tuple<vk::Buffer, vk::DeviceMemory> createBuffer(vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties);
    std::tuple<vk::Buffer, vk::DeviceMemory, vk::DeviceSize> createBufferForArrayObjects(vk::DeviceSize singleObjectSize, vk::DeviceSize numOfObjects, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags proerties);
    void copyBuffer(vk::Buffer src, vk::Buffer dst, vk::DeviceSize size);
    vk::CommandBuffer beginSingleTimeCommand();
    void endSingleTimeCommand(vk::CommandBuffer command);
    void transitionImageLayout(vk::Image image, vk::Format format, vk::ImageLayout oldLayout, vk::ImageLayout newLayout);
    void copyBufferToImage(vk::Buffer buffer, vk::Image image, uint32_t width, uint32_t height);
    vk::DeviceMemory allocateImageMemory(vk::Image image);

private:
    void mainLoop();
    virtual void userInit();
    virtual void userLoopFunc();
    virtual void userDestroy();
};