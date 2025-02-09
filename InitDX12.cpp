#include <d3d12.h>     
#include "d3dApp.h"
#include "Box.h"



int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE prevInstance, PSTR cmdLine, int showCmd)
{
    try
    {
        Box app(hInstance);
        if (!app.Init())
            return -1;

        return app.Run();
    }
    catch (const std::exception& e)
    {
        
        return -1;
    }
}


