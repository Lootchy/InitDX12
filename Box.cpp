#include "Box.h"

Box::Box(HINSTANCE hInstance) : d3dApp(hInstance)
{
}

Box::~Box()
{
	if (mPSO) mPSO->Release();
	if (mRootSignature) mRootSignature->Release();
	if (mCbvHeap) mCbvHeap->Release();
	if (mvsByteCode) mvsByteCode->Release();
	if (mpsByteCode) mpsByteCode->Release();
}

bool Box::Init()
{
	d3dApp::Initialize();
	mCommandList->Reset(mDirectCmdListAlloc, nullptr);
	BuildDescriptorHeaps();
	BuildConstantBuffers();
	BuildRootSignature();
	BuildShadersAndInputLayout();
	BuildBox();
	buildPSO();

	mCommandList->Close();
	ID3D12CommandList* cmdsLists[] = { mCommandList };
	mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	FlushCommandQueue();

	return true;
}

void Box::Draw()
{
	mCommandList->Reset(mDirectCmdListAlloc, mPSO);

	mCommandList->RSSetViewports(1, &mScreenViewport);
	mCommandList->RSSetScissorRects(1, &mScissorRect);
	// Transition de la back buffer vers l'état RENDER_TARGET
	CD3DX12_RESOURCE_BARRIER barrierToRenderTarget = CD3DX12_RESOURCE_BARRIER::Transition(
		mSwapChainBuffer[mCurrBackBuffer],
		D3D12_RESOURCE_STATE_PRESENT,
		D3D12_RESOURCE_STATE_RENDER_TARGET
	);
	mCommandList->ResourceBarrier(1, &barrierToRenderTarget);



	// Effacer la back buffer et le tampon de profondeur
	mCommandList->ClearRenderTargetView(CD3DX12_CPU_DESCRIPTOR_HANDLE(
		mRtvHeap->GetCPUDescriptorHandleForHeapStart(),
		mCurrBackBuffer,
		mRtvDescriptorSize), DirectX::Colors::LightSteelBlue, 0, nullptr);
	mCommandList->ClearDepthStencilView(mDsvHeap->GetCPUDescriptorHandleForHeapStart(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

	// Définir les cibles de rendu
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(
		mRtvHeap->GetCPUDescriptorHandleForHeapStart(),
		mCurrBackBuffer,
		mRtvDescriptorSize
	);
	D3D12_CPU_DESCRIPTOR_HANDLE dsdew = mDsvHeap->GetCPUDescriptorHandleForHeapStart();
	mCommandList->OMSetRenderTargets(1, &rtvHandle, true, &dsdew);

	ID3D12DescriptorHeap* descriptorHeaps[] = { mCbvHeap };
	mCommandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

	mCommandList->SetPipelineState(mPSO);
	mCommandList->SetGraphicsRootSignature(mRootSignature);
	
	D3D12_VERTEX_BUFFER_VIEW qac = mBoxGeo->VertexBufferView();
	mCommandList->IASetVertexBuffers(0, 1, &qac);
	D3D12_INDEX_BUFFER_VIEW opiu = mBoxGeo->IndexBufferView();
	mCommandList->IASetIndexBuffer(&opiu);
	mCommandList->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	mCommandList->SetGraphicsRootDescriptorTable(0, mCbvHeap->GetGPUDescriptorHandleForHeapStart());
	mCommandList->DrawIndexedInstanced(
		mBoxGeo->DrawArgs["box"].IndexCount,
		1, 0, 0, 0);


	// Transition de la back buffer vers l'état PRESENT
	CD3DX12_RESOURCE_BARRIER barrierToPresent = CD3DX12_RESOURCE_BARRIER::Transition(
		mSwapChainBuffer[mCurrBackBuffer],
		D3D12_RESOURCE_STATE_RENDER_TARGET,
		D3D12_RESOURCE_STATE_PRESENT
	);
	mCommandList->ResourceBarrier(1, &barrierToPresent);

	// Fermer la commande list
	mCommandList->Close();

	// Exécuter les commandes
	ID3D12CommandList* cmdsLists[] = { mCommandList };
	mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	// Présenter la frame
	mSwapChain->Present(0, 0);
	mCurrBackBuffer = (mCurrBackBuffer + 1) % SwapChainBufferCount;

	// Synchronisation GPU/CPU
	FlushCommandQueue();
}

void Box::BuildBox()
{
	std::array<Vertex, 3> vertices = {
		Vertex({ XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT4(Colors::White) }),
		Vertex({ XMFLOAT3(-1.0f, +1.0f, -1.0f), XMFLOAT4(Colors::Black) }),
		Vertex({ XMFLOAT3(+1.0f, +1.0f, -1.0f), XMFLOAT4(Colors::Red) }),
	};

	std::array<std::uint16_t, 3> indices = {
		1, 3, 2
	};

	const UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);
	const UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);

	mBoxGeo = std::make_unique<MeshGeometry>();
	mBoxGeo->Name = "boxGeo";

	ThrowIfFailed(D3DCreateBlob(vbByteSize, &mBoxGeo->VertexBufferCPU));
	CopyMemory(mBoxGeo->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);

	ThrowIfFailed(D3DCreateBlob(ibByteSize, &mBoxGeo->IndexBufferCPU));
	CopyMemory(mBoxGeo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

	mBoxGeo->VertexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice,
		mCommandList, vertices.data(), vbByteSize, mBoxGeo->VertexBufferUploader);

	mBoxGeo->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice,
		mCommandList, indices.data(), ibByteSize, mBoxGeo->IndexBufferUploader);

	mBoxGeo->VertexByteStride = sizeof(Vertex);
	mBoxGeo->VertexBufferByteSize = vbByteSize;
	mBoxGeo->IndexFormat = DXGI_FORMAT_R16_UINT;
	mBoxGeo->IndexBufferByteSize = ibByteSize;

	SubmeshGeometry submesh;
	submesh.IndexCount = (UINT)indices.size();
	submesh.StartIndexLocation = 0;
	submesh.BaseVertexLocation = 0;

	mBoxGeo->DrawArgs["box"] = submesh;
}

