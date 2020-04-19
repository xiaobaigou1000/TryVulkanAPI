#include"VulkanApp.h"
#include<limits>
#include<chrono>
#include<opencv2/core.hpp>
#include<opencv2/imgcodecs.hpp>
#include<opencv2/imgproc.hpp>
#include<iostream>

void VulkanApp::run()
{
    mainLoop();
}

void VulkanApp::init()
{
    window.init();
    context.init(window);
    vk::PhysicalDeviceFeatures physicalDeviceFeature;
    physicalDeviceFeature.setSamplerAnisotropy(VK_TRUE);
    device = context.createLogicalDevice({ VK_KHR_SWAPCHAIN_EXTENSION_NAME }, physicalDeviceFeature);
    graphicsQueue = device.getQueue(context.getQueueFamilyIndex(), 0);
    swapChain.init(context, device, window.extent());
    swapChainHandle = swapChain.handle();
    std::tie(swapChainImages, swapChainImageViews) = swapChain.getSwapChainImages();
    vk::CommandPoolCreateInfo commandPoolInfo({}, context.getQueueFamilyIndex());
    commandPool = device.createCommandPool(commandPoolInfo);

    createDepthResources();

    userInit();
}

void VulkanApp::cleanup()
{
    userDestroy();
    for (auto i : depthImages)
    {
        device.destroyImage(i);
    }

    for (auto i : depthImageViews)
    {
        device.destroyImageView(i);
    }

    for (auto i : depthImageMemorys)
    {
        device.freeMemory(i);
    }

    device.destroyCommandPool(commandPool);
    swapChain.destroy();
    device.destroy();
    context.destroy();
    window.destroy();
}

std::tuple<vk::Buffer, vk::DeviceMemory> VulkanApp::createBuffer(vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties)
{
    uint32_t queueFamilyIndex = context.getQueueFamilyIndex();
    vk::BufferCreateInfo bufferCreateInfo({}, size, usage, vk::SharingMode::eExclusive, 1, &queueFamilyIndex);
    vk::Buffer buffer = device.createBuffer(bufferCreateInfo);
    vk::MemoryRequirements memoryRequirements = device.getBufferMemoryRequirements(buffer);
    vk::MemoryAllocateInfo memoryAllocInfo(memoryRequirements.size, context.findMemoryType(memoryRequirements.memoryTypeBits, properties));
    vk::DeviceMemory memory = device.allocateMemory(memoryAllocInfo);
    device.bindBufferMemory(buffer, memory, 0);
    return { buffer,memory };
}

std::tuple<vk::Buffer, vk::DeviceMemory, vk::DeviceSize> VulkanApp::createBufferForArrayObjects(vk::DeviceSize singleObjectSize, vk::DeviceSize numOfObjects, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties)
{
    auto minOffset = context.getPhysicalDeviceProperties().limits.minUniformBufferOffsetAlignment;
    vk::DeviceSize offset = (singleObjectSize / minOffset + 1) * minOffset;
    vk::DeviceSize wholeBufferSize = offset * numOfObjects;
    vk::Buffer buffer;
    vk::DeviceMemory memory;
    std::tie(buffer, memory) = createBuffer(wholeBufferSize, usage, properties);
    return { buffer,memory,offset };
}

void VulkanApp::copyBuffer(vk::Buffer src, vk::Buffer dst, vk::DeviceSize size)
{
    vk::CommandBuffer commandBuffer = beginSingleTimeCommand();
    vk::BufferCopy copyRegion(0, 0, size);
    commandBuffer.copyBuffer(src, dst, copyRegion);
    endSingleTimeCommand(commandBuffer);
}

vk::CommandBuffer VulkanApp::beginSingleTimeCommand()
{
    vk::CommandBufferAllocateInfo allocInfo(commandPool, vk::CommandBufferLevel::ePrimary, 1);
    vk::CommandBuffer commandBuffer = device.allocateCommandBuffers(allocInfo).front();
    vk::CommandBufferBeginInfo beginInfo({}, nullptr);
    commandBuffer.begin(beginInfo);
    return commandBuffer;
}

void VulkanApp::endSingleTimeCommand(vk::CommandBuffer command)
{
    command.end();
    vk::SubmitInfo submitInfo(0, nullptr, nullptr, 1, &command, 0, nullptr);
    graphicsQueue.submit(submitInfo, {});
    graphicsQueue.waitIdle();
    device.freeCommandBuffers(commandPool, command);
}

