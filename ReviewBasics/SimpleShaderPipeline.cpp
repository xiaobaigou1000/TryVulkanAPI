#include "SimpleShaderPipeline.h"
#include<fstream>

void SimpleShaderPipeline::init(const vk::Device device, const vk::Extent2D windowExtent, const vk::Format framebufferFormat, const vk::Format depthStencilFormat)
{
    this->device = device;
    this->framebufferFormat = framebufferFormat;
    this->depthStencilFormat = depthStencilFormat;

    vertexInputInfo = vk::PipelineVertexInputStateCreateInfo({}, 0, nullptr, 0, nullptr);
    inputAssemblyInfo = vk::PipelineInputAssemblyStateCreateInfo({}, vk::PrimitiveTopology::eTriangleList, VK_FALSE);
    viewport = vk::Viewport(0, 0, static_cast<float>(windowExtent.width), static_cast<float>(windowExtent.height), 0.0f, 1.0f);
    scissor = vk::Rect2D({ 0,0 }, windowExtent);
    viewportInfo = vk::PipelineViewportStateCreateInfo({}, 1, &viewport, 1, &scissor);
    rasterizer = vk::PipelineRasterizationStateCreateInfo(
        {}, VK_FALSE, VK_FALSE, vk::PolygonMode::eFill,
        vk::CullModeFlagBits::eBack, vk::FrontFace::eCounterClockwise, VK_FALSE);
    rasterizer.setLineWidth(1.0f);

    multisampleInfo = vk::PipelineMultisampleStateCreateInfo({}, vk::SampleCountFlagBits::e1, VK_FALSE);
    colorBlendAttachmentInfo = vk::PipelineColorBlendAttachmentState(VK_FALSE);
    colorBlendAttachmentInfo.setColorWriteMask(vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
        vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA);

    colorBlendStateInfo = vk::PipelineColorBlendStateCreateInfo({}, VK_FALSE);
    colorBlendStateInfo.setAttachmentCount(1).setPAttachments(&colorBlendAttachmentInfo);
    pipelineLayoutCreateInfo = vk::PipelineLayoutCreateInfo({}, 0, nullptr, 0, nullptr);
}

void SimpleShaderPipeline::createColorOnlyRenderPass()
{
    vk::AttachmentDescription colorAttachment(
        {}, framebufferFormat, vk::SampleCountFlagBits::e1,
        vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eStore,
        vk::AttachmentLoadOp::eDontCare, vk::AttachmentStoreOp::eDontCare,
        vk::ImageLayout::eUndefined, vk::ImageLayout::ePresentSrcKHR);

    vk::AttachmentReference colorAttachmentRef(0, vk::ImageLayout::eColorAttachmentOptimal);
    vk::SubpassDescription subpass({}, vk::PipelineBindPoint::eGraphics, 0, nullptr, 1, &colorAttachmentRef, nullptr, nullptr, 0, nullptr);
    vk::SubpassDependency dependency(VK_SUBPASS_EXTERNAL, 0,
        vk::PipelineStageFlagBits::eColorAttachmentOutput, vk::PipelineStageFlagBits::eColorAttachmentOutput,
        {}, vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite);
    vk::RenderPassCreateInfo renderPassInfo({}, 1, &colorAttachment, 1, &subpass, 1, &dependency);
    renderPass = device.createRenderPass(renderPassInfo);
}

