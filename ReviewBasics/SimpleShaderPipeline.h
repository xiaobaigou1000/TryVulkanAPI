#pragma once
#include<vulkan/vulkan.hpp>
#include<vector>

class SimpleShaderPipeline
{
public:
    void init(const vk::Device device, const vk::Extent2D windowExtent);
    void createDefaultVFShader(const std::string& vertexShaderPath, const std::string& fragmentShaderPath,
        const vk::PipelineVertexInputStateCreateInfo vertexInput, const vk::PipelineLayoutCreateInfo pipelineLayout);
    void destroy();
private:
    vk::Device device;
    vk::PipelineLayout pipelineLayout;
    vk::Pipeline pipeline;

    std::vector<vk::ShaderModule> shaderModules;
    std::vector<vk::PipelineShaderStageCreateInfo> shaderStages;
    vk::PipelineVertexInputStateCreateInfo vertexInputInfo;
    vk::Viewport viewport;
    vk::Rect2D scissor;
    vk::PipelineInputAssemblyStateCreateInfo inputAssemblyInfo;
    vk::PipelineMultisampleStateCreateInfo multisampleInfo;
    vk::PipelineRasterizationStateCreateInfo rasterizer;
    vk::PipelineColorBlendAttachmentState colorBlendAttachmentInfo;
    vk::PipelineColorBlendStateCreateInfo colorBlendStateInfo;
    vk::PipelineLayoutCreateInfo pipelineLayoutCreateInfo;

    vk::ShaderModule createShaderModule(const std::string& filePath);
};

