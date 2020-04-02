#pragma once
#define GLFW_INCLUDE_VULKAN
#include<GLFW/glfw3.h>
#include<vulkan/vulkan.hpp>
#include<glm.hpp>
#include<gtc/matrix_transform.hpp>
#include<iostream>
#include<vector>
#include<functional>
#include<array>
#include<fstream>
#include<chrono>

class HelloTriangleApplication
{
protected:
    struct UniformBufferObject
    {
        glm::mat4 model;
        glm::mat4 view;
        glm::mat4 proj;
    };

    struct Vertex
    {
        glm::vec2 position;
        glm::vec3 color;

        static vk::VertexInputBindingDescription getBindingDescription()
        {
            return vk::VertexInputBindingDescription{ 0,sizeof(Vertex),vk::VertexInputRate::eVertex };
        }

        static std::array<vk::VertexInputAttributeDescription, 2> getAttributeDescriptions()
        {
            return std::array<vk::VertexInputAttributeDescription, 2>{
                vk::VertexInputAttributeDescription{ 0,0,vk::Format::eR32G32Sfloat,offsetof(Vertex,position) },
                    vk::VertexInputAttributeDescription{ 1,0,vk::Format::eR32G32B32Sfloat,offsetof(Vertex,color) }
            };
        }
    };

    struct QueueFamilyIndices
    {
        uint32_t graphicsFamily;
        uint32_t presentFamily;
    };

    struct SwapChainSupportDetails
    {
        vk::SurfaceCapabilitiesKHR capabilities;
        std::vector<vk::SurfaceFormatKHR> formats;
        std::vector<vk::PresentModeKHR> presentModes;
    };
public:
    void run();
private:
    constexpr static std::array<Vertex, 4> vertices = {
    Vertex{glm::vec2{-0.5f,-0.5f}, glm::vec3{1.0f, 1.0f, 1.0f}},
    Vertex{glm::vec2{ 0.5f,-0.5f}, glm::vec3{0.0f, 1.0f, 0.0f}},
    Vertex{glm::vec2{ 0.5f, 0.5f}, glm::vec3{0.0f, 0.0f, 1.0f}},
    Vertex{glm::vec2{-0.5f, 0.5f}, glm::vec3{1.0f, 0.0f, 0.0f}},
    };

    constexpr static std::array<uint32_t, 6> indices = {
        0, 1, 2, 2, 3, 0
    };

    GLFWwindow* window;
    vk::Instance instance;
    vk::DebugUtilsMessengerEXT debugMessenger;
    vk::DebugUtilsMessengerCreateInfoEXT debugMessengerCreateInfo;
    vk::PhysicalDevice physicalDevice;
    vk::Device device;
    vk::Queue graphicsQueue;
    vk::Queue presentQueue;
    vk::RenderPass renderPass;
    vk::DescriptorSetLayout descriptorSetLayout;
    vk::PipelineLayout pipelineLayout;
    vk::Pipeline graphicsPipeline;

    vk::Buffer vertexBuffer;
    vk::DeviceMemory vertexBufferMemory;
    vk::Buffer indexBuffer;
    vk::DeviceMemory indexBufferMemory;

    std::vector<vk::Buffer> uniformBuffers;
    std::vector<vk::DeviceMemory> uniformBuffersMemory;

    vk::DescriptorPool descriptorPool;
    std::vector<vk::DescriptorSet> descriptorSets;

    vk::SurfaceKHR surface;
    vk::SwapchainKHR swapChain;
    std::vector<vk::Image> swapChainImages;
    std::vector<vk::ImageView> swapChainImageViews;
    std::vector<vk::Framebuffer> swapChainFrameBuffers;
    vk::Format swapChainImageFormat;
    vk::Extent2D swapChainExtent;

    vk::CommandPool commandPool;
    std::vector<vk::CommandBuffer> commandBuffers;

    std::vector<vk::Semaphore> imageAvailableSemaphore;
    std::vector<vk::Semaphore> renderFinishedSemaphore;
    std::vector<vk::Fence> inFlightFences;

    uint32_t width = 800;
    uint32_t height = 600;
    constexpr static int MAX_FRAMES_IN_FLIGHT = 2;
    uint32_t currentFrame = 0;

    std::vector<const char*> physicalDeviceExtensions{ VK_KHR_SWAPCHAIN_EXTENSION_NAME };
    std::vector<const char*> validationLayers{ "VK_LAYER_KHRONOS_validation" };
#ifdef NDEBUG
    const bool enableValidationLayers = false;
#else
    const bool enableValidationLayers = true;
#endif

    void initWindow();
    void initVulkan();
    void mainLoop();
    void cleanup();

    void createInstance();
    bool checkValidationLayerSupport();
    void checkPhysicalDeviceExtensionSupport();
    void setupDebugMessenger();
    void pickPhysicalDevice();
    void createLogicalDevice();
    void createSurface();
    void createSwapChain();
    void createImageViews();
    void createRenderPass();
    void createGraphicsPipeline();
    void createFrameBuffers();
    void createCommandPool();
    void createCommandBuffers();
    void drawFrame();
    void createSyncObjects();
    void createVertexBuffer();
    void createIndexBuffer();
    void createDescriptionSetLayout();
    void createUniformBuffers();
    void createDescriptorPool();
    void createDescriptorSets();

    void updateUniformBuffer(uint32_t currentImage);
    void copyBuffer(vk::Buffer src, vk::Buffer dst, vk::DeviceSize size);
    std::tuple<vk::Buffer, vk::DeviceMemory> createBufferHelpFunc(vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties);
    static std::vector<char> readShaderCode(const std::string& fileName);
    static vk::SurfaceFormatKHR chooseSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& availableFormats);
    static vk::PresentModeKHR chooseSwapPresentMode(const std::vector<vk::PresentModeKHR>& availablePresentModes);
    vk::ShaderModule createShaderModule(const std::vector<char>& code);
    vk::Extent2D chooseSwapExtent(const vk::SurfaceCapabilitiesKHR& capabilities);
    SwapChainSupportDetails querySwapChainSupport();
    vk::DebugUtilsMessengerCreateInfoEXT getDebugMessengerCreateInfo();
    std::vector<const char*> getRequiredExtensions();
    QueueFamilyIndices findQueueFamilies();
    uint32_t findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties);

    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageType,
        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void* pUserData);
};
