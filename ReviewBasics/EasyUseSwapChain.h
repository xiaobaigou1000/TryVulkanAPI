#pragma once
#include<vulkan/vulkan.hpp>

class EasyUseSwapChain
{
public:
    void init(const vk::Device device, const vk::PhysicalDevice physicalDevice);

private:
    void querySwapChainSupportDetails(const vk::PhysicalDevice physicalDevice);
    vk::Device device;
};