void SimpleShaderPipeline::createColorDepthRenderPass()
{
    vk::AttachmentDescription colorAttachment(
        {},
        framebufferFormat,
        vk::SampleCountFlagBits::e1,
        vk::AttachmentLoadOp::eClear,
        vk::AttachmentStoreOp::eStore,
        vk::AttachmentLoadOp::eDontCare,
        vk::AttachmentStoreOp::eDontCare,
        vk::ImageLayout::eUndefined,
        vk::ImageLayout::ePresentSrcKHR);

    vk::AttachmentDescription depthAttachment(
        {},
        depthStencilFormat,
        vk::SampleCountFlagBits::e1,
        vk::AttachmentLoadOp::eClear,
        vk::AttachmentStoreOp::eStore,
        vk::AttachmentLoadOp::eDontCare,
        vk::AttachmentStoreOp::eDontCare,
        vk::ImageLayout::eUndefined,
        vk::ImageLayout::eDepthAttachmentOptimal);

    vk::AttachmentReference colorAttachmentRef(0, vk::ImageLayout::eColorAttachmentOptimal);
    vk::AttachmentReference depthAttachmentRef(1, vk::ImageLayout::eDepthAttachmentOptimal);
    vk::SubpassDescription subpass(
        {},
        vk::PipelineBindPoint::eGraphics,
        0,
        nullptr,
        1,
        &colorAttachmentRef,
        nullptr,
        &depthAttachmentRef,
        0,
        nullptr);

    vk::SubpassDependency subDependency(
        VK_SUBPASS_EXTERNAL,
        0,
        vk::PipelineStageFlagBits::eColorAttachmentOutput,
        vk::PipelineStageFlagBits::eColorAttachmentOutput,
        {},
        vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite,
        {});

    std::vector<vk::AttachmentDescription> attachments{ colorAttachment,depthAttachment };
    vk::RenderPassCreateInfo renderpassInfo(
        {},
        static_cast<uint32_t>(attachments.size()),
        attachments.data(),
        1,
        &subpass,
        1,
        &subDependency);

    renderPass = device.createRenderPass(renderpassInfo);
}

void SimpleShaderPipeline::createDefaultVFShader(const std::string& vertexShaderPath, const std::string& fragmentShaderPath, const vk::PipelineVertexInputStateCreateInfo vertexInput, const vk::PipelineLayoutCreateInfo pipelineLayoutInfo)
{
    vk::ShaderModule vertexShaderModule = createShaderModule(vertexShaderPath);
    vk::ShaderModule fragmentShaderModule = createShaderModule(fragmentShaderPath);
    vk::PipelineShaderStageCreateInfo vertexShaderStageInfo({}, vk::ShaderStageFlagBits::eVertex, vertexShaderModule, "main");
    vk::PipelineShaderStageCreateInfo fragmentShaderStageInfo({}, vk::ShaderStageFlagBits::eFragment, fragmentShaderModule, "main");
    shaderModules.push_back(vertexShaderModule);
    shaderModules.push_back(fragmentShaderModule);
    shaderStages.push_back(vertexShaderStageInfo);
    shaderStages.push_back(fragmentShaderStageInfo);

    vertexInputInfo = vertexInput;
    pipelineLayoutCreateInfo = pipelineLayoutInfo;
    pipelineLayout = device.createPipelineLayout(pipelineLayoutCreateInfo);
    createPipeline();
}

void SimpleShaderPipeline::createPipeline()
{
    vk::GraphicsPipelineCreateInfo pipelineInfo(
        {}, static_cast<uint32_t>(shaderStages.size()), shaderStages.data(),
        &vertexInputInfo, &inputAssemblyInfo, nullptr,
        &viewportInfo, &rasterizer, &multisampleInfo,
        nullptr, &colorBlendStateInfo, nullptr, pipelineLayout, renderPass, 0);

    pipeline = device.createGraphicsPipeline({}, pipelineInfo);
}

void SimpleShaderPipeline::destroy()
{
    for (const auto i : shaderModules)
    {
        device.destroyShaderModule(i);
    }
    device.destroyPipeline(pipeline);
    device.destroyPipelineLayout(pipelineLayout);
    device.destroyRenderPass(renderPass);
}

vk::ShaderModule SimpleShaderPipeline::createShaderModule(const std::string& filePath)
{
    std::ifstream file(filePath, std::ios::binary);
    std::vector<char> fileStr{ std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>() };
    file.close();
    vk::ShaderModuleCreateInfo createInfo({}, fileStr.size(), reinterpret_cast<const uint32_t*>(fileStr.data()));
    return device.createShaderModule(createInfo);
}
