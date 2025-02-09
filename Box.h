#pragma once
#include "d3dApp.h"
#include "MathHelper.h"
#include "UploadBuffer.h"



using namespace DirectX;
using namespace DirectX::PackedVector;

struct Vertex
{
    XMFLOAT3 Pos;
    XMFLOAT4 Color;
};

struct ObjectConstants
{
    XMFLOAT4X4 WorldViewProj = MathHelper::Identity4x4();
};

class Box : public d3dApp
{
private:
    std::unique_ptr<MeshGeometry> mBoxGeo = nullptr;
    ID3DBlob* mvsByteCode = nullptr;
    ID3DBlob* mpsByteCode = nullptr;
    std::vector<D3D12_INPUT_ELEMENT_DESC> mInputLayout;
    ID3D12RootSignature* mRootSignature = nullptr;
    ID3D12PipelineState* mPSO = nullptr;
    ID3D12DescriptorHeap* mCbvHeap = nullptr;
    std::unique_ptr<UploadBuffer<ObjectConstants>> mObjectCB = nullptr;

    float mTheta = 1.5f * XM_PI;
    float mPhi = XM_PIDIV4;
    float mRadius = 5.0f;

    XMFLOAT4X4 mWorld = MathHelper::Identity4x4();
    XMFLOAT4X4 mView = MathHelper::Identity4x4();
    XMFLOAT4X4 mProj = MathHelper::Identity4x4();

public:
    Box(HINSTANCE hInstance);
    ~Box();
    bool Init();
    void Draw()override;
    void Update()override;
    void BuildBox();
    void BuildShadersAndInputLayout();
    void buildPSO();
    void BuildDescriptorHeaps();
    void BuildConstantBuffers();
    void BuildRootSignature();
};

