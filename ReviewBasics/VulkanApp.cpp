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
    shader.createColorOnlyRenderPass();
    auto vertexBinding = Vertex::getBindingDescription();
    auto vertexAttribute = Vertex::getAttributeDescription();
    vk::DescriptorSetLayoutBinding descriptorBinding(
        0,
        vk::DescriptorType::eUniformBuffer,
        1,
        vk::ShaderStageFlagBits::eVertex,
        nullptr);
    vk::DescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo({}, 1, &descriptorBinding);
    descriptorSetLayout = device.createDescriptorSetLayout(descriptorSetLayoutCreateInfo);
    vk::PipelineLayoutCreateInfo pipelineLayoutInfo({}, 1, &descriptorSetLayout, 0, nullptr);
    vk::PipelineVertexInputStateCreateInfo vertexInputInfo{ {},1,&vertexBinding,static_cast<uint32_t>(vertexAttribute.size()),vertexAttribute.data() };
    shader.createDefaultVFShader("./shaders/triangleWithUniformVert.spv", "./shaders/triangleWithUniformFrag.spv",
        vertexInputInfo, pipelineLayoutInfo);

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

    //create vertex buffers
    vk::DeviceSize vertexBufferSize = vertices.size() * sizeof(Vertex);
    vk::Buffer stageBuffer;
    vk::DeviceMemory stageBufferMemory;
    std::tie(stageBuffer, stageBufferMemory) = createBuffer(
        vertexBufferSize,
        vk::BufferUsageFlagBits::eTransferSrc,
        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);

    void* data = device.mapMemory(stageBufferMemory, 0, vertexBufferSize);
    memcpy(data, vertices.data(), vertexBufferSize);
    device.unmapMemory(stageBufferMemory);

    std::tie(vertexBuffer, vertexBufferMemory) = createBuffer(
        vertexBufferSize,
        vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer,
        vk::MemoryPropertyFlagBits::eDeviceLocal);

    copyBuffer(stageBuffer, vertexBuffer, vertexBufferSize);
    device.destroyBuffer(stageBuffer);
    device.freeMemory(stageBufferMemory);

    //create index buffer
    vk::DeviceSize indexBufferSize = indices.size() * sizeof(uint32_t);
    std::tie(stageBuffer, stageBufferMemory) = createBuffer(
        indexBufferSize,
        vk::BufferUsageFlagBits::eTransferSrc,
        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);

    data = device.mapMemory(stageBufferMemory, 0, indexBufferSize);
    memcpy(data, indices.data(), indexBufferSize);
    device.unmapMemory(stageBufferMemory);

    std::tie(indexBuffer, indexBufferMemory) = createBuffer(
        indexBufferSize,
        vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eTransferDst,
        vk::MemoryPropertyFlagBits::eDeviceLocal
        );

    copyBuffer(stageBuffer, indexBuffer, indexBufferSize);
    device.destroyBuffer(stageBuffer);
    device.freeMemory(stageBufferMemory);

    //create uniform buffers
    uniformBuffers.resize(swapChainImages.size());
    uniformBufferMemorys.resize(swapChainImages.size());
    for (uint32_t i = 0; i < uniformBuffers.size(); ++i)
    {
        vk::DeviceSize uboSize = sizeof(SimpleUniformObject);
        std::tie(uniformBuffers[i], uniformBufferMemorys[i]) = createBuffer(
            uboSize,
            vk::BufferUsageFlagBits::eUniformBuffer,
            vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent
            );
    }

    //load texture image to stage buffer
    cv::Mat rawPicture = cv::imread("./textures/dog.jpg");
    cv::Mat picture;
    cv::cvtColor(rawPicture, picture, cv::ColorConversionCodes::COLOR_BGR2RGBA);

    void* pictureData = picture.data;
    size_t elemSize = picture.elemSize();
    size_t pictureSize = picture.total() * picture.elemSize();
    std::tie(stageBuffer, stageBufferMemory) = createBuffer(
        pictureSize,
        vk::BufferUsageFlagBits::eTransferSrc,
        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent
        );
    data = device.mapMemory(stageBufferMemory, 0, pictureSize);
    memcpy(data, pictureData, pictureSize);
    device.unmapMemory(stageBufferMemory);

    //create texture VkImage
    uint32_t queueFamilyIndex = context.getQueueFamilyIndex();
    vk::ImageCreateInfo textureImageCreateInfo(
        {},
        vk::ImageType::e2D,
        vk::Format::eR8G8B8A8Unorm,
        vk::Extent3D(picture.rows, picture.cols, 1),
        1,
        1,
        vk::SampleCountFlagBits::e1,
        vk::ImageTiling::eOptimal,
        vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled,
        vk::SharingMode::eExclusive,
        1,
        &queueFamilyIndex,
        vk::ImageLayout::eUndefined);
    textureImage = device.createImage(textureImageCreateInfo);
    vk::MemoryRequirements textureMemoryRequirements = device.getImageMemoryRequirements(textureImage);
    vk::MemoryAllocateInfo textureImageMemoryAllocateInfo(
        textureMemoryRequirements.size,
        findMemoryType(textureMemoryRequirements.memoryTypeBits, vk::MemoryPropertyFlagBits::eDeviceLocal));
    textureImageMemory = device.allocateMemory(textureImageMemoryAllocateInfo);
    device.bindImageMemory(textureImage, textureImageMemory, 0);

    transitionImageLayout(textureImage, vk::Format::eR8G8B8A8Unorm, vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal);
    copyBufferToImage(stageBuffer, textureImage, picture.rows, picture.cols);
    transitionImageLayout(textureImage, vk::Format::eR8G8B8A8Unorm, vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eShaderReadOnlyOptimal);

    //free staging buffer
    device.destroyBuffer(stageBuffer);
    device.freeMemory(stageBufferMemory);

    //create texture image view
    vk::ImageSubresourceRange subResource;
    vk::ImageViewCreateInfo textureViewInfo(
        {},
        textureImage,
        vk::ImageViewType::e2D,
        vk::Format::eR8G8B8A8Unorm,
        {},
        { vk::ImageAspectFlagBits::eColor,0,1,0,1 });
    textureImageView = device.createImageView(textureViewInfo);

    //create texture sampler
    vk::SamplerCreateInfo samplerCreateInfo(
        {},
        vk::Filter::eLinear,
        vk::Filter::eLinear,
        vk::SamplerMipmapMode::eLinear,
        vk::SamplerAddressMode::eRepeat,
        vk::SamplerAddressMode::eRepeat,
        vk::SamplerAddressMode::eRepeat,
        0.0f,
        VK_TRUE,
        16.0f,
        VK_FALSE,
        vk::CompareOp::eAlways,
        0.0f,
        0.0f,
        vk::BorderColor::eIntOpaqueWhite,
        VK_FALSE);
    textureSampler = device.createSampler(samplerCreateInfo);

    //create descriptor pool
    vk::DescriptorPoolSize descriptorPoolSize(vk::DescriptorType::eUniformBuffer, static_cast<uint32_t>(swapChainImages.size()));
    vk::DescriptorPoolCreateInfo descriptorPoolCreateInfo({}, static_cast<uint32_t>(swapChainImages.size()), 1, &descriptorPoolSize);
    descriptorPool = device.createDescriptorPool(descriptorPoolCreateInfo);

    //create and configure descriptor sets
    std::vector<vk::DescriptorSetLayout> layouts(swapChainImages.size(), descriptorSetLayout);
    vk::DescriptorSetAllocateInfo descriptorSetAllocateInfo(descriptorPool, static_cast<uint32_t>(layouts.size()), layouts.data());
    descriptorSets = device.allocateDescriptorSets(descriptorSetAllocateInfo);
    for (uint32_t i = 0; i < descriptorSets.size(); ++i)
    {
        vk::DescriptorBufferInfo descriptorBufferInfo(uniformBuffers[i], 0, sizeof(SimpleUniformObject));
        vk::WriteDescriptorSet descriptorWrite(
            descriptorSets[i],
            0,
            0,
            1,
            vk::DescriptorType::eUniformBuffer,
            nullptr,
            &descriptorBufferInfo,
            nullptr);
        device.updateDescriptorSets(descriptorWrite, {});
    }

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
        commandBuffers[i].bindIndexBuffer(indexBuffer, 0, vk::IndexType::eUint32);
        commandBuffers[i].bindDescriptorSets(vk::PipelineBindPoint::eGraphics, shader.getPipelineLayout(), 0, descriptorSets[i], {});
        commandBuffers[i].drawIndexed(static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);
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
    device.destroySampler(textureSampler);
    device.destroyImageView(textureImageView);
    device.destroyImage(textureImage);
    device.freeMemory(textureImageMemory);

    for (uint32_t i = 0; i < uniformBuffers.size(); ++i)
    {
        device.destroyBuffer(uniformBuffers[i]);
        device.freeMemory(uniformBufferMemorys[i]);
    }

    device.destroyDescriptorPool(descriptorPool);
    device.destroyDescriptorSetLayout(descriptorSetLayout);

    device.destroyBuffer(indexBuffer);
    device.freeMemory(indexBufferMemory);

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

std::tuple<vk::Buffer, vk::DeviceMemory> VulkanApp::createBuffer(vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties)
{
    uint32_t queueFamilyIndex = context.getQueueFamilyIndex();
    vk::BufferCreateInfo bufferCreateInfo({}, size, usage, vk::SharingMode::eExclusive, 1, &queueFamilyIndex);
    vk::Buffer buffer = device.createBuffer(bufferCreateInfo);
    vk::MemoryRequirements memoryRequirements = device.getBufferMemoryRequirements(buffer);
    vk::MemoryAllocateInfo memoryAllocInfo(memoryRequirements.size, findMemoryType(memoryRequirements.memoryTypeBits, properties));
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

void VulkanApp::mainLoop()
{
    while (!window.shouldClose())
    {
        window.pollEvents();
        userLoopFunc();
    }
    graphicsQueue.waitIdle();
}
