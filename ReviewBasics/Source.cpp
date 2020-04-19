#include<iostream>
#include"MyVulkanApp.h"

int main()
{
    MyVulkanApp app;
    app.init();
    app.run();
    app.cleanup();
    return 0;
}