#include<iostream>
#include<vulkan/vulkan.hpp>
#include<fstream>
#include<iomanip>
#include<opencv2/core.hpp>
#include<armadillo>
#include<chrono>
#include<thread>
#include"SimpleComputeContext.h"

class MyComputeProgram :protected SimpleComputeContext
{
private:
    uint32_t vectorSize;

    vk::Buffer dlInvBuffer;
    vk::DeviceMemory dlInvMemory;

    vk::Buffer uMat;
    vk::DeviceMemory uMatMemory;

    vk::Buffer xkab;
    vk::DeviceMemory xkabMemory;

    vk::Buffer resultBuffer;
    vk::DeviceMemory resultBuffermemory;

    vk::DescriptorSetLayout descriptorSetLayout;
    vk::PipelineLayout pipelineLayout;

    vk::Pipeline computePipeline;
    vk::DescriptorPool descriptorPool;
    vk::DescriptorSet descriptorSet;
    vk::CommandBuffer command;
    vk::SubmitInfo submitInfo;
    vk::Fence fence;

public:
    MyComputeProgram()
        :vectorSize(0)
    {
    }

    ~MyComputeProgram()
    {
        destroy();
    }

    void init()
    {
        arma::Mat<float> inputMat(2, 2);
        inputMat(0, 0) = 16;
        inputMat(0, 1) = 3;
        inputMat(1, 0) = 7;
        inputMat(1, 1) = -11;

        std::cout << "origin matrix A :\n" << inputMat << std::endl;

        float b[2];
        b[0] = 11;
        b[1] = 13;

        std::cout << "origin vector b :\n" << b[0] << '\n' << b[1] << '\n';

        float x0[2];
        x0[0] = 1.0;
        x0[1] = 1.0;

        vectorSize = 2;

        std::cout << "choosen vector x0 :\n" << x0[0] << '\n' << x0[1] << '\n';

        arma::Mat<float> L = arma::trimatl(inputMat);
        for (int i = 0; i < L.n_rows; i++)
            L(i, i) = 0;
        arma::Mat<float> U = arma::trimatu(inputMat);
        for (int i = 0; i < U.n_rows; i++)
            U(i, i) = 0;
        arma::Mat<float> D = arma::diagmat(inputMat);
        arma::Mat<float> DLInv = (D + L).i();

        std::tie(dlInvBuffer, dlInvMemory) = createHostBuffer(
            DLInv.n_elem * sizeof(float),
            vk::BufferUsageFlagBits::eStorageBuffer);

        void* data = device.mapMemory(dlInvMemory, 0, VK_WHOLE_SIZE);
        memcpy(data, DLInv.memptr(), DLInv.n_elem * sizeof(float));
        device.unmapMemory(dlInvMemory);


        std::tie(uMat, uMatMemory) = createHostBuffer(
            U.n_elem * sizeof(float),
            vk::BufferUsageFlagBits::eStorageBuffer);
        data = device.mapMemory(uMatMemory, 0, VK_WHOLE_SIZE);
        memcpy(data, U.memptr(), U.n_elem * sizeof(float));
        device.unmapMemory(uMatMemory);


        std::tie(xkab, xkabMemory) = createHostBuffer(
            sizeof(x0) + sizeof(b),
            vk::BufferUsageFlagBits::eStorageBuffer);
        data = device.mapMemory(xkabMemory, 0, VK_WHOLE_SIZE);
        void* bPtr = (float*)data + sizeof(x0) / sizeof(float);
        memcpy(data, x0, sizeof(x0));
        memcpy(bPtr, b, sizeof(b));
        device.unmapMemory(xkabMemory);

        std::tie(resultBuffer, resultBuffermemory) = createHostBuffer(
            sizeof(x0),
            vk::BufferUsageFlagBits::eStorageBuffer);

        vk::DescriptorSetLayoutBinding dlInvBinding(0, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute, nullptr);
        vk::DescriptorSetLayoutBinding utMatBinding(1, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute);
        vk::DescriptorSetLayoutBinding xkabBinding(2, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute);
        vk::DescriptorSetLayoutBinding resultBinding(3, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute);
        std::vector<vk::DescriptorSetLayoutBinding> uniformBindings{ dlInvBinding,utMatBinding,xkabBinding,resultBinding };
        vk::DescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo({}, uniformBindings.size(), uniformBindings.data());
        descriptorSetLayout = device.createDescriptorSetLayout(descriptorSetLayoutCreateInfo);
        vk::PipelineLayoutCreateInfo pipelineLayoutCreateInfo({}, 1, &descriptorSetLayout, 0, nullptr);
        pipelineLayout = device.createPipelineLayout(pipelineLayoutCreateInfo);

        //load compute shader stage
        std::ifstream file("./Gauss-Seidel.spv", std::ios::binary);
        std::vector<char> fileStr{ std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>() };
        file.close();
        vk::ShaderModuleCreateInfo createInfo({}, fileStr.size(), reinterpret_cast<const uint32_t*>(fileStr.data()));
        vk::ShaderModule computeShaderModule = device.createShaderModule(createInfo);
        vk::PipelineShaderStageCreateInfo computeStageInfo({}, vk::ShaderStageFlagBits::eCompute, computeShaderModule, "main");

        vk::ComputePipelineCreateInfo computePipelineInfo({}, computeStageInfo, pipelineLayout);
        computePipeline = device.createComputePipeline({}, computePipelineInfo);
        device.destroyShaderModule(computeShaderModule);

        //create descriptor pool and update descriptor set
        vk::DescriptorPoolSize poolSize(vk::DescriptorType::eStorageBuffer, 4);
        vk::DescriptorPoolCreateInfo descriptorPoolInfo({}, 1, 1, &poolSize);
        descriptorPool = device.createDescriptorPool(descriptorPoolInfo);
        vk::DescriptorSetAllocateInfo descriptorSetAllocInfo(descriptorPool, 1, &descriptorSetLayout);
        descriptorSet = device.allocateDescriptorSets(descriptorSetAllocInfo).front();

        vk::DescriptorBufferInfo dlInvDescriptorInfo(dlInvBuffer, 0, VK_WHOLE_SIZE);
        vk::DescriptorBufferInfo uMatDescriptorInfo(uMat, 0, VK_WHOLE_SIZE);
        vk::DescriptorBufferInfo xkabDescriptorInfo(xkab, 0, VK_WHOLE_SIZE);
        vk::DescriptorBufferInfo resultDescriptorInfo(resultBuffer, 0, VK_WHOLE_SIZE);
        vk::WriteDescriptorSet dlInvWrite(
            descriptorSet,
            0,
            0,
            1,
            vk::DescriptorType::eStorageBuffer,
            nullptr,
            &dlInvDescriptorInfo,
            nullptr);

        vk::WriteDescriptorSet uMatWrite(
            descriptorSet,
            1,
            0,
            1,
            vk::DescriptorType::eStorageBuffer,
            nullptr,
            &uMatDescriptorInfo,
            nullptr);

        vk::WriteDescriptorSet xkabWrite(
            descriptorSet,
            2,
            0,
            1,
            vk::DescriptorType::eStorageBuffer,
            nullptr,
            &xkabDescriptorInfo,
            nullptr);

        vk::WriteDescriptorSet resultWrite(
            descriptorSet,
            3,
            0,
            1,
            vk::DescriptorType::eStorageBuffer,
            nullptr,
            &resultDescriptorInfo,
            nullptr);

        std::vector<vk::WriteDescriptorSet> descriptorWrites{ dlInvWrite,uMatWrite,xkabWrite,resultWrite };

        device.updateDescriptorSets(descriptorWrites, {});

        vk::CommandBufferAllocateInfo commandInfo(commandPool, vk::CommandBufferLevel::ePrimary, 1);
        command = device.allocateCommandBuffers(commandInfo).front();
        vk::CommandBufferBeginInfo commandBeginInfo({}, nullptr);
        command.begin(commandBeginInfo);
        command.bindPipeline(vk::PipelineBindPoint::eCompute, computePipeline);
        command.bindDescriptorSets(vk::PipelineBindPoint::eCompute, pipelineLayout, 0, descriptorSet, {});
        command.dispatch(vectorSize, 1, 1);
        command.end();

        fence = device.createFence({});

        submitInfo = vk::SubmitInfo(0, nullptr, nullptr, 1, &command, 0, nullptr);

    }