void VulkanApp::transitionImageLayout(vk::Image image, vk::Format format, vk::ImageLayout oldLayout, vk::ImageLayout newLayout)
{
    vk::CommandBuffer command = beginSingleTimeCommand();
    vk::ImageSubresourceRange subRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1);
    vk::ImageMemoryBarrier barrier;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    barrier.subresourceRange = subRange;
    vk::PipelineStageFlags sourceStage;
    vk::PipelineStageFlags destinationStage;
    if (oldLayout == vk::ImageLayout::eUndefined && newLayout == vk::ImageLayout::eTransferDstOptimal)
    {
        barrier.srcAccessMask = {};
        barrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;
        sourceStage = vk::PipelineStageFlagBits::eTopOfPipe;
        destinationStage = vk::PipelineStageFlagBits::eTransfer;
    }
    else if (oldLayout == vk::ImageLayout::eTransferDstOptimal && newLayout == vk::ImageLayout::eShaderReadOnlyOptimal)
    {
        barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
        barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;
        sourceStage = vk::PipelineStageFlagBits::eTransfer;
        destinationStage = vk::PipelineStageFlagBits::eFragmentShader;
    }
    else if (oldLayout == vk::ImageLayout::eUndefined && newLayout == vk::ImageLayout::eDepthStencilAttachmentOptimal)
    {
        barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil;
        barrier.srcAccessMask = {};
        barrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;
        sourceStage = vk::PipelineStageFlagBits::eTopOfPipe;
        destinationStage = vk::PipelineStageFlagBits::eTransfer;
    }
    else
    {
        throw std::invalid_argument("unsupported layout transition!");
    }
    command.pipelineBarrier(sourceStage, destinationStage, {}, {}, {}, barrier);
    endSingleTimeCommand(command);
}

void VulkanApp::copyBufferToImage(vk::Buffer buffer, vk::Image image, uint32_t width, uint32_t height)
{
    vk::CommandBuffer command = beginSingleTimeCommand();
    vk::ImageSubresourceLayers imageSubresource(vk::ImageAspectFlagBits::eColor, 0, 0, 1);
    vk::BufferImageCopy region(
        0,
        0,
        0,
        imageSubresource,
        { 0,0,0 },
        { width,height,1 });
    command.copyBufferToImage(buffer, image, vk::ImageLayout::eTransferDstOptimal, region);
    endSingleTimeCommand(command);
}

void VulkanApp::createDepthResources()
{
    depthImageFormat = context.findSupportedFormat(
        { vk::Format::eD24UnormS8Uint,vk::Format::eD32SfloatS8Uint,vk::Format::eD32Sfloat },
        vk::ImageTiling::eOptimal,
        vk::FormatFeatureFlagBits::eDepthStencilAttachment);

    //create depth buffer resource
    uint32_t queueFamilyIndex = context.getQueueFamilyIndex();

    vk::ImageCreateInfo depthImageCreateInfo(
        {},
        vk::ImageType::e2D,
        depthImageFormat,
        vk::Extent3D{ swapChain.extent().width,swapChain.extent().height,1 },
        1, 1, vk::SampleCountFlagBits::e1,
        vk::ImageTiling::eOptimal,
        vk::ImageUsageFlagBits::eDepthStencilAttachment,
        vk::SharingMode::eExclusive,
        1,
        &queueFamilyIndex,
        vk::ImageLayout::eUndefined);

    depthImages.resize(swapChainImages.size());
    depthImageViews.resize(swapChainImages.size());
    depthImageMemorys.resize(swapChainImages.size());

    for (uint32_t i = 0; i < depthImages.size(); i++)
    {
        depthImages[i] = device.createImage(depthImageCreateInfo);
        vk::ImageViewCreateInfo depthImageViewInfo(
            {},
            depthImages[i],
            vk::ImageViewType::e2D,
            depthImageFormat,
            {},
            { vk::ImageAspectFlagBits::eDepth,0,1,0,1 });
        depthImageMemorys[i] = allocateImageMemory(depthImages[i]);
        depthImageViews[i] = device.createImageView(depthImageViewInfo);
        transitionImageLayout(depthImages[i], depthImageFormat, vk::ImageLayout::eUndefined, vk::ImageLayout::eDepthStencilAttachmentOptimal);
    }
}

vk::DeviceMemory VulkanApp::allocateImageMemory(vk::Image image)
{
    vk::MemoryRequirements memoryRequirement = device.getImageMemoryRequirements(image);
    vk::MemoryAllocateInfo memoryAllocateInfo(
        memoryRequirement.size,
        context.findMemoryType(memoryRequirement.memoryTypeBits, vk::MemoryPropertyFlagBits::eDeviceLocal));
    vk::DeviceMemory memory = device.allocateMemory(memoryAllocateInfo);
    device.bindImageMemory(image, memory, 0);
    return memory;
}

void VulkanApp::mainLoop()
{
    while (!window.shouldClose())
    {
        window.pollEvents();
        userLoopFunc();
    }
    graphicsQueue.waitIdle();
}

void VulkanApp::userInit()
{
    std::cout << "vulkan app user init\n";
    std::cout << "this is and default init function, do you forget to override it?\n";
}

void VulkanApp::userLoopFunc()
{
    std::cout << "vulkan app user loop\n";
    std::cout << "this is and default loop function, do you forget to override it?\n";
}

void VulkanApp::userDestroy()
{
    std::cout << "vulkan app user destroy\n";
    std::cout << "this is and default destroy function, do you forget to override it?\n";
}
