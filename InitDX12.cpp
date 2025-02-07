#include <d3d12.h>     
#include <dxgi1_4.h>     
#include <wrl.h>         
#include <iostream>
#include <stdexcept>
#include <WindowsX.h>
#include <cassert>
#include <crtdbg.h>
#include <exception>
#include <DirectXMath.h>
#include <DirectXColors.h> 
#include "d3dx12.h"


#pragma comment(lib,"d3dcompiler.lib")
#pragma comment(lib, "D3D12.lib")
#pragma comment(lib, "dxgi.lib") 



int main()
{
    std::wstring mMainWndCaption = L"d3d App";
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
    HINSTANCE mhAppInst = nullptr;
    HWND      mhMainWnd = nullptr;

    ID3D12DescriptorHeap* mRtvHeap;
    ID3D12DescriptorHeap* mDsvHeap;

    ID3D12Resource* mSwapChainBuffer[SwapChainBufferCount];

    DXGI_FORMAT mBackBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
    DXGI_FORMAT mDepthStencilFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
    ID3D12Resource* mDepthStencilBuffer;
    D3D12_VIEWPORT mScreenViewport;
    D3D12_RECT mScissorRect;
    UINT64 mCurrentFence = 0;

    float clearColor[] = { 0.0f, 0.0f, 1.0f, 1.0f };

#if defined(DEBUG) || defined(_DEBUG)
    ID3D12Debug* debugController;
    D3D12GetDebugInterface(IID_PPV_ARGS(&debugController));
    debugController->EnableDebugLayer();
#endif

    CreateDXGIFactory1(IID_PPV_ARGS(&mdxgiFactory));

   
    HRESULT hardwareResult = D3D12CreateDevice(
        nullptr, 
        D3D_FEATURE_LEVEL_11_0,
        IID_PPV_ARGS(&md3dDevice));

   
    if (FAILED(hardwareResult)) {
        IDXGIAdapter* pWarpAdapter;
        mdxgiFactory->EnumWarpAdapter(IID_PPV_ARGS(&pWarpAdapter));

        D3D12CreateDevice(
            pWarpAdapter,
            D3D_FEATURE_LEVEL_11_0,
            IID_PPV_ARGS(&md3dDevice));
    }

    std::cout << "D3D12 Device cree avec succes!" << std::endl;

    md3dDevice->CreateFence(
        0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&mFence));
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
    md3dDevice->CheckFeatureSupport(
        D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS,
        &msQualityLevels,
        sizeof(msQualityLevels));
    m4xMsaaQuality = msQualityLevels.NumQualityLevels;
    assert(m4xMsaaQuality > 0 && "Unexpected MSAA quality level.");

    D3D12_COMMAND_QUEUE_DESC queueDesc = {};
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    md3dDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&mCommandQueue));

    md3dDevice->CreateCommandAllocator(
        D3D12_COMMAND_LIST_TYPE_DIRECT,
        IID_PPV_ARGS(&mDirectCmdListAlloc));

    md3dDevice->CreateCommandList(
        0,
        D3D12_COMMAND_LIST_TYPE_DIRECT,
        mDirectCmdListAlloc,
        nullptr,
        IID_PPV_ARGS(&mCommandList));
    mCommandList->Close();

    WNDCLASS wc = {};
    wc.lpfnWndProc = DefWindowProc;
    wc.hInstance = mhAppInst;
    wc.lpszClassName = L"MainWnd";
    if (!RegisterClass(&wc)) {
        MessageBox(0, L"Failed to register window class", 0, 0);
        return -1;
    }

    mhMainWnd = CreateWindow(L"MainWnd", mMainWndCaption.c_str(),
        WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 800, 600, 0, 0, mhAppInst, 0);

    if (!mhMainWnd) {
        MessageBox(0, L"Failed to create window", 0, 0);
        return -1;
    }
    ShowWindow(mhMainWnd, SW_SHOW);
    UpdateWindow(mhMainWnd);

    

    mSwapChain = nullptr;

    DXGI_SWAP_CHAIN_DESC sd;
    sd.BufferDesc.Width = 800;
    sd.BufferDesc.Height = 600;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.BufferDesc.Format = mBackBufferFormat;
    sd.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
    sd.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
    sd.SampleDesc.Count = m4xMsaaState ? 4 : 1;
    sd.SampleDesc.Quality = m4xMsaaState ? (m4xMsaaQuality - 1) : 0;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.BufferCount = SwapChainBufferCount;
    sd.OutputWindow = mhMainWnd;
    sd.Windowed = true;
    sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

    // Note: Swap chain uses queue to perform flush.
    mdxgiFactory->CreateSwapChain(
        mCommandQueue,
        &sd,
        &mSwapChain);


    D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc;
    rtvHeapDesc.NumDescriptors = SwapChainBufferCount;
    rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    rtvHeapDesc.NodeMask = 0;
    md3dDevice->CreateDescriptorHeap(
        &rtvHeapDesc, IID_PPV_ARGS(&mRtvHeap));


    D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc;
    dsvHeapDesc.NumDescriptors = 1;
    dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
    dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    dsvHeapDesc.NodeMask = 0;
    md3dDevice->CreateDescriptorHeap(
        &dsvHeapDesc, IID_PPV_ARGS(&mDsvHeap));

    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHeapHandle(mRtvHeap->GetCPUDescriptorHandleForHeapStart());
    for (UINT i = 0; i < SwapChainBufferCount; i++)
    {
        mSwapChain->GetBuffer(i, IID_PPV_ARGS(&mSwapChainBuffer[i]));
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
 
    depthStencilDesc.Format = DXGI_FORMAT_R24G8_TYPELESS;

    depthStencilDesc.SampleDesc.Count = m4xMsaaState ? 4 : 1;
    depthStencilDesc.SampleDesc.Quality = m4xMsaaState ? (m4xMsaaQuality - 1) : 0;
    depthStencilDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    depthStencilDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

    D3D12_CLEAR_VALUE optClear;
    optClear.Format = mDepthStencilFormat;
    optClear.DepthStencil.Depth = 1.0f;
    optClear.DepthStencil.Stencil = 0;

    CD3DX12_HEAP_PROPERTIES heapProperties(D3D12_HEAP_TYPE_DEFAULT);

    md3dDevice->CreateCommittedResource(
        &heapProperties,
        D3D12_HEAP_FLAG_NONE,
        &depthStencilDesc,
        D3D12_RESOURCE_STATE_COMMON,
        &optClear,
        IID_PPV_ARGS(&mDepthStencilBuffer)
    );


    // Create descriptor to mip level 0 of entire resource using the format of the resource.
    D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc;
    dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
    dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
    dsvDesc.Format = mDepthStencilFormat;
    dsvDesc.Texture2D.MipSlice = 0;
    md3dDevice->CreateDepthStencilView(mDepthStencilBuffer, &dsvDesc, mDsvHeap->GetCPUDescriptorHandleForHeapStart());

    CD3DX12_RESOURCE_BARRIER depthStencilBarrier = CD3DX12_RESOURCE_BARRIER::Transition(mDepthStencilBuffer,
        D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_DEPTH_WRITE);

    // Transition the resource from its initial state to be used as a depth buffer.
    mCommandList->ResourceBarrier(1, &depthStencilBarrier);

    // Execute the resize commands.
    mCommandList->Close();
    ID3D12CommandList* cmdsLists[] = { mCommandList };
    mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

    mScreenViewport.TopLeftX = 0;
    mScreenViewport.TopLeftY = 0;
    mScreenViewport.Width = static_cast<float>(800);
    mScreenViewport.Height = static_cast<float>(600);
    mScreenViewport.MinDepth = 0.0f;
    mScreenViewport.MaxDepth = 1.0f;

    MSG msg = {};
    while (msg.message != WM_QUIT)
    {
        if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            

            mCommandList->Reset(mDirectCmdListAlloc, nullptr);

            CD3DX12_RESOURCE_BARRIER barrierToRenderTarget = CD3DX12_RESOURCE_BARRIER::Transition(
                mSwapChainBuffer[mCurrBackBuffer],
                D3D12_RESOURCE_STATE_PRESENT,
                D3D12_RESOURCE_STATE_RENDER_TARGET
            );

            mCommandList->ResourceBarrier(1, &barrierToRenderTarget);


            // Set the viewport and scissor rect.  This needs to be reset whenever the command list is reset.
            mCommandList->RSSetViewports(1, &mScreenViewport);
            mCommandList->RSSetScissorRects(1, &mScissorRect);

            // Clear the back buffer and depth buffer.
            mCommandList->ClearRenderTargetView(CD3DX12_CPU_DESCRIPTOR_HANDLE(
                mRtvHeap->GetCPUDescriptorHandleForHeapStart(),
                mCurrBackBuffer,
                mRtvDescriptorSize), DirectX::Colors::LightSteelBlue, 0, nullptr);
            mCommandList->ClearDepthStencilView(mDsvHeap->GetCPUDescriptorHandleForHeapStart(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

            CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(
                mRtvHeap->GetCPUDescriptorHandleForHeapStart(),
                mCurrBackBuffer,
                mRtvDescriptorSize
            );

            D3D12_CPU_DESCRIPTOR_HANDLE dsdew = mDsvHeap->GetCPUDescriptorHandleForHeapStart();
            mCommandList->OMSetRenderTargets(1, &rtvHandle, true, &dsdew);

            CD3DX12_RESOURCE_BARRIER barrierToPresent = CD3DX12_RESOURCE_BARRIER::Transition(
                mSwapChainBuffer[mCurrBackBuffer],
                D3D12_RESOURCE_STATE_RENDER_TARGET,
                D3D12_RESOURCE_STATE_PRESENT
            );

            mCommandList->ResourceBarrier(1, &barrierToPresent);


            mCommandList->Close();

            ID3D12CommandList* cmdsLists[] = { mCommandList };
            mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

            mSwapChain->Present(0, 0);
            mCurrBackBuffer = (mCurrBackBuffer + 1) % SwapChainBufferCount;

            mCurrentFence++;

            mCommandQueue->Signal(mFence, mCurrentFence);

            // Wait until the GPU has completed commands up to this fence point.
            if (mFence->GetCompletedValue() < mCurrentFence)
            {
                HANDLE eventHandle = CreateEventEx(nullptr, nullptr, false, EVENT_ALL_ACCESS);


                mFence->SetEventOnCompletion(mCurrentFence, eventHandle);
                WaitForSingleObject(eventHandle, INFINITE);
                CloseHandle(eventHandle);
            }
        }
    }
    return 0;
}