void Box::BuildShadersAndInputLayout()
{
	HRESULT hr = S_OK;

	mvsByteCode = d3dUtil::CompileShader(L"color.hlsl", nullptr, "VS", "vs_5_0");
	mpsByteCode = d3dUtil::CompileShader(L"color.hlsl", nullptr, "PS", "ps_5_0");

	mInputLayout =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};
}

void Box::buildPSO()
{
	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc;
	ZeroMemory(&psoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	psoDesc.InputLayout = { mInputLayout.data(), (UINT)mInputLayout.size() };
	psoDesc.pRootSignature = mRootSignature;
	psoDesc.VS =
	{
		reinterpret_cast<BYTE*>(mvsByteCode->GetBufferPointer()),
		mvsByteCode->GetBufferSize()
	};
	psoDesc.PS =
	{
		reinterpret_cast<BYTE*>(mpsByteCode->GetBufferPointer()),
		mpsByteCode->GetBufferSize()
	};
	psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	psoDesc.SampleMask = UINT_MAX;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = mBackBufferFormat;
	psoDesc.SampleDesc.Count = m4xMsaaState ? 4 : 1;
	psoDesc.SampleDesc.Quality = m4xMsaaState ? (m4xMsaaQuality - 1) : 0;
	psoDesc.DSVFormat = mDepthStencilFormat;
	md3dDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&mPSO));
}

void Box::BuildDescriptorHeaps()
{
	D3D12_DESCRIPTOR_HEAP_DESC cbvHeapDesc;
	cbvHeapDesc.NumDescriptors = 1;
	cbvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	cbvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	cbvHeapDesc.NodeMask = 0;
	md3dDevice->CreateDescriptorHeap(&cbvHeapDesc,
		IID_PPV_ARGS(&mCbvHeap));
}

void Box::BuildConstantBuffers()
{
	mObjectCB = std::make_unique<UploadBuffer<ObjectConstants>>(md3dDevice, 1, true);

	UINT objCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(ObjectConstants));

	D3D12_GPU_VIRTUAL_ADDRESS cbAddress = mObjectCB->Resource()->GetGPUVirtualAddress();
	// Offset to the ith object constant buffer in the buffer.
	int boxCBufIndex = 0;
	cbAddress += boxCBufIndex * objCBByteSize;

	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc;
	cbvDesc.BufferLocation = cbAddress;
	cbvDesc.SizeInBytes = d3dUtil::CalcConstantBufferByteSize(sizeof(ObjectConstants));

	md3dDevice->CreateConstantBufferView(
		&cbvDesc,
		mCbvHeap->GetCPUDescriptorHandleForHeapStart());
}

void Box::BuildRootSignature()
{
	
	// Shader programs typically require resources as input (constant buffers,
	// textures, samplers).  The root signature defines the resources the shader
	// programs expect.  If we think of the shader programs as a function, and
	// the input resources as function parameters, then the root signature can be
	// thought of as defining the function signature.  

	// Root parameter can be a table, root descriptor or root constants.
	CD3DX12_ROOT_PARAMETER slotRootParameter[1];

	// Create a single descriptor table of CBVs.
	CD3DX12_DESCRIPTOR_RANGE cbvTable;
	cbvTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0);
	slotRootParameter[0].InitAsDescriptorTable(1, &cbvTable);

	// A root signature is an array of root parameters.
	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(1, slotRootParameter, 0, nullptr,
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	// create a root signature with a single slot which points to a descriptor range consisting of a single constant buffer
	ID3DBlob* serializedRootSig = nullptr;
	ID3DBlob* errorBlob = nullptr;
	HRESULT hr = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1,
		&serializedRootSig, &errorBlob);

	if (errorBlob != nullptr)
	{
		::OutputDebugStringA((char*)errorBlob->GetBufferPointer());
	}
	ThrowIfFailed(hr);

	ThrowIfFailed(md3dDevice->CreateRootSignature(
		0,
		serializedRootSig->GetBufferPointer(),
		serializedRootSig->GetBufferSize(),
		IID_PPV_ARGS(&mRootSignature)));
}