    void run()
    {
        for (uint32_t i = 0; i < 5 * vectorSize; i++)
        {
            queue.submit(submitInfo, fence);
            device.waitForFences(fence, VK_TRUE, 0xFFFFFFFF);
            device.resetFences(fence);

            void* resultPtr = device.mapMemory(resultBuffermemory, 0, VK_WHOLE_SIZE);
            void* xkPtr = device.mapMemory(xkabMemory, 0, VK_WHOLE_SIZE);
            memcpy(xkPtr, resultPtr, vectorSize * sizeof(float));
            device.unmapMemory(resultBuffermemory);
            device.unmapMemory(xkabMemory);
        }
        queue.waitIdle();
        void* result = device.mapMemory(resultBuffermemory, 0, VK_WHOLE_SIZE);
        float trueResult[2];
        memcpy(trueResult, result, vectorSize * sizeof(float));

        std::cout << "result vector :\n" << trueResult[0] << '\n' << trueResult[1] << '\n';
    }

    void destroy()
    {
        //clean up
        destroyBufferAndFreeMemory(dlInvBuffer, dlInvMemory);
        destroyBufferAndFreeMemory(uMat, uMatMemory);
        destroyBufferAndFreeMemory(xkab, xkabMemory);
        destroyBufferAndFreeMemory(resultBuffer, resultBuffermemory);
        device.destroyFence(fence);
        device.destroyPipeline(computePipeline);
        device.destroyDescriptorPool(descriptorPool);
        device.destroyPipelineLayout(pipelineLayout);
        device.destroyDescriptorSetLayout(descriptorSetLayout);
    }
};

int main()
{
    MyComputeProgram program;
    program.init();
    program.run();
    return 0;
}