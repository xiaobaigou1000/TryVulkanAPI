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
    vk::PipelineLayoutCreateInfo pipelineLayoutInfo({}, 0, nullptr, 0, nullptr);
    shader.createColorOnlyRenderPass();
    auto vertexBinding = Vertex::getBindingDescription();
    auto vertexAttribute = Vertex::getAttributeDescription();
    vk::PipelineVertexInputStateCreateInfo vertexInputInfo{ {},1,&vertexBinding,static_cast<uint32_t>(vertexAttribute.size()),vertexAttribute.data() };
    shader.createDefaultVFShader("./shaders/triangleWithVertexInputVert.spv", "./shaders/triangleWithVertexInputFrag.spv",
        vertexInputInfo, pipelineLayoutInfo);

    //create vertex buffers
    uint32_t queueFamilyIndices = context.getQueueFamilyIndex();
    vk::BufferCreateInfo vertexBufferInfo(
        {},
        vertices.size() * sizeof(Vertex),
        vk::BufferUsageFlagBits::eVertexBuffer,
        vk::SharingMode::eExclusive,
        1,
        &queueFamilyIndices);
    vertexBuffer = device.createBuffer(vertexBufferInfo);
    vk::MemoryRequirements vertexBufferRequirements = device.getBufferMemoryRequirements(vertexBuffer);
    vk::MemoryAllocateInfo vertexMemoryAllocInfo(
        vertexBufferRequirements.size,
        findMemoryType(
            vertexBufferRequirements.memoryTypeBits,
            vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent));
    vertexBufferMemory = device.allocateMemory(vertexMemoryAllocInfo);
    device.bindBufferMemory(vertexBuffer, vertexBufferMemory, 0);

    //fill vertex buffer
    void* data = device.mapMemory(vertexBufferMemory, 0, vertexBufferRequirements.size);
    memcpy(data, vertices.data(), vertexBufferRequirements.size);
    device.unmapMemory(vertexBufferMemory);

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
        commandBuffers[i].bindVertexBuffers(0, std::array<vk::Buffer, 1>{ vertexBuffer }, std::array<vk::DeviceSize, 1>{ 0 });
        commandBuffers[i].draw(static_cast<uint32_t>(vertices.size()), 1, 0, 0);
        commandBuffers[i].endRenderPass();
        commandBuffers[i].end();
    }

    //create synchronize objects
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
    device.destroyBuffer(vertexBuffer);
    device.freeMemory(vertexBufferMemory);

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

uint32_t VulkanApp::findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties)
{
    vk::PhysicalDeviceMemoryProperties memoryProperties = context.getPhysicalDeviceHandle().getMemoryProperties();
    for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; ++i)
    {
        bool memoryTypeSupport = typeFilter & (1 << i);
        bool memoryPropertySupport = (memoryProperties.memoryTypes[i].propertyFlags & properties) == properties;
        if (memoryTypeSupport && memoryPropertySupport)
        {
            return i;
        }
    }
    throw std::runtime_error("no memory type supported.");
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
