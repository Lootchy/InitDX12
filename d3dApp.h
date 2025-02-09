#ifndef D3DAPP_H
#define D3DAPP_H

#include <d3d12.h>
#include <dxgi1_4.h>
#include <wrl.h>
#include <string>
#include <Windows.h>
#include <DirectXMath.h>


class Box;
class d3dApp
{
    
public:
    d3dApp(HINSTANCE hInstance);
    virtual ~d3dApp();

    int Run();
    virtual bool Initialize();
    virtual void Update();
    virtual void Draw();
    virtual void OnResize();

protected:
    bool InitMainWindow();
    bool InitDirect3D();
    void FlushCommandQueue();
    void CreateCommandObjects();
    void CreateSwapChain();
    void CreateRtvAndDsvDescriptorHeaps();
    void CreateRenderTargetViews();

    HINSTANCE mhAppInst;
    HWND      mhMainWnd;
    bool      mAppPaused = false;
    std::wstring mMainWndCaption = L"D3D12 Application";

    ID3D12Device* md3dDevice = nullptr;
    IDXGIFactory4* mdxgiFactory = nullptr;
    ID3D12Fence* mFence;
    ID3D12CommandQueue* mCommandQueue = nullptr;
    ID3D12CommandAllocator* mDirectCmdListAlloc = nullptr;
    ID3D12GraphicsCommandList* mCommandList = nullptr;
    IDXGISwapChain* mSwapChain = nullptr;
    ID3D12DescriptorHeap* mRtvHeap = nullptr;
    ID3D12DescriptorHeap* mDsvHeap = nullptr;
    ID3D12Resource* mSwapChainBuffer[2] = { nullptr, nullptr };
    ID3D12Resource* mDepthStencilBuffer = nullptr;

    D3D12_VIEWPORT mScreenViewport;
    D3D12_RECT mScissorRect;

    UINT mRtvDescriptorSize = 0;
    UINT mDsvDescriptorSize = 0;
    UINT mCbvSrvDescriptorSize = 0;

    DXGI_FORMAT mBackBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
    DXGI_FORMAT mDepthStencilFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

    static const int SwapChainBufferCount = 2;
    int mCurrBackBuffer = 0;

    UINT64 mCurrentFence = 0;

    bool m4xMsaaState = false;
    UINT m4xMsaaQuality = 0;

    float mClearColor[4] = { 0.0f, 0.0f, 1.0f, 1.0f };

};

#endif // D3DAPP_H