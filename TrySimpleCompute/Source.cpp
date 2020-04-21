#include<iostream>
#include<vulkan/vulkan.hpp>
#include<fstream>
#include<iomanip>
#include<opencv2/core.hpp>
#include<armadillo>
#include<chrono>
#include<thread>
#include<random>
#include<sstream>
#include<fmt/format.h>
#include"SimpleComputeContext.h"

class MyComputeProgram :protected SimpleComputeContext
{
private:
    arma::mat A;
    arma::mat b;
    arma::mat solutionX;

    vk::Buffer matrixABuffer;
    vk::DeviceMemory matrixBufferMemory;

    vk::Buffer vectorBBuffer;
    vk::DeviceMemory vectorBBufferMemory;

    vk::Buffer assumeX0Buffer;
    vk::DeviceMemory assumeX0BufferMemory;

    vk::Buffer calculatedXBuffer;
    vk::DeviceMemory calculatedXBufferMemory;

    vk::DescriptorSetLayout descriptorSetLayout;
    vk::PipelineLayout pipelineLayout;

    vk::Pipeline computePipeline;

    vk::DescriptorPool descriptorPool;
    vk::DescriptorSet descriptorSet;

    std::vector<vk::CommandBuffer> commandBuffers;

    int calcRounds = 1;
    std::vector<vk::Semaphore> computeFinishedSemaphores;
    std::vector<vk::Semaphore> bufferCopyFinishedSemaphores;
    //vk::Fence fence;

    std::tuple<vk::Buffer, vk::DeviceMemory> sendMatrixToGPU(const arma::mat& matToSend, vk::BufferUsageFlags usage)
    {
        vk::DeviceSize matSize = matToSend.n_elem * sizeof(double);
        auto result = createHostBuffer(matSize, usage);
        void* dataptr = device.mapMemory(std::get<1>(result), 0, matSize);
        const double* memptr = matToSend.memptr();
        memcpy(dataptr, memptr, matSize);
        device.unmapMemory(std::get<1>(result));
        return result;
    }
public:
    MyComputeProgram()
    {
    }

    ~MyComputeProgram()
    {
    }

