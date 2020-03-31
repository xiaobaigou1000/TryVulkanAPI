#include"HelloTriangle.h"

int main()
{
    HelloTriangleApplication app;
    try
    {
        app.run();
    }
    catch (std::exception e)
    {
        std::cerr << e.what() << '\n';
        return -1;
    }
    return 0;
}