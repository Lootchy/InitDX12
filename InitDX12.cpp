#include <d3d12.h>     
#include "d3dApp.h"



int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE prevInstance, PSTR cmdLine, int showCmd)
{
    try
    {
        d3dApp app(hInstance);
        if (!app.Initialize())
            return -1;

        return app.Run();
    }
    catch (const std::exception& e)
    {
        
        return -1;
    }
}


