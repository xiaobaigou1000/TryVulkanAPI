#include "MyVulkanApp.h"
#include<thread>

std::tuple<std::vector<Vertex>, std::vector<uint32_t>> CreateTorusMesh(float outerRadius, float innerRadius, uint32_t nsides, uint32_t nrings)
{
    size_t faces = nsides * nrings;
    size_t nVerts = nsides * (nrings + 1);   // One extra ring to duplicate first ring

    // Points
    std::vector<float> p(3 * nVerts);
    // Normals
    std::vector<float> n(3 * nVerts);
    // Tex coords
    std::vector<float> tex(2 * nVerts);
    // Elements
    std::vector<uint32_t> el(6 * faces);

    // Generate the vertex data
    float ringFactor = glm::two_pi<float>() / nrings;
    float sideFactor = glm::two_pi<float>() / nsides;
    size_t idx = 0, tidx = 0;
    for (uint32_t ring = 0; ring <= nrings; ring++) {
        float u = ring * ringFactor;
        float cu = cos(u);
        float su = sin(u);
        for (uint32_t side = 0; side < nsides; side++) {
            float v = side * sideFactor;
            float cv = cos(v);
            float sv = sin(v);
            float r = (outerRadius + innerRadius * cv);
            p[idx] = r * cu;
            p[idx + 1] = r * su;
            p[idx + 2] = innerRadius * sv;
            n[idx] = cv * cu * r;
            n[idx + 1] = cv * su * r;
            n[idx + 2] = sv * r;
            tex[tidx] = u / glm::two_pi<float>();
            tex[tidx + 1] = v / glm::two_pi<float>();
            tidx += 2;
            // Normalize
            float len = sqrt(n[idx] * n[idx] +
                n[idx + 1] * n[idx + 1] +
                n[idx + 2] * n[idx + 2]);
            n[idx] /= len;
            n[idx + 1] /= len;
            n[idx + 2] /= len;
            idx += 3;
        }
    }

    idx = 0;
    for (uint32_t ring = 0; ring < nrings; ring++) {
        uint32_t ringStart = ring * nsides;
        uint32_t nextRingStart = (ring + 1) * nsides;
        for (uint32_t side = 0; side < nsides; side++) {
            int nextSide = (side + 1) % nsides;
            // The quad
            el[idx] = (ringStart + side);
            el[idx + 1] = (nextRingStart + side);
            el[idx + 2] = (nextRingStart + nextSide);
            el[idx + 3] = ringStart + side;
            el[idx + 4] = nextRingStart + nextSide;
            el[idx + 5] = (ringStart + nextSide);
            idx += 6;
        }
    }

    std::vector<Vertex> vertices(nVerts);
    for (uint32_t i = 0; i < nVerts; i++)
    {
        Vertex vertex;
        vertex.position = glm::vec3(p[3 * i], p[3 * i + 1], p[3 * i + 2]);
        vertex.normal = glm::vec3(n[3 * i], n[3 * i + 1], n[3 * i + 2]);
        vertex.texCoords = glm::vec2(tex[2 * i], tex[2 * i + 1]);
        vertices[i] = vertex;
    }

    return { std::move(vertices),std::move(el) };
}

