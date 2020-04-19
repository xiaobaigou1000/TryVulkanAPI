#pragma once
#include"VulkanApp.h"

class MyVulkanApp :public VulkanApp
{
private:
    uint32_t max_images_in_flight;
    uint32_t current_frame;

    virtual void userInit()override;
    virtual void userLoopFunc()override;
    virtual void userDestroy()override;
};

