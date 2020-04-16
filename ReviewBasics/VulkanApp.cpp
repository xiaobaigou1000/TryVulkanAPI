#include"VulkanApp.h"
#include<limits>
#include<chrono>
#include<opencv2/core.hpp>
#include<opencv2/imgcodecs.hpp>
#include<opencv2/imgproc.hpp>

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
    swapChain.destroy();
    device.destroy();
    context.destroy();
    window.destroy();
}

void VulkanApp::userInit()
{
    //easy use functions

    //must be called after shader creation
    auto createFramebuffers = [this] {
        swapChainFramebuffers.resize(swapChainImageViews.size());
        for (uint32_t i = 0; i < swapChainImageViews.size(); ++i)
        {
            std::vector<vk::ImageView> framebufferAttachments{ swapChainImageViews[i],depthImageViews[i] };
            vk::FramebufferCreateInfo framebufferInfo(
                {},
                shader.getRenderPass(),
                static_cast<uint32_t>(framebufferAttachments.size()),
                framebufferAttachments.data(),
                swapChain.extent().width,
                swapChain.extent().height,
                1);
            swapChainFramebuffers[i] = device.createFramebuffer(framebufferInfo);
        }
    };

    auto allocateFrameBuffers = [this] {
        vk::CommandBufferAllocateInfo allocInfo(commandPool,
            vk::CommandBufferLevel::ePrimary, static_cast<uint32_t>(swapChainImages.size()));
        commandBuffers = device.allocateCommandBuffers(allocInfo);
    };

    auto createSyncObjects = [this] {
        max_images_in_flight = static_cast<uint32_t>(swapChainImages.size() - 1);
        max_images_in_flight = std::max<uint32_t>(max_images_in_flight, 1);

        imageAvailableSemaphore.resize(max_images_in_flight);
        renderFinishedSemaphore.resize(max_images_in_flight);
        inFlightFences.resize(max_images_in_flight);

        vk::SemaphoreCreateInfo semaphoreCreateInfo;
        vk::FenceCreateInfo fenceCreateInfo(vk::FenceCreateFlagBits::eSignaled);
        for (uint32_t i = 0; i < max_images_in_flight; ++i)
        {
            imageAvailableSemaphore[i] = device.createSemaphore(semaphoreCreateInfo);
            renderFinishedSemaphore[i] = device.createSemaphore(semaphoreCreateInfo);
            inFlightFences[i] = device.createFence(fenceCreateInfo);
        }
    };

    //code here

    
    createSyncObjects();
}

void VulkanApp::userLoopFunc()
{
    //code here
    device.waitForFences(1, &inFlightFences[current_frame], VK_TRUE, (std::numeric_limits<uint32_t>::max)());
    device.resetFences(1, &inFlightFences[current_frame]);

    auto frameIndex = device.acquireNextImageKHR(
        swapChainHandle, (std::numeric_limits<uint32_t>::max)(), imageAvailableSemaphore[current_frame], {});

    updateUniformBuffer(frameIndex.value);

    vk::PipelineStageFlags waitStages = vk::PipelineStageFlagBits::eColorAttachmentOutput;

    vk::SubmitInfo submitInfo(
        1, &imageAvailableSemaphore[current_frame], &waitStages,
        1, &commandBuffers[frameIndex.value],
        1, &renderFinishedSemaphore[current_frame]);

    graphicsQueue.submit(submitInfo, inFlightFences[current_frame]);

    vk::PresentInfoKHR presentInfo(
        1, &renderFinishedSemaphore[current_frame],
        1, &swapChainHandle, &frameIndex.value, nullptr);

    graphicsQueue.presentKHR(presentInfo);

    current_frame = (current_frame + 1) % max_images_in_flight;
}

void VulkanApp::userDestroy()
{
    //code here


    //destroy frame buffers
    for (const auto i : swapChainFramebuffers)
    {
        device.destroyFramebuffer(i);
    }

    //destroy depth resources
    for (uint32_t i = 0; i < depthImages.size(); i++)
    {
        device.destroyImage(depthImages[i]);
        device.destroyImageView(depthImageViews[i]);
        device.freeMemory(depthImageMemorys[i]);
    }

    //destroy synchronize objects
    for (uint32_t i = 0; i < inFlightFences.size(); ++i)
    {
        device.destroySemaphore(imageAvailableSemaphore[i]);
        device.destroySemaphore(renderFinishedSemaphore[i]);
        device.destroyFence(inFlightFences[i]);
    }

    //destroy command pool
    device.destroyCommandPool(commandPool);
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

void VulkanApp::updateUniformBuffer(uint32_t imageIndex)
{
    using glm::mat4;
    using glm::vec3;
    using glm::rotate;
    using glm::translate;
    using glm::scale;
    using glm::lookAt;
    using glm::perspective;
    using glm::radians;

    static auto startTime = std::chrono::high_resolution_clock::now();
    float time =
        std::chrono::duration_cast<std::chrono::duration<float, std::ratio<1>>>(
            std::chrono::high_resolution_clock::now() - startTime).count();

    SimpleUniformObject ubo;
    ubo.model = rotate(mat4(1.0f), time * radians(60.0f), vec3(0.0f, 0.0f, 1.0f));
    ubo.view = lookAt(vec3(2.0f, 2.0f, 2.0f), vec3(0.0f, 0.0f, 0.0f), vec3(0.0f, 0.0f, 1.0f));
    ubo.project = perspective(radians(45.0f), float(swapChain.extent().width) / float(swapChain.extent().height), 0.1f, 10.0f);
    ubo.project[1][1] *= -1;
    void* data = device.mapMemory(uniformBufferMemorys[imageIndex], 0, sizeof(SimpleUniformObject));
    memcpy(data, &ubo, sizeof(SimpleUniformObject));
    device.unmapMemory(uniformBufferMemorys[imageIndex]);
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
