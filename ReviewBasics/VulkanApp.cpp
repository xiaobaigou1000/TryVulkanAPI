#include"VulkanApp.h"
#include<limits>

void VulkanApp::run()
{
    mainLoop();
}

void VulkanApp::init()
{
    window.init();
    context.init(window);
    device = context.createLogicalDevice({ VK_KHR_SWAPCHAIN_EXTENSION_NAME }, {});
    graphicsQueue = device.getQueue(context.getQueueFamilyIndex(), 0);
    swapChain.init(context, device, window.extent());
    swapChainHandle = swapChain.handle();
    std::tie(swapChainImages, swapChainImageViews) = swapChain.getSwapChainImages();

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
    //code here

    //create shader pipeline
    shader.init(device, swapChain.extent(), swapChain.imageFormat());
    vk::PipelineVertexInputStateCreateInfo vertexInputInfo({}, 0, nullptr, 0, nullptr);
    vk::PipelineLayoutCreateInfo pipelineLayoutInfo({}, 0, nullptr, 0, nullptr);
    shader.createColorOnlyRenderPass();
    shader.createDefaultVFShader("./shaders/simpleTriangleVert.spv", "./shaders/simpleTriangleFrag.spv", vertexInputInfo, pipelineLayoutInfo);

    //create framebuffers
    swapChainColorOnlyFramebuffers.resize(swapChainImageViews.size());
    for (uint32_t i = 0; i < swapChainImageViews.size(); ++i)
    {
        vk::FramebufferCreateInfo framebufferInfo({}, shader.getRenderPass(), 1, &swapChainImageViews[i], swapChain.extent().width, swapChain.extent().height, 1);
        swapChainColorOnlyFramebuffers[i] = device.createFramebuffer(framebufferInfo);
    }

    //create command pool and allocate command buffers
    vk::CommandPoolCreateInfo commandPoolInfo({}, context.getQueueFamilyIndex());
    commandPool = device.createCommandPool(commandPoolInfo);
    vk::CommandBufferAllocateInfo allocInfo(commandPool,
        vk::CommandBufferLevel::ePrimary, static_cast<uint32_t>(swapChainColorOnlyFramebuffers.size()));
    commandBuffers = device.allocateCommandBuffers(allocInfo);

    //record command buffers
    for (uint32_t i = 0; i < commandBuffers.size(); ++i)
    {
        vk::CommandBufferBeginInfo beginInfo({}, nullptr);
        commandBuffers[i].begin(beginInfo);
        vk::ClearValue black{ std::array<float,4>{ 0.0f,0.0f,0.0f,1.0f } };
        vk::RenderPassBeginInfo renderPassBeginInfo(shader.getRenderPass(),
            swapChainColorOnlyFramebuffers[i], { {0,0},swapChain.extent() },
            1, &black);
        commandBuffers[i].beginRenderPass(renderPassBeginInfo, vk::SubpassContents::eInline);
        commandBuffers[i].bindPipeline(vk::PipelineBindPoint::eGraphics, shader.getPipeline());
        commandBuffers[i].draw(3, 1, 0, 0);
        commandBuffers[i].endRenderPass();
        commandBuffers[i].end();
    }

    //create synchronize objects
    max_images_in_flight = swapChainImages.size() - 1;
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
}

void VulkanApp::userLoopFunc()
{
    //code here
    device.waitForFences(1, &inFlightFences[current_frame], VK_TRUE, (std::numeric_limits<uint32_t>::max)());
    device.resetFences(1, &inFlightFences[current_frame]);

    auto frameIndex = device.acquireNextImageKHR(
        swapChainHandle, (std::numeric_limits<uint32_t>::max)(), imageAvailableSemaphore[current_frame], {});

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

    for (uint32_t i = 0; i < inFlightFences.size(); ++i)
    {
        device.destroySemaphore(imageAvailableSemaphore[i]);
        device.destroySemaphore(renderFinishedSemaphore[i]);
        device.destroyFence(inFlightFences[i]);
    }

    device.destroyCommandPool(commandPool);
    for (const auto i : swapChainColorOnlyFramebuffers)
    {
        device.destroyFramebuffer(i);
    }
    shader.destroy();
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
