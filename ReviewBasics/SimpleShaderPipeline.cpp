#include "SimpleShaderPipeline.h"
#include<fstream>

void SimpleShaderPipeline::init(const vk::Device device, const vk::Extent2D windowExtent)
{
    this->device = device;

    vertexInputInfo = vk::PipelineVertexInputStateCreateInfo({}, 0, nullptr, 0, nullptr);
    inputAssemblyInfo = vk::PipelineInputAssemblyStateCreateInfo({}, vk::PrimitiveTopology::eTriangleList, VK_FALSE);
    viewport = vk::Viewport(0, 0, windowExtent.width, windowExtent.height, 0.0f, 1.0f);
    scissor = vk::Rect2D({ 0,0 }, windowExtent);
    rasterizer = vk::PipelineRasterizationStateCreateInfo({}, VK_FALSE, VK_FALSE, vk::PolygonMode::eFill, vk::CullModeFlagBits::eBack, vk::FrontFace::eClockwise, VK_FALSE);
    multisampleInfo = vk::PipelineMultisampleStateCreateInfo({}, vk::SampleCountFlagBits::e1, VK_FALSE);
    colorBlendAttachmentInfo = vk::PipelineColorBlendAttachmentState(VK_FALSE);
    colorBlendAttachmentInfo.setColorWriteMask(vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA);
    colorBlendStateInfo = vk::PipelineColorBlendStateCreateInfo({}, VK_FALSE);
    colorBlendStateInfo.setAttachmentCount(1).setPAttachments(&colorBlendAttachmentInfo);
    pipelineLayoutCreateInfo = vk::PipelineLayoutCreateInfo({}, 0, nullptr, 0, nullptr);
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
}

void SimpleShaderPipeline::destroy()
{
    for (const auto i : shaderModules)
    {
        device.destroyShaderModule(i);
    }
    device.destroyPipelineLayout(pipelineLayout);
}

vk::ShaderModule SimpleShaderPipeline::createShaderModule(const std::string& filePath)
{
    std::ifstream file(filePath, std::ios::binary);
    std::vector<char> fileStr{ std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>() };
    file.close();
    vk::ShaderModuleCreateInfo createInfo({}, fileStr.size(), reinterpret_cast<const uint32_t*>(fileStr.data()));
    return device.createShaderModule(createInfo);
}