void MyVulkanApp::userInit()
{
    //init shader
    shader.init(device, swapChain.extent(), swapChain.imageFormat(), depthImageFormat);
    shader.createColorDepthRenderPass();
    std::vector<vk::VertexInputBindingDescription> vertexBindingDescription;
    vertexBindingDescription.push_back({ 0, sizeof(Vertex), vk::VertexInputRate::eVertex });

    std::vector<vk::VertexInputAttributeDescription> vertexInputAttributeDescription;
    vertexInputAttributeDescription.push_back({ 0, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, position) });
    vertexInputAttributeDescription.push_back({ 1, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, normal) });

    vk::PipelineVertexInputStateCreateInfo vertexInputInfo(
        {},
        1, vertexBindingDescription.data(),
        2, vertexInputAttributeDescription.data());

    std::vector<vk::DescriptorSetLayoutBinding> descriptorSetLayoutBinding;
    descriptorSetLayoutBinding.push_back({ 0,vk::DescriptorType::eUniformBuffer,1,vk::ShaderStageFlagBits::eVertex });
    descriptorSetLayoutBinding.push_back({ 1,vk::DescriptorType::eUniformBuffer,1,vk::ShaderStageFlagBits::eFragment });
    vk::DescriptorSetLayoutCreateInfo descriptorSetlayoutCreateInfo(
        {},
        2, descriptorSetLayoutBinding.data());
    descriptorSetLayout = device.createDescriptorSetLayout(descriptorSetlayoutCreateInfo);

    vk::PushConstantRange pushConstantRange(vk::ShaderStageFlagBits::eFragment, 0, 2 * sizeof(float));

    vk::PipelineLayoutCreateInfo pipelineLayoutCreateInfo(
        {},
        1, &descriptorSetLayout,
        1, &pushConstantRange);
    shader.createDefaultVFShader("./shaders/cartoonShaderVert.spv", "./shaders/fogShaderFrag.spv",
        vertexInputInfo, pipelineLayoutCreateInfo);

    //init framebuffers
    vk::FramebufferCreateInfo framebufferCreateInfo(
        {},
        shader.getRenderPass(),
        0, nullptr,
        swapChain.extent().width, swapChain.extent().height,
        1);
    framebuffers.resize(swapChainImages.size());
    for (uint32_t i = 0; i < swapChainImages.size(); i++)
    {
        vk::ImageView attachments[] = { swapChainImageViews[i],depthImageViews[i] };
        framebufferCreateInfo.attachmentCount = 2;
        framebufferCreateInfo.pAttachments = attachments;
        framebuffers[i] = device.createFramebuffer(framebufferCreateInfo);
    }

    //create vertex and index buffers
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    std::tie(vertices, indices) = CreateTorusMesh(0.7f, 0.3f, 50, 50);
    uint32_t queueFamilyIndex = context.getQueueFamilyIndex();
    vk::DeviceSize vertexIndexBufferSize = vertices.size() * sizeof(Vertex) + indices.size() * sizeof(float);
    vertexOffset = 0;
    indexOffset = vertices.size() * sizeof(Vertex);

    vk::Buffer stageBuffer;
    vk::DeviceMemory stageBufferMemory;
    std::tie(stageBuffer, stageBufferMemory) = createBuffer(
        vertexIndexBufferSize,
        vk::BufferUsageFlagBits::eTransferSrc,
        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
    void* stageMemoryPtr = device.mapMemory(stageBufferMemory, 0, vertexIndexBufferSize);
    memcpy(stageMemoryPtr, vertices.data(), vertices.size() * sizeof(Vertex));
    device.unmapMemory(stageBufferMemory);
    stageMemoryPtr = device.mapMemory(stageBufferMemory, indexOffset, indices.size() * sizeof(float));
    memcpy(stageMemoryPtr, indices.data(), indices.size() * sizeof(float));
    device.unmapMemory(stageBufferMemory);

    std::tie(vertexIndexBuffer, vertexIndexBufferMemory) = createBuffer(
        vertexIndexBufferSize,
        vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eIndexBuffer,
        vk::MemoryPropertyFlagBits::eDeviceLocal);

    copyBuffer(stageBuffer, vertexIndexBuffer, vertexIndexBufferSize);
    device.destroyBuffer(stageBuffer);
    device.freeMemory(stageBufferMemory);

    //create uniform buffer
    CameraUniform cu;
    camera.view = glm::lookAt(glm::vec3(0.0f, 0.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    camera.projection = glm::perspective(glm::radians(70.0f), (float)swapChain.extent().width / swapChain.extent().height, 0.3f, 100.0f);
    camera.projection[1][1] *= -1.0f;
    camera.model = glm::mat4(1.0f);
    camera.model = glm::rotate(camera.model, glm::radians(-35.0f), glm::vec3(1.0f, 0.0f, 0.0f));
    camera.model = glm::rotate(camera.model, glm::radians(35.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    cu.modelView = camera.view * camera.model;
    glm::mat3 normalMat = glm::mat3(glm::vec3(cu.modelView[0]), glm::vec3(cu.modelView[1]), glm::vec3(cu.modelView[2]));
    cu.normal = glm::mat4(glm::vec4(normalMat[0], 1.0f), glm::vec4(normalMat[1], 1.0f), glm::vec4(normalMat[2], 1.0f), glm::vec4(0.0f));
    cu.MVP = camera.projection * camera.view * camera.model;

    LightUniform lu;
    lu.lightPosition = camera.view * glm::vec4(5.0f, 5.0f, 2.0f, 1.0f);
    lu.Kd = glm::vec4(0.9f, 0.5f, 0.3f, 0.0f);
    lu.Ld = glm::vec4(1.0f, 1.0f, 1.0f, 0.0f);
    lu.Ka = 0.1f * lu.Kd;
    lu.fogColor = glm::vec4(0.5f, 0.5f, 0.5f, 1.0f);

    vk::DeviceSize uniformObjectSize = sizeof(CameraUniform) + sizeof(LightUniform);
    lightUniformOffset = sizeof(CameraUniform);
    std::tie(uniformBuffer, uniformBufferMemory, uniformBufferSize) = createBufferForArrayObjects(
        uniformObjectSize,
        swapChainImages.size(),
        vk::BufferUsageFlagBits::eUniformBuffer,
        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);

    uint8_t* uniformDataPtr = (uint8_t*)device.mapMemory(uniformBufferMemory, 0, VK_WHOLE_SIZE);
    for (uint32_t i = 0; i < swapChainImages.size(); i++)
    {
        memcpy(uniformDataPtr + i * uniformBufferSize, &cu, sizeof(CameraUniform));
        memcpy(uniformDataPtr + i * uniformBufferSize + sizeof(CameraUniform), &lu, sizeof(LightUniform));
    }
    device.unmapMemory(uniformBufferMemory);

    //create descriptor pool
    vk::DescriptorPoolSize uniformDescriptorSize(vk::DescriptorType::eUniformBuffer, 2 * swapChainImages.size());
    vk::DescriptorPoolCreateInfo descriptorPoolInfo({}, swapChainImages.size(), 1, &uniformDescriptorSize);
    descriptorPool = device.createDescriptorPool(descriptorPoolInfo);
    std::vector<vk::DescriptorSetLayout> descriptorSetLayoutAllocInfo(swapChainImages.size(), descriptorSetLayout);
    vk::DescriptorSetAllocateInfo descriptorSetAllocInfo(descriptorPool, swapChainImages.size(), descriptorSetLayoutAllocInfo.data());
    descriptorSets = device.allocateDescriptorSets(descriptorSetAllocInfo);

    //update descriptor sets
    for (int i = 0; i < descriptorSets.size(); i++)
    {
        vk::DescriptorBufferInfo cameraUniformBufferInfo(uniformBuffer, i * uniformBufferSize, sizeof(CameraUniform));
        vk::DescriptorBufferInfo lightUniformInfo(uniformBuffer, lightUniformOffset + i * uniformBufferSize, sizeof(LightUniform));

        vk::WriteDescriptorSet cameraUniformWrite(
            descriptorSets[i],
            0,
            0,
            1,
            vk::DescriptorType::eUniformBuffer,
            nullptr,
            &cameraUniformBufferInfo);

        vk::WriteDescriptorSet lightUniformWrite(
            descriptorSets[i],
            1,
            0,
            1,
            vk::DescriptorType::eUniformBuffer,
            nullptr,
            &lightUniformInfo);

        device.updateDescriptorSets({ cameraUniformWrite,lightUniformWrite }, {});
    }

    //record command buffers
    vk::CommandBufferAllocateInfo commandBufferAllocInfo(commandPool, vk::CommandBufferLevel::ePrimary, swapChainImages.size());
    commandBuffers = device.allocateCommandBuffers(commandBufferAllocInfo);

    for (int i = 0; i < commandBuffers.size(); i++)
    {
        vk::CommandBufferBeginInfo commandBeginInfo({}, nullptr);
        commandBuffers[i].begin(commandBeginInfo);
        std::vector<vk::ClearValue> clearValues;
        clearValues.push_back(vk::ClearColorValue(std::array<float, 4>{ 0.5, 0.5, 0.5, 1 }));
        clearValues.push_back(vk::ClearDepthStencilValue(1.0f, 0));
        vk::RenderPassBeginInfo renderPassBeginInfo(shader.getRenderPass(), framebuffers[i], { {0,0},swapChain.extent() }, clearValues.size(), clearValues.data());
        commandBuffers[i].beginRenderPass(renderPassBeginInfo, vk::SubpassContents::eInline);
        commandBuffers[i].bindPipeline(vk::PipelineBindPoint::eGraphics, shader.getPipeline());
        commandBuffers[i].bindVertexBuffers(0, vertexIndexBuffer, { 0 });
        commandBuffers[i].bindIndexBuffer(vertexIndexBuffer, indexOffset, vk::IndexType::eUint32);
        commandBuffers[i].bindDescriptorSets(vk::PipelineBindPoint::eGraphics, shader.getPipelineLayout(), 0, descriptorSets[i], {});
        commandBuffers[i].pushConstants<float>(shader.getPipelineLayout(), vk::ShaderStageFlagBits::eFragment, 0, { 4.0f,1 / 4.0f });
        commandBuffers[i].drawIndexed(indices.size(), 1, 0, 0, 0);
        commandBuffers[i].endRenderPass();
        commandBuffers[i].end();
    }

    max_images_in_flight = swapChainImages.size() - 1;
    max_images_in_flight = max_images_in_flight > 0 ? max_images_in_flight : 1;

    imageReadySemaphores.resize(max_images_in_flight);
    renderFinishedSemaphores.resize(max_images_in_flight);
    fences.resize(max_images_in_flight);
    for (uint32_t i = 0; i < max_images_in_flight; i++)
    {
        imageReadySemaphores[i] = device.createSemaphore({});
        renderFinishedSemaphores[i] = device.createSemaphore({});
        fences[i] = device.createFence({ vk::FenceCreateFlagBits::eSignaled });
    }

    last_frame = start_time = std::chrono::high_resolution_clock::now();
}

void MyVulkanApp::userLoopFunc()
{
    device.waitForFences(fences[current_frame], VK_TRUE, 0xFFFFFFFF);
    device.resetFences(fences[current_frame]);
    auto current_time = std::chrono::high_resolution_clock::now();
    float timeInterval = std::chrono::duration_cast<std::chrono::duration<float, std::ratio<1>>>(current_time - last_frame).count();
    last_frame = current_time;

    auto imageIndex = device.acquireNextImageKHR(swapChainHandle, 0xFFFFFFFF, imageReadySemaphores[current_frame], {});

    std::this_thread::sleep_for(std::chrono::milliseconds(5));
    CameraUniform camUniform;
    camera.model = glm::rotate(camera.model, timeInterval * glm::radians(45.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    camUniform.modelView = camera.view * camera.model;
    camUniform.MVP = camera.projection * camera.view * camera.model;
    camUniform.projection = camera.projection;
    glm::mat3 normalMat = glm::mat3(glm::vec3(camUniform.modelView[0]), glm::vec3(camUniform.modelView[1]), glm::vec3(camUniform.modelView[2]));
    camUniform.normal = glm::mat4(glm::vec4(normalMat[0], 1.0f), glm::vec4(normalMat[1], 1.0f), glm::vec4(normalMat[2], 1.0f), glm::vec4(0.0f));
    uint8_t* uniformPtr = (uint8_t*)device.mapMemory(uniformBufferMemory, 0, VK_WHOLE_SIZE);
    size_t uniformSize = sizeof(CameraUniform);
    memcpy(uniformPtr + uniformBufferSize * imageIndex.value, &camUniform, sizeof(CameraUniform));
    device.unmapMemory(uniformBufferMemory);

    vk::PipelineStageFlags waitStage = vk::PipelineStageFlagBits::eColorAttachmentOutput;
    vk::SubmitInfo renderSubmitInfo(1, &imageReadySemaphores[current_frame], &waitStage, 1, &commandBuffers[imageIndex.value], 1, &renderFinishedSemaphores[current_frame]);
    graphicsQueue.submit(renderSubmitInfo, fences[current_frame]);
    vk::PresentInfoKHR presentInfo(1, &renderFinishedSemaphores[current_frame], 1, &swapChainHandle, &imageIndex.value, nullptr);
    graphicsQueue.presentKHR(presentInfo);

    current_frame = (current_frame + 1) % max_images_in_flight;
}

void MyVulkanApp::userDestroy()
{
    device.destroyDescriptorSetLayout(descriptorSetLayout);
    device.destroyDescriptorPool(descriptorPool);
    for (auto i : framebuffers)
        device.destroyFramebuffer(i);
    for (auto i : fences)
        device.destroyFence(i);
    for (auto i : imageReadySemaphores)
        device.destroySemaphore(i);
    for (auto i : renderFinishedSemaphores)
        device.destroySemaphore(i);
    device.destroyBuffer(vertexIndexBuffer);
    device.freeMemory(vertexIndexBufferMemory);
    device.destroyBuffer(uniformBuffer);
    device.freeMemory(uniformBufferMemory);

    shader.destroy();
}
