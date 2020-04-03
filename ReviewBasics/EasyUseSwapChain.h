#pragma once
#include<vulkan/vulkan.hpp>
#include"VulkanContext.h"

class EasyUseSwapChain
{
public:
    EasyUseSwapChain() :presentMode(vk::PresentModeKHR::eFifo), context(nullptr) {}
    EasyUseSwapChain(const EasyUseSwapChain&) = delete;
    EasyUseSwapChain& operator=(const EasyUseSwapChain&) = delete;
    EasyUseSwapChain(EasyUseSwapChain&&) = delete;

    inline vk::SwapchainKHR handle() { return swapChain; }
    inline vk::Format imageFormat() { return surfaceFormat.format; }
    inline vk::Extent2D extent() { return swapChainExtent; }
    void init(const VulkanContext& context,const vk::Device device, vk::Extent2D ideaExtent);
    void destroy();
private:
    vk::Device device;
    vk::SurfaceCapabilitiesKHR surfaceCapabilities;
    std::vector<vk::SurfaceFormatKHR> availableSurfaceFormats;
    std::vector<vk::PresentModeKHR> availablePresentModes;

    vk::SurfaceFormatKHR surfaceFormat;
    vk::PresentModeKHR presentMode;
    vk::Extent2D swapChainExtent;
    vk::SwapchainKHR swapChain;
    const VulkanContext* context;

    void querySwapChainSupportDetails();
    void selectSwapchainBasicInfo();
    void createSwapchain(vk::Extent2D ideaExtent);
    vk::Extent2D checkExtent(vk::Extent2D ideaExtent);
};

