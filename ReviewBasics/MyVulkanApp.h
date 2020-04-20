#pragma once
#include<iostream>
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include<glm.hpp>
#include<chrono>
#include"VulkanApp.h"

struct Vertex
{
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 texCoords;
};

class MyVulkanApp :public VulkanApp
{
public:
    struct CameraUniform
    {
        glm::mat4 modelView;
        glm::mat4 projection;
        glm::mat4 MVP;
        glm::mat4 normal;
    };

    struct LightUniform
    {
        glm::vec4 lightPosition;
        glm::vec4 Kd;
        glm::vec4 Ld;
        glm::vec4 Ka;
    };

    struct CameraProperty
    {
        glm::mat4 model;
        glm::mat4 view;
        glm::mat4 projection;
        glm::mat4 normalMat;
    };
private:
    vk::DescriptorSetLayout descriptorSetLayout;
    std::vector<vk::Framebuffer> framebuffers;

    vk::Buffer vertexIndexBuffer;
    vk::DeviceMemory vertexIndexBufferMemory;
    vk::DeviceSize vertexOffset = 0;
    vk::DeviceSize indexOffset = 0;

    vk::Buffer uniformBuffer;
    vk::DeviceMemory uniformBufferMemory;
    vk::DeviceSize lightUniformOffset = 0;
    vk::DeviceSize uniformBufferSize = 0;

    vk::DescriptorPool descriptorPool;
    std::vector<vk::DescriptorSet> descriptorSets;

    std::vector<vk::CommandBuffer> commandBuffers;

    std::vector<vk::Semaphore> imageReadySemaphores;
    std::vector<vk::Semaphore> renderFinishedSemaphores;
    std::vector<vk::Fence> fences;

    CameraProperty camera;

    SimpleShaderPipeline shader;
    uint32_t max_images_in_flight = 1;
    uint32_t current_frame = 0;

    std::chrono::time_point<std::chrono::high_resolution_clock> start_time = std::chrono::high_resolution_clock::now();
    std::chrono::time_point<std::chrono::high_resolution_clock> last_frame;

    virtual void userInit()override;
    virtual void userLoopFunc()override;
    virtual void userDestroy()override;
};

