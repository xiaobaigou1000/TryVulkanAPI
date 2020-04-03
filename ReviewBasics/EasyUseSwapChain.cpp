#include "EasyUseSwapChain.h"
#include<algorithm>

void EasyUseSwapChain::init(const VulkanContext& context,const vk::Device device, vk::Extent2D ideaExtent)
{
    this->context = &context;
    this->device = device;

    querySwapChainSupportDetails();
    selectSwapchainBasicInfo();
    createSwapchain(ideaExtent);
}

void EasyUseSwapChain::destroy()
{
    device.destroySwapchainKHR(swapChain);
}

void EasyUseSwapChain::querySwapChainSupportDetails()
{
    vk::PhysicalDevice physicalDevice = context->getPhysicalDeviceHandle();
    vk::SurfaceKHR surface = context->getSurface();

    surfaceCapabilities = physicalDevice.getSurfaceCapabilitiesKHR(surface);
    availableSurfaceFormats = physicalDevice.getSurfaceFormatsKHR(surface);
    availablePresentModes = physicalDevice.getSurfacePresentModesKHR(surface);
}

void EasyUseSwapChain::selectSwapchainBasicInfo()
{
    auto surfaceFormatPtr = std::find_if(availableSurfaceFormats.begin(), availableSurfaceFormats.end(),
        [](auto i) {return i.format == vk::Format::eB8G8R8A8Unorm && i.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear; });
    if (surfaceFormatPtr == availableSurfaceFormats.end())
    {
        throw std::runtime_error("no available surface format");
    }
    surfaceFormat = *surfaceFormatPtr;

    auto presentModePtr = std::find(availablePresentModes.begin(), availablePresentModes.end(), vk::PresentModeKHR::eMailbox);
    if (presentModePtr == availablePresentModes.end())
    {
        presentMode = vk::PresentModeKHR::eFifo;
    }
    presentMode = *presentModePtr;
}

void EasyUseSwapChain::createSwapchain(vk::Extent2D ideaExtent)
{
    swapChainExtent = checkExtent(ideaExtent);
    uint32_t imageCount = surfaceCapabilities.minImageCount + 1;
    if (surfaceCapabilities.maxImageCount > 0 && imageCount > surfaceCapabilities.maxImageCount)
    {
        imageCount = surfaceCapabilities.maxImageCount;
    }

    uint32_t queueFamilyIndex = context->getQueueFamilyIndex();
    vk::SwapchainCreateInfoKHR createInfo(
        {}, context->getSurface(), imageCount,
        surfaceFormat.format, surfaceFormat.colorSpace,
        swapChainExtent, 1, vk::ImageUsageFlagBits::eColorAttachment,
        vk::SharingMode::eExclusive, 1, &queueFamilyIndex,
        vk::SurfaceTransformFlagBitsKHR::eIdentity, vk::CompositeAlphaFlagBitsKHR::eOpaque, presentMode, VK_TRUE);
    swapChain = device.createSwapchainKHR(createInfo);
}

vk::Extent2D EasyUseSwapChain::checkExtent(vk::Extent2D ideaExtent)
{
    if (surfaceCapabilities.currentExtent == ideaExtent && surfaceCapabilities.currentExtent.width != 0xFFFFFFFF)
    {
        return ideaExtent;
    }
    else
    {
        vk::Extent2D actualExtent;
        actualExtent.width = std::max<uint32_t>(surfaceCapabilities.minImageExtent.width, std::min<uint32_t>(surfaceCapabilities.maxImageExtent.width, ideaExtent.width));
        actualExtent.height = std::max<uint32_t>(surfaceCapabilities.minImageExtent.height, std::min<uint32_t>(surfaceCapabilities.maxImageExtent.height, ideaExtent.height));
        return actualExtent;
    }
}
