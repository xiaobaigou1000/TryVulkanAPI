#include<iostream>
#include"VulkanApp.h"

int main()
{
    VulkanApp app;
    app.init();
    app.run();
    app.cleanup();
    return 0;
}