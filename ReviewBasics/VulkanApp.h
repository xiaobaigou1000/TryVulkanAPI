#pragma once
#include"NativeWindow.h"
#include"VulkanContext.h"
#include"EasyUseSwapChain.h"
#include"SimpleShaderPipeline.h"
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include<glm.hpp>
#include<gtc/matrix_transform.hpp>

class VulkanApp
{
protected:
    struct SimpleUniformObject
    {
        glm::mat4 model;
        glm::mat4 view;
        glm::mat4 project;
    };

    struct Vertex
    {
        glm::vec3 position;
        glm::vec3 color;
        glm::vec2 texCoord;

        static vk::VertexInputBindingDescription getBindingDescription()
        {
            vk::VertexInputBindingDescription bindingDescription{ 0,sizeof(Vertex),vk::VertexInputRate::eVertex };
            return bindingDescription;
        }

        static std::array<vk::VertexInputAttributeDescription, 3> getAttributeDescription()
        {
            vk::VertexInputAttributeDescription vertexPosition{ 0,0,vk::Format::eR32G32B32Sfloat,offsetof(Vertex,position) };
            vk::VertexInputAttributeDescription vertexColor{ 1,0,vk::Format::eR32G32B32A32Sfloat,offsetof(Vertex,color) };
            vk::VertexInputAttributeDescription VertexTexCoord{ 2,0,vk::Format::eR32G32Sfloat,offsetof(Vertex,texCoord) };
            return { vertexPosition,vertexColor,VertexTexCoord };
        }
    };
public:
    void run();
    void init();
    void cleanup();

private:
    void userInit();
    void userLoopFunc();
    void userDestroy();

    NativeWindow window;
    VulkanContext context;
    EasyUseSwapChain swapChain;
    vk::SwapchainKHR swapChainHandle;
    std::vector<vk::Image> swapChainImages;
    std::vector<vk::ImageView> swapChainImageViews;
    vk::Device device;
    vk::Queue graphicsQueue;//be able to present image
    uint32_t findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties);
    std::tuple<vk::Buffer, vk::DeviceMemory> createBuffer(vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties);
    void copyBuffer(vk::Buffer src, vk::Buffer dst, vk::DeviceSize size);
    vk::CommandBuffer beginSingleTimeCommand();
    void endSingleTimeCommand(vk::CommandBuffer command);
    void transitionImageLayout(vk::Image image, vk::Format format, vk::ImageLayout oldLayout, vk::ImageLayout newLayout);
    void copyBufferToImage(vk::Buffer buffer, vk::Image image, uint32_t width, uint32_t height);

    //code here
    SimpleShaderPipeline shader;
    vk::DescriptorSetLayout descriptorSetLayout;

    std::vector<vk::Framebuffer> swapChainColorOnlyFramebuffers;
    vk::CommandPool commandPool;
    std::vector<vk::CommandBuffer> commandBuffers;
    std::vector<vk::Semaphore> imageAvailableSemaphore;
    std::vector<vk::Semaphore> renderFinishedSemaphore;
    std::vector<vk::Fence> inFlightFences;
    uint32_t max_images_in_flight = 0;
    uint32_t current_frame = 0;


    constexpr static std::array<Vertex, 8> vertices{
        Vertex{glm::vec3{-0.5f,-0.5f,0.0f},glm::vec3{1.0f,0.0f,0.0f},glm::vec2{1.0f,0.0f}},
        Vertex{glm::vec3{0.5f,-0.5f,0.0f},glm::vec3{0.0f,1.0f,0.0f},glm::vec2{0.0f,0.0f}},
        Vertex{glm::vec3{0.5f,0.5f,0.0f},glm::vec3{0.0f,0.0f,1.0f},glm::vec2{0.0f,1.0f}},
        Vertex{glm::vec3{-0.5f,0.5f,0.0f},glm::vec3{1.0f,1.0f,1.0f},glm::vec2{1.0f,1.0f}},

        Vertex{glm::vec3{-0.5f,-0.5f,-0.5f},glm::vec3{1.0f,0.0f,0.0f},glm::vec2{1.0f,0.0f}},
        Vertex{glm::vec3{0.5f,-0.5f,-0.5f},glm::vec3{0.0f,1.0f,0.0f},glm::vec2{0.0f,0.0f}},
        Vertex{glm::vec3{0.5f,0.5f,-0.5f},glm::vec3{0.0f,0.0f,1.0f},glm::vec2{0.0f,1.0f}},
        Vertex{glm::vec3{-0.5f,0.5f,-0.5f},glm::vec3{1.0f,1.0f,1.0f},glm::vec2{1.0f,1.0f}}
    };

    constexpr static std::array<uint32_t, 12> indices{
        0, 1, 2, 2, 3, 0,
        4, 5, 6, 6, 7, 4
    };

    vk::Buffer vertexBuffer;
    vk::DeviceMemory vertexBufferMemory;
    vk::Buffer indexBuffer;
    vk::DeviceMemory indexBufferMemory;
    std::vector<vk::Buffer> uniformBuffers;
    std::vector<vk::DeviceMemory> uniformBufferMemorys;
    void updateUniformBuffer(uint32_t imageIndex);

    vk::DescriptorPool descriptorPool;
    std::vector<vk::DescriptorSet> descriptorSets;

    vk::Image textureImage;
    vk::DeviceMemory textureImageMemory;
    vk::ImageView textureImageView;
    vk::Sampler textureSampler;

    vk::Image depthImage;
    vk::DeviceMemory depthImageMemory;
    vk::ImageView depthImageView;

    void mainLoop();
};