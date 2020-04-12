#pragma once
#include"NativeWindow.h"
#include"VulkanContext.h"
#include"EasyUseSwapChain.h"
#include"SimpleShaderPipeline.h"
#include<glm.hpp>
#include<gtc/matrix_transform.hpp>

class VulkanApp
{
protected:
    struct Vertex
    {
        glm::vec2 position;
        glm::vec3 color;

        static vk::VertexInputBindingDescription getBindingDescription()
        {
            vk::VertexInputBindingDescription bindingDescription{ 0,sizeof(Vertex),vk::VertexInputRate::eVertex };
            return bindingDescription;
        }

        static std::array<vk::VertexInputAttributeDescription, 2> getAttributeDescription()
        {
            vk::VertexInputAttributeDescription vertexPosition{ 0,0,vk::Format::eR32G32Sfloat,offsetof(Vertex,position) };
            vk::VertexInputAttributeDescription vertexColor{ 1,0,vk::Format::eR32G32B32A32Sfloat,offsetof(Vertex,color) };
            return { vertexPosition,vertexColor };
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
    std::tuple<vk::Buffer, vk::DeviceMemory> createBuffer(vk::DeviceSize size, vk::BufferUsageFlagBits usage, vk::MemoryPropertyFlags properties);

    //code here
    SimpleShaderPipeline shader;
    std::vector<vk::Framebuffer> swapChainColorOnlyFramebuffers;
    vk::CommandPool commandPool;
    std::vector<vk::CommandBuffer> commandBuffers;
    std::vector<vk::Semaphore> imageAvailableSemaphore;
    std::vector<vk::Semaphore> renderFinishedSemaphore;
    std::vector<vk::Fence> inFlightFences;
    uint32_t max_images_in_flight = 0;
    uint32_t current_frame = 0;

    constexpr static std::array<Vertex, 3> vertices{
        Vertex{glm::vec2{0.0f,-0.5f},glm::vec3{1.0f,1.0f,1.0f}},
        Vertex{glm::vec2{0.5f,0.5f},glm::vec3{0.0f,1.0f,0.0f}},
        Vertex{glm::vec2{-0.5f,0.5f},glm::vec3{0.0f,0.0f,1.0f}}
    };

    vk::Buffer vertexBuffer;
    vk::DeviceMemory vertexBufferMemory;

    void mainLoop();
};