#include <d3d12.h>     
#include <dxgi1_4.h>     
#include <wrl.h>         
#include <iostream>
#include <stdexcept>
#include <WindowsX.h>
#include <cassert>
#include "d3dx12.h"

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib") 

inline void ThrowIfFailed(HRESULT hr)
{
    if (FAILED(hr)) {
        throw std::runtime_error("HRESULT failed.");
    }
}

void CreateSwapChain(IDXGISwapChain** SwapChain, DXGI_FORMAT BackBufferFormat, bool f4xMsaaState, const int SwapChainBufferCount, HWND hMainWnd, ID3D12CommandQueue* CommandQueue, IDXGIFactory4* dxgiFactory)
{
    // Release the previous swapchain we will be recreating.
    (*SwapChain)->Release();

    DXGI_SWAP_CHAIN_DESC sd;
    sd.BufferDesc.Width = 800;
    sd.BufferDesc.Height = 600;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.BufferDesc.Format = BackBufferFormat;
    sd.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
    sd.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
    sd.SampleDesc.Count = f4xMsaaState ? 4 : 1;
    sd.SampleDesc.Quality = f4xMsaaState ? (f4xMsaaState - 1) : 0;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.BufferCount = SwapChainBufferCount;
    sd.OutputWindow = hMainWnd;
    sd.Windowed = true;
    sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

    // Note: Swap chain uses queue to perform flush.
    ThrowIfFailed(dxgiFactory->CreateSwapChain(
        CommandQueue,
        &sd,
        SwapChain));
}



int main()
{
    ID3D12Device* md3dDevice;
    IDXGIFactory4* mdxgiFactory;
    ID3D12Fence* mFence;

    ID3D12CommandQueue* mCommandQueue;
    ID3D12CommandAllocator* mDirectCmdListAlloc;
    ID3D12GraphicsCommandList* mCommandList;

    IDXGISwapChain* mSwapChain;
    static const int SwapChainBufferCount = 2;
    int mCurrBackBuffer = 0;

    bool      m4xMsaaState = false;   
    UINT      m4xMsaaQuality = 0;     

    HWND      mhMainWnd = nullptr;

    ID3D12DescriptorHeap* mRtvHeap;
    ID3D12DescriptorHeap* mDsvHeap;

    ID3D12Resource* mSwapChainBuffer[SwapChainBufferCount];

    DXGI_FORMAT mBackBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
    DXGI_FORMAT mDepthStencilFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
    ID3D12Resource* mDepthStencilBuffer;
    D3D12_VIEWPORT mScreenViewport;

#if defined(DEBUG) || defined(_DEBUG)
    ID3D12Debug* debugController;
    ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)));
    debugController->EnableDebugLayer();