    void init()
    {
        //init matrix data
        std::default_random_engine dre(std::chrono::system_clock::now().time_since_epoch().count());
        std::uniform_real_distribution<double> uid(0.1, 20.0);

        A = arma::mat(10, 10);
        solutionX = arma::mat(10, 1);
        A.imbue([&dre, uid] {return uid(dre); });
        for (int i = 0; i < A.n_rows; i++)
        {
            auto row = A.row(i);
            double temp = 0.0;
            for (auto k = row.begin(); k != row.end(); k++)
            {
                temp += *k;
            }
            row[i] = temp + 3.0;
        }
        solutionX.imbue([&dre, uid] {return uid(dre); });
        b = A * solutionX;
        arma::mat assumeX(b.n_elem, 1);
        assumeX.imbue([&dre, uid] {return 10; });

        //print equation
        fmt::print("equation :\n");
        for (size_t i = 0; i < b.n_elem; i++)
        {
            auto rowOfA = A.row(i);
            fmt::print("{:>10.4f} * x0", rowOfA[0]);
            for (size_t j = 1; j < rowOfA.n_elem; j++)
            {
                fmt::print(" + {:>10.4f} * x{}", rowOfA[j], j);
            }
            fmt::print(" = {:>10.4f}\n", b[i]);
        }

        //cpu calculation for debug

        //for (int fun = 0; fun < 5 * b.n_elem; fun++)
        //{
        //    arma::mat _result(10, 1);
        //    for (auto i = 0; i < b.n_elem; i++)
        //    {
        //        auto rowOfA = A.row(i);
        //        double temp = 0.0;
        //        for (int k = 0; k < rowOfA.n_elem; k++)
        //        {
        //            temp += rowOfA[k] * assumeX[k];
        //        }
        //        temp -= rowOfA[i] * assumeX[i];
        //        temp = (b[i] - temp) / rowOfA[i];
        //        fmt::print("{:.4f}\n", temp);
        //        _result[i] = temp;
        //    }
        //    assumeX = _result;
        //    std::cout << '\n';
        //}

        std::tie(matrixABuffer, matrixBufferMemory) = sendMatrixToGPU(A, vk::BufferUsageFlagBits::eStorageBuffer);
        std::tie(vectorBBuffer, vectorBBufferMemory) = sendMatrixToGPU(b, vk::BufferUsageFlagBits::eStorageBuffer);
        vk::DeviceSize sizeOfx = b.n_elem * sizeof(double);
        std::tie(calculatedXBuffer, calculatedXBufferMemory) = createHostBuffer(sizeOfx, vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferSrc);
        std::tie(assumeX0Buffer, assumeX0BufferMemory) = sendMatrixToGPU(assumeX, vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst);

        std::vector<vk::DescriptorSetLayoutBinding> uniformBindings;
        uniformBindings.push_back({ 0, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute });
        uniformBindings.push_back({ 1, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute });
        uniformBindings.push_back({ 2, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute });
        uniformBindings.push_back({ 3, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute });
        vk::DescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo({}, 4, uniformBindings.data());
        descriptorSetLayout = device.createDescriptorSetLayout(descriptorSetLayoutCreateInfo);
        vk::PushConstantRange pushConstRange(vk::ShaderStageFlagBits::eCompute, 0, 4);
        vk::PipelineLayoutCreateInfo pipelineLayoutCreateInfo({}, 1, &descriptorSetLayout, 1, &pushConstRange);
        pipelineLayout = device.createPipelineLayout(pipelineLayoutCreateInfo);

        //load compute shader stage
        std::ifstream file("./shaders/jacobi.spv", std::ios::binary);
        std::vector<char> fileStr{ std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>() };
        file.close();
        vk::ShaderModuleCreateInfo shaderModuleCreateInfo({}, fileStr.size(), reinterpret_cast<const uint32_t*>(fileStr.data()));
        vk::ShaderModule computeShaderModule = device.createShaderModule(shaderModuleCreateInfo);
        vk::PipelineShaderStageCreateInfo computeStageInfo({}, vk::ShaderStageFlagBits::eCompute, computeShaderModule, "main");

        vk::ComputePipelineCreateInfo pipelineCreateInfo({}, computeStageInfo, pipelineLayout);
        computePipeline = device.createComputePipeline({}, pipelineCreateInfo);
        device.destroyShaderModule(computeShaderModule);

        vk::DescriptorPoolSize descriptorPoolSize(vk::DescriptorType::eStorageBuffer, 4);
        vk::DescriptorPoolCreateInfo descriptorPoolInfo({}, 1, 1, &descriptorPoolSize);
        descriptorPool = device.createDescriptorPool(descriptorPoolInfo);
        vk::DescriptorSetAllocateInfo descriptorSetAllocateInfo(descriptorPool, 1, &descriptorSetLayout);
        descriptorSet = device.allocateDescriptorSets(descriptorSetAllocateInfo).front();

        vk::DescriptorBufferInfo mataBindInfo{ matrixABuffer,0,VK_WHOLE_SIZE };
        vk::DescriptorBufferInfo vecbBindInfo{ vectorBBuffer,0,VK_WHOLE_SIZE };
        vk::DescriptorBufferInfo assumeXBindInfo{ assumeX0Buffer,0,VK_WHOLE_SIZE };
        vk::DescriptorBufferInfo resultBufferBindInfo{ calculatedXBuffer,0,VK_WHOLE_SIZE };

        std::vector<vk::WriteDescriptorSet> descriptorSetWrites;
        descriptorSetWrites.push_back({ descriptorSet,0,0,1,vk::DescriptorType::eStorageBuffer,nullptr,&mataBindInfo });
        descriptorSetWrites.push_back({ descriptorSet,1,0,1,vk::DescriptorType::eStorageBuffer,nullptr,&vecbBindInfo });
        descriptorSetWrites.push_back({ descriptorSet,2,0,1,vk::DescriptorType::eStorageBuffer,nullptr,&assumeXBindInfo });
        descriptorSetWrites.push_back({ descriptorSet,3,0,1,vk::DescriptorType::eStorageBuffer,nullptr,&resultBufferBindInfo });
        device.updateDescriptorSets(descriptorSetWrites, {});

        vk::CommandBufferAllocateInfo commandAllocInfo(commandPool, vk::CommandBufferLevel::ePrimary, 2);
        commandBuffers = device.allocateCommandBuffers(commandAllocInfo);
        vk::CommandBufferBeginInfo beginInfo(vk::CommandBufferUsageFlagBits::eSimultaneousUse);
        commandBuffers[0].begin(beginInfo);
        commandBuffers[0].bindPipeline(vk::PipelineBindPoint::eCompute, computePipeline);
        commandBuffers[0].bindDescriptorSets(vk::PipelineBindPoint::eCompute, pipelineLayout, 0, descriptorSet, {});
        int32_t pushConst = b.n_elem;
        commandBuffers[0].pushConstants<uint32_t>(pipelineLayout, vk::ShaderStageFlagBits::eCompute, 0, pushConst);
        commandBuffers[0].dispatch(b.n_elem, 1, 1);
        commandBuffers[0].end();

        commandBuffers[1].begin(beginInfo);
        vk::BufferCopy copyRange(0, 0, b.n_elem * sizeof(double));
        commandBuffers[1].copyBuffer(calculatedXBuffer, assumeX0Buffer, copyRange);
        commandBuffers[1].end();

        calcRounds = 5 * b.n_elem;
        computeFinishedSemaphores.resize(calcRounds);
        bufferCopyFinishedSemaphores.resize(calcRounds);

        for (int i = 0; i < calcRounds; i++)
        {
            computeFinishedSemaphores[i] = device.createSemaphore({});
            bufferCopyFinishedSemaphores[i] = device.createSemaphore({});
        }
    }

    void run()
    {
        vk::SubmitInfo computeSubmitInfo(0, nullptr, nullptr, 1, &commandBuffers[0], 1, &computeFinishedSemaphores[0]);
        vk::PipelineStageFlags bufferCopyStage = vk::PipelineStageFlagBits::eAllCommands;
        vk::SubmitInfo bufferCopySubmitInfo(1, &computeFinishedSemaphores[0], &bufferCopyStage, 1, &commandBuffers[0], 1, &bufferCopyFinishedSemaphores[0]);
        queue.submit(computeSubmitInfo, {});
        queue.submit(bufferCopySubmitInfo, {});

        for (int i = 1; i < calcRounds; i++)
        {
            vk::PipelineStageFlags computeSubmitStage = vk::PipelineStageFlagBits::eAllCommands;
            vk::SubmitInfo computeSubmitInfo(1, &bufferCopyFinishedSemaphores[i - 1], &computeSubmitStage, 1, &commandBuffers[0], 1, &computeFinishedSemaphores[i]);
            vk::PipelineStageFlags bufferCopyStage = vk::PipelineStageFlagBits::eAllCommands;
            vk::SubmitInfo bufferCopySubmitInfo(1, &computeFinishedSemaphores[i], &bufferCopyStage, 1, &commandBuffers[1], 1, &bufferCopyFinishedSemaphores[i]);
            queue.submit(computeSubmitInfo, {});
            queue.submit(bufferCopySubmitInfo, {});
        }

        queue.waitIdle();

        //used to find bug

        //void* ptr = device.mapMemory(calculatedXBufferMemory, 0, VK_WHOLE_SIZE);
        //void* dstPtr = device.mapMemory(assumeX0BufferMemory, 0, VK_WHOLE_SIZE);
        //for (int i = 0; i < calcRounds; i++)
        //{
        //    vk::SubmitInfo computeSubmit;
        //    computeSubmit.commandBufferCount = 1;
        //    computeSubmit.pCommandBuffers = &commandBuffers[0];
        //    queue.submit(computeSubmit, fence);
        //    queue.waitIdle();
        //    device.waitForFences(fence, VK_TRUE, 0xFFFFFFFF);
        //    device.resetFences(fence);
        //    memcpy(dstPtr, ptr, b.n_elem * sizeof(double));
        //}
        //device.unmapMemory(calculatedXBufferMemory);
        //device.unmapMemory(assumeX0BufferMemory);

        void* dataptr = device.mapMemory(calculatedXBufferMemory, 0, VK_WHOLE_SIZE);
        arma::mat resultMat(b.n_elem, 1);
        memcpy(resultMat.memptr(), dataptr, b.n_elem * sizeof(double));
        device.unmapMemory(calculatedXBufferMemory);
        std::cout << "result :\n" << resultMat;
        resultMat = arma::solve(A, b);
        std::cout << "real result :\n" << resultMat;
    }

    void destroy()
    {
        //clean up
        auto destroyBuffer = [this](auto buffer, auto memory)
        {
            device.destroyBuffer(buffer);
            device.freeMemory(memory);
        };

        destroyBuffer(matrixABuffer, matrixBufferMemory);
        destroyBuffer(vectorBBuffer, vectorBBufferMemory);
        destroyBuffer(assumeX0Buffer, assumeX0BufferMemory);
        destroyBuffer(calculatedXBuffer, calculatedXBufferMemory);
        device.destroyDescriptorSetLayout(descriptorSetLayout);
        device.destroyDescriptorPool(descriptorPool);
        device.destroyPipeline(computePipeline);
        device.destroyPipelineLayout(pipelineLayout);
        for (auto i : computeFinishedSemaphores)
        {
            device.destroySemaphore(i);
        }
        for (auto i : bufferCopyFinishedSemaphores)
        {
            device.destroySemaphore(i);
        }
    }
};

int main()
{
    MyComputeProgram program;
    program.init();
    program.run();
    program.destroy();
    return 0;
}