#endif

    ThrowIfFailed(CreateDXGIFactory1(IID_PPV_ARGS(&mdxgiFactory)));

   
    HRESULT hardwareResult = D3D12CreateDevice(
        nullptr, 
        D3D_FEATURE_LEVEL_11_0,
        IID_PPV_ARGS(&md3dDevice));

   
    if (FAILED(hardwareResult)) {
        IDXGIAdapter* pWarpAdapter;
        ThrowIfFailed(mdxgiFactory->EnumWarpAdapter(IID_PPV_ARGS(&pWarpAdapter)));

        ThrowIfFailed(D3D12CreateDevice(
            pWarpAdapter,
            D3D_FEATURE_LEVEL_11_0,
            IID_PPV_ARGS(&md3dDevice)));
    }

    std::cout << "D3D12 Device cree avec succes!" << std::endl;

    ThrowIfFailed(md3dDevice->CreateFence(
        0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&mFence)));
    UINT mRtvDescriptorSize = md3dDevice->GetDescriptorHandleIncrementSize(
            D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    UINT mDsvDescriptorSize = md3dDevice->GetDescriptorHandleIncrementSize(
        D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

    UINT mCbvSrvDescriptorSize = md3dDevice->GetDescriptorHandleIncrementSize(
        D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS msQualityLevels;
    msQualityLevels.Format = mBackBufferFormat;
    msQualityLevels.SampleCount = 4;
    msQualityLevels.Flags = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE;
    msQualityLevels.NumQualityLevels = 0;
    ThrowIfFailed(md3dDevice->CheckFeatureSupport(
        D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS,
        &msQualityLevels,
        sizeof(msQualityLevels)));
    m4xMsaaQuality = msQualityLevels.NumQualityLevels;
    assert(m4xMsaaQuality > 0 && "Unexpected MSAA quality level.");

    D3D12_COMMAND_QUEUE_DESC queueDesc = {};
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    ThrowIfFailed(md3dDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&mCommandQueue)));

    ThrowIfFailed(md3dDevice->CreateCommandAllocator(
        D3D12_COMMAND_LIST_TYPE_DIRECT,
        IID_PPV_ARGS(&mDirectCmdListAlloc)));

    ThrowIfFailed(md3dDevice->CreateCommandList(
        0,
        D3D12_COMMAND_LIST_TYPE_DIRECT,
        mDirectCmdListAlloc,
        nullptr,
        IID_PPV_ARGS(&mCommandList)));
    mCommandList->Close();

    mSwapChain->Release();

    DXGI_SWAP_CHAIN_DESC sd;
    sd.BufferDesc.Width = 800;
    sd.BufferDesc.Height = 600;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.BufferDesc.Format = mBackBufferFormat;
    sd.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
    sd.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
    sd.SampleDesc.Count = m4xMsaaState ? 4 : 1;
    sd.SampleDesc.Quality = m4xMsaaState ? (m4xMsaaState - 1) : 0;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.BufferCount = SwapChainBufferCount;
    sd.OutputWindow = mhMainWnd;
    sd.Windowed = true;
    sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

    // Note: Swap chain uses queue to perform flush.
    ThrowIfFailed(mdxgiFactory->CreateSwapChain(
        mCommandQueue,
        &sd,
        &mSwapChain));


    D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc;
    rtvHeapDesc.NumDescriptors = SwapChainBufferCount;
    rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    rtvHeapDesc.NodeMask = 0;
    ThrowIfFailed(md3dDevice->CreateDescriptorHeap(
        &rtvHeapDesc, IID_PPV_ARGS(&mRtvHeap)));


    D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc;
    dsvHeapDesc.NumDescriptors = 1;
    dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
    dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    dsvHeapDesc.NodeMask = 0;
    ThrowIfFailed(md3dDevice->CreateDescriptorHeap(
        &dsvHeapDesc, IID_PPV_ARGS(&mDsvHeap)));

    CD3DX12_CPU_DESCRIPTOR_HANDLE dc = CD3DX12_CPU_DESCRIPTOR_HANDLE(
        mRtvHeap->GetCPUDescriptorHandleForHeapStart(),
        mCurrBackBuffer,
        mRtvDescriptorSize);

    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHeapHandle(mRtvHeap->GetCPUDescriptorHandleForHeapStart());

    for (UINT i = 0; i < SwapChainBufferCount; i++)
    {
        ThrowIfFailed(mSwapChain->GetBuffer(i, IID_PPV_ARGS(&mSwapChainBuffer[i])));
        md3dDevice->CreateRenderTargetView(mSwapChainBuffer[i], nullptr, rtvHeapHandle);
        rtvHeapHandle.Offset(1, mRtvDescriptorSize);
    }

    D3D12_RESOURCE_DESC depthStencilDesc;
    depthStencilDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    depthStencilDesc.Alignment = 0;
    depthStencilDesc.Width = 800;
    depthStencilDesc.Height = 600;
    depthStencilDesc.DepthOrArraySize = 1;
    depthStencilDesc.MipLevels = 1;

    // Correction 11/12/2016: SSAO chapter requires an SRV to the depth buffer to read from 
    // the depth buffer.  Therefore, because we need to create two views to the same resource:
    //   1. SRV format: DXGI_FORMAT_R24_UNORM_X8_TYPELESS
    //   2. DSV Format: DXGI_FORMAT_D24_UNORM_S8_UINT
    // we need to create the depth buffer resource with a typeless format.  
    depthStencilDesc.Format = DXGI_FORMAT_R24G8_TYPELESS;

    depthStencilDesc.SampleDesc.Count = m4xMsaaState ? 4 : 1;
    depthStencilDesc.SampleDesc.Quality = m4xMsaaState ? (m4xMsaaQuality - 1) : 0;
    depthStencilDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    depthStencilDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

    D3D12_CLEAR_VALUE optClear;
    optClear.Format = mDepthStencilFormat;
    optClear.DepthStencil.Depth = 1.0f;
    optClear.DepthStencil.Stencil = 0;

    CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
        D3D12_HEAP_FLAG_NONE,
        & depthStencilDesc,
        D3D12_RESOURCE_STATE_COMMON,
        & optClear,
        IID_PPV_ARGS(&mDepthStencilBuffer);

    ThrowIfFailed(md3dDevice->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
        D3D12_HEAP_FLAG_NONE,
        &depthStencilDesc,
        D3D12_RESOURCE_STATE_COMMON,
        &optClear,
        IID_PPV_ARGS(&mDepthStencilBuffer)));

    // Create descriptor to mip level 0 of entire resource using the format of the resource.
    D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc;
    dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
    dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
    dsvDesc.Format = mDepthStencilFormat;
    dsvDesc.Texture2D.MipSlice = 0;
    md3dDevice->CreateDepthStencilView(mDepthStencilBuffer, &dsvDesc, rtvHeapHandle);

    // Transition the resource from its initial state to be used as a depth buffer.
    mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mDepthStencilBuffer,
        D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_DEPTH_WRITE));

    // Execute the resize commands.
    ThrowIfFailed(mCommandList->Close());
    ID3D12CommandList* cmdsLists[] = { mCommandList };
    mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

    mScreenViewport.TopLeftX = 0;
    mScreenViewport.TopLeftY = 0;
    mScreenViewport.Width = static_cast<float>(800);
    mScreenViewport.Height = static_cast<float>(600);
    mScreenViewport.MinDepth = 0.0f;
    mScreenViewport.MaxDepth = 1.0f;

    return 0;
}


