#include "stdafx.h"
#include "DX12.h"
#include "PMDUtil.h"

#include <tchar.h>
#include <iostream>


constexpr uint32_t shadow_definition = 1024;

D3D_FEATURE_LEVEL levels[] = {
	D3D_FEATURE_LEVEL_12_1,
	D3D_FEATURE_LEVEL_12_0,
	D3D_FEATURE_LEVEL_11_1,
	D3D_FEATURE_LEVEL_11_0,
};

DX12::DX12(HWND hwnd) : mDev(nullptr), mDxgiFactory(nullptr), mCmdAllocator(nullptr),
mCmdQueue(nullptr), mSwapChain(nullptr), mFence(nullptr), mFenceVal(0), mParallelLightVec(1, -1, 1)
{
	//mHwnd = hwnd;
	InitializeLoadTable();
	Init(hwnd);
}

void DX12::Update()
{
	XMFLOAT3 eye(0, 20, -25);
	XMFLOAT3 target(0, 10, 0);
	XMFLOAT3 up(0, 1, 0);
	// �e�𗎂Ƃ�����
	//XMFLOAT4 planeVec(0, 1, 0, 0);

	auto lightVec = -XMLoadFloat3(&mParallelLightVec);
	auto targetPos = XMLoadFloat3(&target);
	auto eyePos = XMLoadFloat3(&eye);
	auto upVec = XMLoadFloat3(&up);

	auto armLength = ::XMVector3Length(::XMVectorSubtract(targetPos, eyePos)).m128_f32[0];
	auto lightPos = targetPos + ::XMVector3Normalize(lightVec) * armLength;
	//mSceneMatrix.shadow = ::XMMatrixShadow(XMLoadFloat4(&planeVec), lightVec);
	mMappedMatrix->lightCamera = ::XMMatrixLookAtLH(lightPos, targetPos, upVec) * ::XMMatrixOrthographicLH(40, 40, 1.0f, 100.0f);

}

void DX12::DrawToLightDepth()
{
	// light depth�̃n���h��
	auto handle = mDsvHeap->GetCPUDescriptorHandleForHeapStart();
	handle.ptr += mDev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
	mCmdList->OMSetRenderTargets(0, nullptr, false, &handle);
	mCmdList->ClearDepthStencilView(handle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

	ID3D12DescriptorHeap* descHeap[] = { mSceneDescHeap.Get() };
	auto sceneHandle = mSceneDescHeap->GetGPUDescriptorHandleForHeapStart();
	mCmdList->SetDescriptorHeaps(1, descHeap);
	mCmdList->SetGraphicsRootDescriptorTable(0, sceneHandle);

	auto viewport = CD3DX12_VIEWPORT(0.0f, 0.0f, shadow_definition, shadow_definition);
	CD3DX12_RECT rect(0, 0, shadow_definition, shadow_definition);
	mCmdList->RSSetViewports(1, &viewport);
	mCmdList->RSSetScissorRects(1, &rect);
}

void DX12::BeginDraw()
{
	////�o�b�N�o�b�t�@�ɑ΂��R�}���h�̎��s
	auto bbIdx = mSwapChain->GetCurrentBackBufferIndex();
	//���\�[�X�o���A
	auto resBarrier = CD3DX12_RESOURCE_BARRIER::Transition(
		mBackBuffers[bbIdx],
		D3D12_RESOURCE_STATE_PRESENT,
		D3D12_RESOURCE_STATE_RENDER_TARGET
	);
	mCmdList->ResourceBarrier(1, &resBarrier);

	auto rtvH = mRtvHeap->GetCPUDescriptorHandleForHeapStart();
	rtvH.ptr += bbIdx * mDev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	auto dsvH = mDsvHeap->GetCPUDescriptorHandleForHeapStart();
	mCmdList->OMSetRenderTargets(1, &rtvH, true, &dsvH);
	//mCmdList->ClearDepthStencilView(dsvH, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

	//��ʃN���A
	float clearColor[] = { 0.0f, 1.0f, 1.0f, 1.0f };
	mCmdList->ClearRenderTargetView(rtvH, clearColor, 0, nullptr);

	mCmdList->RSSetViewports(1, mViewport.get());
	mCmdList->RSSetScissorRects(1, mScissorRect.get());
}

void DX12::Draw()
{
	ID3D12DescriptorHeap* descHeap[] = { mSceneDescHeap.Get() };
	auto heapHandle = mSceneDescHeap->GetGPUDescriptorHandleForHeapStart();
	mCmdList->SetDescriptorHeaps(1, descHeap);
	mCmdList->SetGraphicsRootDescriptorTable(0, heapHandle);

	// lightDepth
	mCmdList->SetDescriptorHeaps(1, mDepthSrvHeap.GetAddressOf());
	auto handle = mDepthSrvHeap->GetGPUDescriptorHandleForHeapStart();
	handle.ptr += mDev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	mCmdList->SetGraphicsRootDescriptorTable(3, handle);

}

void DX12::DrawPera()
{
	// �؂�|���S��
	mCmdList->SetPipelineState(mPeraPipeline.Get());
	mCmdList->SetGraphicsRootSignature(mPeraRootSignature.Get());
	mCmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

	mCmdList->SetDescriptorHeaps(1, mPeraSRVHeap.GetAddressOf());
	auto handle = mPeraSRVHeap->GetGPUDescriptorHandleForHeapStart();
	mCmdList->SetGraphicsRootDescriptorTable(0, handle);

	mCmdList->SetDescriptorHeaps(1, mDepthSrvHeap.GetAddressOf());
	mCmdList->SetGraphicsRootDescriptorTable(1, mDepthSrvHeap->GetGPUDescriptorHandleForHeapStart());

	mCmdList->IASetVertexBuffers(0, 1, &mPeraVBView);
	mCmdList->DrawInstanced(4, 1, 0, 0);
}

void DX12::EndDraw()
{
	auto bbIdx = mSwapChain->GetCurrentBackBufferIndex();

	//�o���A�̑J�ڏ�Ԃ̐ݒ�
	auto resBarrier = CD3DX12_RESOURCE_BARRIER::Transition(
		mBackBuffers[bbIdx],
		D3D12_RESOURCE_STATE_RENDER_TARGET,
		D3D12_RESOURCE_STATE_PRESENT
	);
	mCmdList->ResourceBarrier(1, &resBarrier);

	//���߂̃N���[�Y
	mCmdList->Close();

	//�R�}���h���s
	ID3D12CommandList* cmdlists[] = { mCmdList.Get() };
	mCmdQueue->ExecuteCommandLists(1, cmdlists);

	//�t�F���X
	mCmdQueue->Signal(mFence.Get(), ++mFenceVal);
	while (mFence->GetCompletedValue() != mFenceVal)
	{
		auto event = CreateEvent(nullptr, false, false, nullptr);
		mFence->SetEventOnCompletion(mFenceVal, event);

		WaitForSingleObject(event, INFINITE);

		CloseHandle(event);
	}


	mCmdAllocator->Reset();
	mCmdList->Reset(mCmdAllocator.Get(), nullptr);
}

void DX12::BeginDrawPera()
{
	//���\�[�X�o���A
	for (auto& resource : mPeraResources)
	{
		auto resBarrier = CD3DX12_RESOURCE_BARRIER::Transition(
			resource.Get(),
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
			D3D12_RESOURCE_STATE_RENDER_TARGET
		);
		mCmdList->ResourceBarrier(1, &resBarrier);
	}

	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandles[3];
	auto rtvBaseHandle = mPeraRTVHeap->GetCPUDescriptorHandleForHeapStart();
	auto increment = mDev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	UINT offset = 0;
	for (auto& handle : rtvHandles)
	{
		handle.InitOffsetted(rtvBaseHandle, offset);
		offset += increment;
	}

	auto dsvH = mDsvHeap->GetCPUDescriptorHandleForHeapStart();
	mCmdList->OMSetRenderTargets(3, rtvHandles, true, &dsvH);
	mCmdList->ClearDepthStencilView(dsvH, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

	//��ʃN���A
	float clearColor[] = { .0f, .0f, .0f, 1.0f };
	float clearColorWhite[] = { 1.0f, 1.0f, 1.0f, 1.0f };
	mCmdList->ClearRenderTargetView(rtvHandles[0], clearColorWhite, 0, nullptr);
	mCmdList->ClearRenderTargetView(rtvHandles[1], clearColor, 0, nullptr);
	mCmdList->ClearRenderTargetView(rtvHandles[2], clearColor, 0, nullptr);
	//for (auto& handle : rtvHandles)
	//{
	//	mCmdList->ClearRenderTargetView(handle, clearColor, 0, nullptr);
	//}
	

	mCmdList->RSSetViewports(1, mViewport.get());
	mCmdList->RSSetScissorRects(1, mScissorRect.get());
}

void DX12::EndDrawPera()
{
	//�o���A�̑J�ڏ�Ԃ̐ݒ�
	for (auto& resource : mPeraResources)
	{
		auto resBarrier = CD3DX12_RESOURCE_BARRIER::Transition(
			resource.Get(),
			D3D12_RESOURCE_STATE_RENDER_TARGET,
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
		);
		mCmdList->ResourceBarrier(1, &resBarrier);
	}
}

void DX12::DrawShrinkTextureForBlur()
{
	mCmdList->SetPipelineState(mBlurPipeline.Get());
	mCmdList->SetGraphicsRootSignature(mPeraRootSignature.Get());

	// ���_�o�b�t�@
	mCmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
	mCmdList->IASetVertexBuffers(0, 1, &mPeraVBView);

	// ���P�x�̓V�F�[�_�[���\�[�X��
	auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
		mBloomBuffer[0].Get(),
		D3D12_RESOURCE_STATE_RENDER_TARGET, 
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	mCmdList->ResourceBarrier(1, &barrier);

	// �k���o�b�t�@��RTV��
	barrier = CD3DX12_RESOURCE_BARRIER::Transition(
		mBloomBuffer[1].Get(),
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		D3D12_RESOURCE_STATE_RENDER_TARGET);
	mCmdList->ResourceBarrier(1, &barrier);

	// ��ʊE�[�x�p�k���o�b�t�@��RTV��
	barrier = CD3DX12_RESOURCE_BARRIER::Transition(
		mDofBuffer.Get(),
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		D3D12_RESOURCE_STATE_RENDER_TARGET);
	mCmdList->ResourceBarrier(1, &barrier);

	auto rtvHandle = mPeraRTVHeap->GetCPUDescriptorHandleForHeapStart();
	const auto rtvIncrement = mDev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandles[2] = {};
	rtvHandles[0].InitOffsetted(rtvHandle, 3 * rtvIncrement);
	rtvHandles[1].InitOffsetted(rtvHandle, 4 * rtvIncrement);
	// bloom�Ɣ�ʊE�[�x���o��
	mCmdList->OMSetRenderTargets(2, rtvHandles, false, nullptr);
	//float clearColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
	//mCmdList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);

	auto srvHandle = mPeraSRVHeap->GetGPUDescriptorHandleForHeapStart();
	//srvHandle.ptr += 2 * mDev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	// ���P�x���e�N�X�`���Ƃ��ėp����B
	mCmdList->SetDescriptorHeaps(1, mPeraSRVHeap.GetAddressOf());
	mCmdList->SetGraphicsRootDescriptorTable(0, srvHandle);

	// �k���o�b�t�@�ɏ�������ł����B
	auto desc = mBloomBuffer[0]->GetDesc();
	D3D12_VIEWPORT viewport = {};
	D3D12_RECT rect = {};
	viewport.MinDepth = 1.0f;
	viewport.MaxDepth = 0.0f;
	viewport.Height = desc.Height / 2;
	viewport.Width = desc.Width/ 2;
	rect.top = 0;
	rect.left = 0;
	rect.right = viewport.Width;
	rect.bottom = viewport.Height;

	for (int i = 0; i < 8; i++)
	{
		mCmdList->RSSetViewports(1, &viewport);
		mCmdList->RSSetScissorRects(1, &rect);
		mCmdList->DrawInstanced(4, 1, 0, 0);

		// ���ɂ��炵�ăT�C�Y������ɔ����ɂ���
		rect.top += viewport.Height;
		viewport.TopLeftX = 0;
		viewport.TopLeftY = rect.top;

		viewport.Width /= 2;
		viewport.Height /= 2;
		rect.bottom = rect.top + viewport.Height;
	}

	// �k���o�b�t�@��SRV��
	barrier = CD3DX12_RESOURCE_BARRIER::Transition(
		mBloomBuffer[1].Get(),
		D3D12_RESOURCE_STATE_RENDER_TARGET,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	mCmdList->ResourceBarrier(1, &barrier);

	// �k���o�b�t�@��SRV��
	barrier = CD3DX12_RESOURCE_BARRIER::Transition(
		mDofBuffer.Get(),
		D3D12_RESOURCE_STATE_RENDER_TARGET,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	mCmdList->ResourceBarrier(1, &barrier);
}

// 
HRESULT DX12::Init(HWND hwnd)
{
	HRESULT result = S_OK;

#ifdef _DEBUG
	CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG, IID_PPV_ARGS(mDxgiFactory.ReleaseAndGetAddressOf()));
#else	
	CreateDXGIFactory1(IID_PPV_ARGS(&_dxgiFactory));
#endif // _DEBUG

	result = CreateGPUDevice();
	if (FAILED(result)) {
		cout << "Create GPU Device Failed." << endl;
		return result;
	}
	result = InitializeCommand();
	if (FAILED(result)) {
		cout << "Initialize Command Failed." << endl;
		return result;
	}
	result = CreateSwapChain(hwnd);
	if (FAILED(result))
	{
		cout << "Create Swap Chain Failed." << endl;
		return result;
	}
	result = CreateRenderTargetView();
	if (FAILED(result))
	{
		cout << "Create Render Target View Failed." << endl;
		return result;
	}
	result = mDev->CreateFence(mFenceVal, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(mFence.ReleaseAndGetAddressOf()));
	if (FAILED(result))
	{
		cout << "Create Fence Failed." << endl;
		return result;
	}
	result = CreateDepthStencilView();
	if (FAILED(result))
	{
		cout << "Create Depth Stencil View Failed." << endl;
		return result;
	}
	result = CreateDepthSRV();
	if (FAILED(result))
	{
		cout << "Create Depth SRV Failed." << endl;
		return result;
	}
	result = CreateSceneView();
	if (FAILED(result))
	{
		cout << "Create Scene Matrix View Failed." << endl;
		return result;
	}
	result = CreatePeraResource();
	if (FAILED(result))
	{
		cout << "Create PeraResource Failed." << endl;
		return result;
	}

	if (FAILED(CreatePeraVertexAndIndexBufferView()))
	{
		cout << "Create Pera Vertex Buffer View Failed." << endl;
		return result;
	}

	if (FAILED(CreateGraphicsPipeline()))
	{
		cout << "Create Graphics Pipeline Failed." << endl;
	}

	return result;
}

HRESULT DX12::CreateGPUDevice()
{
	vector<IDXGIAdapter*> adapters;
	IDXGIAdapter* tmpAdapter = nullptr;

	// �g����GPU���
	for (int i = 0; mDxgiFactory->EnumAdapters(i, &tmpAdapter) != DXGI_ERROR_NOT_FOUND; i++)
	{
		adapters.push_back(tmpAdapter);
	}

	for (auto adpt : adapters)
	{
		DXGI_ADAPTER_DESC adesc = {};
		adpt->GetDesc(&adesc);

		wstring strDesc = adesc.Description;
		// �g�p����GPU��NVIDIA�ɐݒ�
		if (strDesc.find(L"NVIDIA") != string::npos)
		{
			tmpAdapter = adpt;
			break;
		}
	}

	if (tmpAdapter == nullptr)
	{
		return false;
	}

	HRESULT result;
	D3D_FEATURE_LEVEL featureLevel;
	for (auto lv : levels)
	{
		result = D3D12CreateDevice(tmpAdapter, lv, IID_PPV_ARGS(mDev.ReleaseAndGetAddressOf()));
		if (SUCCEEDED(result)) {
			featureLevel = lv;
			return result;
		}
	}

	return result;
}

HRESULT DX12::InitializeCommand()
{
	// �R�}���h�֘A������
	HRESULT result = mDev->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(mCmdAllocator.ReleaseAndGetAddressOf()));
	if (FAILED(result)) {
		cout << "CreateCommandAllocator Failed." << endl;
		return result;
	}
	result = mDev->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, mCmdAllocator.Get(), nullptr, IID_PPV_ARGS(mCmdList.ReleaseAndGetAddressOf()));
	if (FAILED(result)) {
		cout << "CreateCommandList Failed." << endl;
		return result;
	}

	D3D12_COMMAND_QUEUE_DESC cmdQueueDesc = {};
	// �^�C���A�E�g����
	cmdQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	// �A�_�v�^�[��������g��Ȃ��Ƃ���0�ł悢
	cmdQueueDesc.NodeMask = 0;
	// �v���C�I���e�B�w��Ȃ�
	cmdQueueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
	// �R�}���h���X�g�Ɛݒ�����킹��
	cmdQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

	result = mDev->CreateCommandQueue(&cmdQueueDesc, IID_PPV_ARGS(mCmdQueue.ReleaseAndGetAddressOf()));
	if (FAILED(result)) {
		cout << "CreateCommandQueue Failed." << endl;
	}

	return result;
}

HRESULT DX12::CreateSwapChain(HWND hwnd)
{
	::GetWindowRect(hwnd, &mWindowRect);

	DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
	swapChainDesc.Width  = mWindowRect.right - mWindowRect.left;
	swapChainDesc.Height = mWindowRect.bottom - mWindowRect.top;
	swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.Stereo = false;
	swapChainDesc.SampleDesc.Count = 1;
	swapChainDesc.SampleDesc.Quality = 0;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.BufferCount = 2;
	// �o�b�N�o�b�t�@�͐L�яk�݉�
	swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
	// �t���b�v��͔j��
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	// �w��Ȃ�
	swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
	// �t���X�N���[���؂�ւ���
	swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

	HRESULT result = mDxgiFactory->CreateSwapChainForHwnd(
		mCmdQueue.Get(),
		hwnd,
		&swapChainDesc,
		nullptr,
		nullptr,
		(IDXGISwapChain1**)mSwapChain.ReleaseAndGetAddressOf()
	);

	return result;
}

HRESULT DX12::CreateRenderTargetView()
{
	D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
	heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	heapDesc.NodeMask = 0;
	heapDesc.NumDescriptors = 2;
	heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

	HRESULT result = mDev->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(mRtvHeap.ReleaseAndGetAddressOf()));
	if (FAILED(result))
	{
		cout << "Create Descriptor Heap Failed." << endl;
		return result;
	}

	// sRGB�ݒ�
	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
	rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

	// �X���b�v�`�F�[���̃o�b�t�@�Ɗ֘A�t��
	DXGI_SWAP_CHAIN_DESC swcDesc = {};
	result = mSwapChain->GetDesc(&swcDesc);
	mBackBuffers.resize(swcDesc.BufferCount);

	auto handle = mRtvHeap->GetCPUDescriptorHandleForHeapStart();
	for (int i = 0; i < swcDesc.BufferCount; i++)
	{
		// �o�b�t�@�擾
		result = mSwapChain->GetBuffer(i, IID_PPV_ARGS(&mBackBuffers[i]));
		if (FAILED(result)) {
			cout << "GetBuffer From Swapchain Failed." << endl;
			return result;
		}
		rtvDesc.Format = mBackBuffers[i]->GetDesc().Format;
		// RTV�쐬
		mDev->CreateRenderTargetView(
			mBackBuffers[i],
			&rtvDesc,
			handle
		);

		// �|�C���^���炵
		handle.ptr += mDev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	}

	DXGI_SWAP_CHAIN_DESC1 desc = {};
	result = mSwapChain->GetDesc1(&desc);
	//�r���[�|�[�g
	mViewport.reset(new CD3DX12_VIEWPORT(mBackBuffers[0]));
	mScissorRect.reset(new CD3DX12_RECT(0, 0, desc.Width, desc.Height));

	return result;
}

HRESULT DX12::CreateDepthStencilView()
{
	DXGI_SWAP_CHAIN_DESC1 desc = {};
	auto result = mSwapChain->GetDesc1(&desc);
	// �[�x�o�b�t�@�̍쐬
	D3D12_RESOURCE_DESC depthResDesc = {};
	depthResDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	depthResDesc.Width = desc.Width;
	depthResDesc.Height = desc.Height;
	depthResDesc.DepthOrArraySize = 1;
	depthResDesc.Format = DXGI_FORMAT_R32_TYPELESS;
	depthResDesc.SampleDesc.Count = 1;
	depthResDesc.SampleDesc.Quality = 0;
	depthResDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
	depthResDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	depthResDesc.MipLevels = 1;
	depthResDesc.Alignment = 0;
	// �q�[�v�v���p�e�B
	auto depthHeapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

	// �N���A�o�����[
	D3D12_CLEAR_VALUE depthClearValue = {};
	depthClearValue.DepthStencil.Depth = 1.0f;	// �[��1.0�ŃN���A
	depthClearValue.DepthStencil.Stencil = 0.0f;
	depthClearValue.Format = DXGI_FORMAT_D32_FLOAT;

	result = mDev->CreateCommittedResource(
		&depthHeapProp,
		D3D12_HEAP_FLAG_NONE,
		&depthResDesc,
		D3D12_RESOURCE_STATE_DEPTH_WRITE,	// �[�x�l�������ݗp
		&depthClearValue,
		IID_PPV_ARGS(mDsvBuffer.ReleaseAndGetAddressOf())
	);
	cout << "Create DepthStencil Buffer: " << result << endl;

	// ���C�g�p�[�x�o�b�t�@
	depthResDesc.Width = shadow_definition;
	depthResDesc.Height = shadow_definition;
	result = mDev->CreateCommittedResource(
		&depthHeapProp,
		D3D12_HEAP_FLAG_NONE,
		&depthResDesc,
		D3D12_RESOURCE_STATE_DEPTH_WRITE,
		&depthClearValue,
		IID_PPV_ARGS(mLightDepthBuffer.ReleaseAndGetAddressOf())
	);

	// �[�x�o�b�t�@�r���[�쐬
	D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
	dsvHeapDesc.NodeMask = 0;
	// �ʏ�̐[�x�ƃV���h�E�}�b�v�p�[�x
	dsvHeapDesc.NumDescriptors = 2;
	dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

	result = mDev->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(mDsvHeap.ReleaseAndGetAddressOf()));
	if (FAILED(result))
	{
		cout << "Create DepthStencil Buffer View Failed" << endl;
	}

	// �[�x�r���[�쐬
	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
	dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	dsvDesc.Flags = D3D12_DSV_FLAG_NONE;

	auto handle = mDsvHeap->GetCPUDescriptorHandleForHeapStart();
	mDev->CreateDepthStencilView(mDsvBuffer.Get(), &dsvDesc, handle);
	handle.ptr += mDev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
	// ���C�g�f�v�X
	mDev->CreateDepthStencilView(mLightDepthBuffer.Get(), &dsvDesc, handle);

	return result;
}

// �[�x�e�N�X�`��
HRESULT DX12::CreateDepthSRV()
{
	D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
	heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	heapDesc.NodeMask = 0;
	heapDesc.NumDescriptors = 2;
	heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	auto result = mDev->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&mDepthSrvHeap));
	if (FAILED(result))
	{
		cout << "Create Depth SRV Failed" << endl;
	}

	D3D12_SHADER_RESOURCE_VIEW_DESC resDesc = {};
	resDesc.Format = DXGI_FORMAT_R32_FLOAT;
	resDesc.Texture2D.MipLevels = 1;
	resDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	resDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;

	auto handle = mDepthSrvHeap->GetCPUDescriptorHandleForHeapStart();
	// �ʏ�[�x�̃e�N�X�`��
	mDev->CreateShaderResourceView(mDsvBuffer.Get(), &resDesc, handle);
	handle.ptr += mDev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	// ���C�g�f�v�X�̃e�N�X�`��
	mDev->CreateShaderResourceView(mLightDepthBuffer.Get(), &resDesc, handle);

	return result;
}

HRESULT DX12::CreateSceneView()
{
	DXGI_SWAP_CHAIN_DESC1 desc = {};
	auto result = mSwapChain->GetDesc1(&desc);

	XMFLOAT3 eye(0, 20, -25);
	XMFLOAT3 target(0, 10, 0);
	XMFLOAT3 up(0, 1, 0);

	// �r���[�s��
	XMMATRIX viewMatrix = ::XMMatrixLookAtLH(XMLoadFloat3(&eye), XMLoadFloat3(&target), XMLoadFloat3(&up));
	// �v���W�F�N�V�����s��
	XMMATRIX projMatrix = ::XMMatrixPerspectiveFovLH(
		XM_PIDIV4,
		static_cast<float>(desc.Width) / static_cast<float>(desc.Height),
		1.0f,
		100.0f
	);
	
	mSceneMatrix.eye = eye;
	mSceneMatrix.view = viewMatrix;
	mSceneMatrix.proj = projMatrix;

	//�s��p�萔�o�b�t�@�쐬
	//ID3D12Resource* constBuff = nullptr;
	auto heapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	auto resDesc = CD3DX12_RESOURCE_DESC::Buffer((sizeof(SceneMatrix) + 0xff) & ~0xff);
	result = mDev->CreateCommittedResource(
		&heapProp,
		D3D12_HEAP_FLAG_NONE,
		&resDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(mSceneBuffer.ReleaseAndGetAddressOf())
	);

	cout << "Create Constant Buffer: " << result << endl;

	// �}�b�v�ɂ��萔�̃R�s�[
	result = mSceneBuffer->Map(0, nullptr, (void**)&mMappedMatrix);	// �}�b�v
	*mMappedMatrix = mSceneMatrix;

	// �f�B�X�N���v�^�q�[�v
	D3D12_DESCRIPTOR_HEAP_DESC descHeapDesc = {};
	descHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;	//�V�F�[�_�[���猩����悤��
	descHeapDesc.NodeMask = 0;	//�}�X�N��0
	descHeapDesc.NumDescriptors = 1;	//SRV��CBV
	descHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;	//�V�F�[�_�[���\�[�X�r���[�p
	//����
	result = mDev->CreateDescriptorHeap(&descHeapDesc, IID_PPV_ARGS(mSceneDescHeap.ReleaseAndGetAddressOf()));

	auto basicHeapHandle = mSceneDescHeap->GetCPUDescriptorHandleForHeapStart();

	// �萔�o�b�t�@�r���[�쐬

	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
	cbvDesc.BufferLocation = mSceneBuffer->GetGPUVirtualAddress();
	cbvDesc.SizeInBytes = mSceneBuffer->GetDesc().Width;

	mDev->CreateConstantBufferView(
		&cbvDesc,
		basicHeapHandle
	);
	return result;
}

HRESULT DX12::CreatePeraResource()
{
	// 
	auto heapDesc = mRtvHeap->GetDesc();
	heapDesc.NumDescriptors = 5;

	// �o�b�N�o�b�t�@�̐ݒ���p�N��
	auto& bbuff = mBackBuffers[0];
	auto resDesc = bbuff->GetDesc();

	D3D12_HEAP_PROPERTIES heapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

	float clsClr[4] = { 0.5, 0.5, 0.5, 1.0 };
	D3D12_CLEAR_VALUE clearValue = CD3DX12_CLEAR_VALUE(DXGI_FORMAT_R8G8B8A8_UNORM, clsClr);

	HRESULT result;
	for (auto& resource : mPeraResources)
	{
		result = mDev->CreateCommittedResource(
			&heapProp,
			D3D12_HEAP_FLAG_NONE,
			&resDesc,
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
			&clearValue,
			IID_PPV_ARGS(resource.ReleaseAndGetAddressOf())
		);

		if (FAILED(result))
		{
			cout << "Create PeraResource Failed." << endl;
			return result;
		}
	}

	for (auto& resource : mBloomBuffer)
	{
		result = mDev->CreateCommittedResource(
			&heapProp,
			D3D12_HEAP_FLAG_NONE,
			&resDesc,
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
			&clearValue,
			IID_PPV_ARGS(resource.ReleaseAndGetAddressOf())
		);
		resDesc.Width >>= 1;
	}

	// ��ʊE�[�x�p�o�b�t�@
	if (FAILED(CreateDofResource()))
	{
		cout << "Create DOF Resource Failed." << endl;
	}

	// RTV�p�q�[�v�쐬
	result = mDev->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(mPeraRTVHeap.ReleaseAndGetAddressOf()));

	if (FAILED(result))
	{
		cout << "Create RTV for PeraResource Failed." << endl;
		return result;
	}

	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
	rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
	rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;

	// RTV�쐬
	auto handle = mPeraRTVHeap->GetCPUDescriptorHandleForHeapStart();
	const auto rtvIncrement = mDev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	for (auto& resource : mPeraResources)
	{
		mDev->CreateRenderTargetView(resource.Get(), &rtvDesc, handle);
		handle.ptr += rtvIncrement;
	}

	// �u���[���pRTV
	mDev->CreateRenderTargetView(mBloomBuffer[0].Get(), &rtvDesc, handle);
	// �u���[���p�k���o�b�t�@
	handle.ptr += rtvIncrement;
	mDev->CreateRenderTargetView(mBloomBuffer[1].Get(), &rtvDesc, handle);
	// ��ʊE�[�x�p
	handle.ptr += rtvIncrement;
	mDev->CreateRenderTargetView(mDofBuffer.Get(), &rtvDesc, handle);

	// SRV�p�q�[�v�쐬
	// �؂�x2, �}���`�����_�[�^�[�Q�b�g�ɂ��normal, �u���[��
	heapDesc.NumDescriptors = 4;
	heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

	result = mDev->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(mPeraSRVHeap.ReleaseAndGetAddressOf()));

	if (FAILED(result))
	{
		cout << "Create SRV for PeraResource Failed." << endl;
		return result;
	}

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Format = rtvDesc.Format;
	srvDesc.Texture2D.MipLevels = 1;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

	// SRV�쐬
	handle = mPeraSRVHeap->GetCPUDescriptorHandleForHeapStart();
	const auto srvIncrement = mDev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	for (auto& resource : mPeraResources)
	{
		mDev->CreateShaderResourceView(resource.Get(), &srvDesc, handle);
		handle.ptr += srvIncrement;
	}

	// �u���[���pSRV
	mDev->CreateShaderResourceView(mBloomBuffer[0].Get(), &srvDesc, handle);
	handle.ptr += srvIncrement;
	mDev->CreateShaderResourceView(mBloomBuffer[1].Get(), &srvDesc, handle);
	handle.ptr += srvIncrement;
	// ��ʊE�[�x�p
	mDev->CreateShaderResourceView(mDofBuffer.Get(), &srvDesc, handle);

	return result;
}

HRESULT DX12::CreatePeraVertexAndIndexBufferView()
{
	PeraVertex pv[4] = {
		{{-1, -1, 0.1}, {0, 1}},
		{{-1,  1, 0.1}, {0, 0}},
		{{ 1, -1, 0.1}, {1, 1}},
		{{ 1,  1, 0.1}, {1, 0}}
	};

	auto heapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	auto resDesc = CD3DX12_RESOURCE_DESC::Buffer(sizeof(pv));
	auto result = mDev->CreateCommittedResource(
		&heapProp,
		D3D12_HEAP_FLAG_NONE,
		&resDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(mPeraVertexBuff.ReleaseAndGetAddressOf())
	);

	mPeraVBView.BufferLocation = mPeraVertexBuff->GetGPUVirtualAddress();
	mPeraVBView.SizeInBytes = sizeof(pv);
	mPeraVBView.StrideInBytes = sizeof(PeraVertex);

	PeraVertex* mappedPera = nullptr;
	mPeraVertexBuff->Map(0, nullptr, (void**)&mappedPera);
	copy(begin(pv), end(pv), mappedPera);
	mPeraVertexBuff->Unmap(0, nullptr);

	return result;
}

//HRESULT DX12::CreateBloomResource()
//{
//
//}

HRESULT DX12::CreateDofResource()
{
	auto heapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	auto resDesc = mBackBuffers[0]->GetDesc();
	float clearColor[] = { 0.0f, 0.0f, 0.0f, 1.0f };
	auto clearValue = CD3DX12_CLEAR_VALUE(DXGI_FORMAT_R8G8B8A8_UNORM, clearColor);

	resDesc.Width >>= 1;
	HRESULT result = mDev->CreateCommittedResource(
		&heapProp,
		D3D12_HEAP_FLAG_NONE,
		&resDesc,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		&clearValue,
		IID_PPV_ARGS(mDofBuffer.ReleaseAndGetAddressOf())
	);
	return result;
}

HRESULT DX12::CreateGraphicsPipeline()
{
	D3D12_INPUT_ELEMENT_DESC layout[2] =
	{
		{
			"POSITION",
			0,
			DXGI_FORMAT_R32G32B32_FLOAT,
			0,
			D3D12_APPEND_ALIGNED_ELEMENT,
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
			0
		},
		{
			"TEXCOORD",
			0,
			DXGI_FORMAT_R32G32_FLOAT,
			0,
			D3D12_APPEND_ALIGNED_ELEMENT,
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
			0
		}
	};

	D3D12_GRAPHICS_PIPELINE_STATE_DESC gPipelineDesc = {};
	gPipelineDesc.InputLayout.NumElements = _countof(layout);
	gPipelineDesc.InputLayout.pInputElementDescs = layout;

	ComPtr<ID3DBlob> vs;
	ComPtr<ID3DBlob> ps;
	ComPtr<ID3DBlob> errorBlob;

	auto result = D3DCompileFromFile(
		L"PeraVertex.hlsl",
		nullptr,
		D3D_COMPILE_STANDARD_FILE_INCLUDE,
		"vs",
		"vs_5_0",
		0,
		0,
		vs.ReleaseAndGetAddressOf(),
		errorBlob.ReleaseAndGetAddressOf()
	);

	if (FAILED(result))
	{
		cout << "Compile Vertex Shader Failed." << endl;
		return result;
	}

	result = D3DCompileFromFile(
		L"PeraPixel.hlsl",
		nullptr,
		D3D_COMPILE_STANDARD_FILE_INCLUDE,
		"ps",
		"ps_5_0",
		0,
		0,
		ps.ReleaseAndGetAddressOf(),
		errorBlob.ReleaseAndGetAddressOf()
	);

	if (FAILED(result))
	{
		cout << "Compile Pixel Shader Failed." << endl;
		return result;
	}

	gPipelineDesc.VS = CD3DX12_SHADER_BYTECODE(vs.Get());
	gPipelineDesc.PS = CD3DX12_SHADER_BYTECODE(ps.Get());

	gPipelineDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	gPipelineDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	// �ʏ�J���[�A�@���Abloom
	gPipelineDesc.NumRenderTargets = 3;
	gPipelineDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	gPipelineDesc.RTVFormats[1] = DXGI_FORMAT_R8G8B8A8_UNORM;
	gPipelineDesc.RTVFormats[2] = DXGI_FORMAT_R8G8B8A8_UNORM;

	gPipelineDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	gPipelineDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
	gPipelineDesc.SampleDesc.Count = 1;
	gPipelineDesc.SampleDesc.Quality = 0;
	gPipelineDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;

	if (FAILED(CreateRootSignature()))
	{
		cout << "Create Root Signature Failed" << endl;
	}

	gPipelineDesc.pRootSignature = mPeraRootSignature.Get();
	
	result = mDev->CreateGraphicsPipelineState(
		&gPipelineDesc,
		IID_PPV_ARGS(mPeraPipeline.ReleaseAndGetAddressOf())
	);

	result = D3DCompileFromFile(L"PeraPixel.hlsl", nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "BlurPS", "ps_5_0",
		0, 0, ps.ReleaseAndGetAddressOf(), errorBlob.ReleaseAndGetAddressOf());
	if (FAILED(result))
	{
		cout << "Compile Blur Pixel Shader Failed." << endl;
		return result;
	}

	gPipelineDesc.PS = CD3DX12_SHADER_BYTECODE(ps.Get());
	// blur�̏k���o�b�t�@
	gPipelineDesc.NumRenderTargets = 2;
	gPipelineDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	gPipelineDesc.RTVFormats[1] = DXGI_FORMAT_R8G8B8A8_UNORM;
	gPipelineDesc.RTVFormats[2] = DXGI_FORMAT_UNKNOWN;

	result = mDev->CreateGraphicsPipelineState(&gPipelineDesc, IID_PPV_ARGS(mBlurPipeline.ReleaseAndGetAddressOf()));



	return result;
}

HRESULT DX12::CreateRootSignature()
{
	CD3DX12_DESCRIPTOR_RANGE range[2] = {};
	// �ʏ�̐F�A�@���A���P�x�A���P�x�k���A�ʏ�k��
	range[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 5, 0);
	// �[�x�l�e�N�X�`���p
	range[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 2, 5);

	CD3DX12_ROOT_PARAMETER rootParam[2] = {};
	rootParam[0].InitAsDescriptorTable(1, &range[0], D3D12_SHADER_VISIBILITY_PIXEL);
	rootParam[1].InitAsDescriptorTable(1, &range[1], D3D12_SHADER_VISIBILITY_PIXEL);

	D3D12_STATIC_SAMPLER_DESC sampler = CD3DX12_STATIC_SAMPLER_DESC(0);

	D3D12_ROOT_SIGNATURE_DESC rootSigDesc = {};
	rootSigDesc.NumParameters = 2;
	rootSigDesc.pParameters = rootParam;
	rootSigDesc.NumStaticSamplers = 1;
	rootSigDesc.pStaticSamplers = &sampler;
	rootSigDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

	ComPtr<ID3DBlob> rsBlob;
	ComPtr<ID3DBlob> errBlob;

	auto result = D3D12SerializeRootSignature(
		&rootSigDesc,
		D3D_ROOT_SIGNATURE_VERSION_1,
		rsBlob.ReleaseAndGetAddressOf(),
		errBlob.ReleaseAndGetAddressOf()
	);

	if (FAILED(result))
	{
		return result;
	}

	result = mDev->CreateRootSignature(
		0,
		rsBlob->GetBufferPointer(),
		rsBlob->GetBufferSize(),
		IID_PPV_ARGS(mPeraRootSignature.ReleaseAndGetAddressOf())
	);

	return result;
}

ComPtr<ID3D12Resource> DX12::LoadTextureFromFile(string& texPath)
{
	auto it = mTextureTable.find(texPath);
	if (it != mTextureTable.end())
	{
		return (*it).second;
	}
	// WIC�e�N�X�`���̃��[�h
	TexMetadata metadata = {};
	ScratchImage scratchImg = {};
	auto texPath_w = GetWideStringFromString(texPath);
	auto ext = GetExtention(texPath);
	HRESULT result = CoInitializeEx(0, COINIT_MULTITHREADED);	//No Interface Error��� 
	result = mLoadLamdaTable[ext](texPath_w, &metadata, scratchImg);

	if (FAILED(result))
	{
		return nullptr;
	}
	// ���f�[�^���o
	auto img = scratchImg.GetImage(0, 0, 0);

	auto texHeapProp = CD3DX12_HEAP_PROPERTIES(D3D12_CPU_PAGE_PROPERTY_WRITE_BACK, D3D12_MEMORY_POOL_L0);
	auto resDesc = CD3DX12_RESOURCE_DESC::Tex2D(
		metadata.format,
		metadata.width,
		metadata.height,
		metadata.arraySize,
		metadata.mipLevels
	);

	// �o�b�t�@�쐬
	ID3D12Resource* texBuff = nullptr;
	result = mDev.Get()->CreateCommittedResource(
		&texHeapProp,
		D3D12_HEAP_FLAG_NONE,
		&resDesc,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		nullptr,
		IID_PPV_ARGS(&texBuff)
	);

	if (FAILED(result))
	{
		return nullptr;
	}

	result = texBuff->WriteToSubresource(
		0,
		nullptr,
		img->pixels,
		img->rowPitch,
		img->slicePitch
	);

	if (FAILED(result))
	{
		return nullptr;
	}

	mTextureTable[texPath] = texBuff;
	return texBuff;
}

void DX12::InitializeLoadTable()
{
	mLoadLamdaTable["sph"] = mLoadLamdaTable["spa"] = mLoadLamdaTable["bmp"] = mLoadLamdaTable["png"] = mLoadLamdaTable["jpg"]
		= [](const std::wstring& path, TexMetadata* meta, ScratchImage& img) -> HRESULT
	{
		return LoadFromWICFile(path.c_str(), WIC_FLAGS_NONE, meta, img);
	};
	mLoadLamdaTable["tga"]
		= [](const std::wstring& path, TexMetadata* meta, ScratchImage& img) -> HRESULT
	{
		return LoadFromTGAFile(path.c_str(), TGA_FLAGS_NONE, meta, img);
	};
	mLoadLamdaTable["dds"]
		= [](const std::wstring& path, TexMetadata* meta, ScratchImage& img) -> HRESULT
	{
		return LoadFromDDSFile(path.c_str(), DDS_FLAGS_NONE, meta, img);
	};
}

ComPtr<ID3D12Resource> DX12::CreateWhiteTexture()
{
	if (mWhiteBuff != nullptr)
	{
		return mWhiteBuff;
	}
	//ComPtr<ID3D12Resource> whiteBuff = nullptr;
	auto texHeapProp = CD3DX12_HEAP_PROPERTIES(D3D12_CPU_PAGE_PROPERTY_WRITE_BACK, D3D12_MEMORY_POOL_L0);
	auto resDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R8G8B8A8_UNORM, 4, 4);

	auto result = mDev->CreateCommittedResource(
		&texHeapProp,
		D3D12_HEAP_FLAG_NONE,
		&resDesc,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		nullptr,
		IID_PPV_ARGS(mWhiteBuff.ReleaseAndGetAddressOf())
	);

	if (FAILED(result))
	{
		cout << "Create White Texture Failed With Error Code: " << result << endl;
		return nullptr;
	}

	std::vector<unsigned char> data(4 * 4 * 4);
	std::fill(data.begin(), data.end(), 0xff); //255�Ŗ��߂�

	// �f�[�^�]��
	result = mWhiteBuff->WriteToSubresource(
		0,
		nullptr,
		data.data(),
		4 * 4,
		data.size()
	);

	return mWhiteBuff;
}

ComPtr<ID3D12Resource> DX12::CreateBlackTexture()
{
	if (mBlackBuff != nullptr)
	{
		return mBlackBuff;
	}
	//ComPtr<ID3D12Resource> blackBuff = nullptr;
	auto texHeapProp = CD3DX12_HEAP_PROPERTIES(D3D12_CPU_PAGE_PROPERTY_WRITE_BACK, D3D12_MEMORY_POOL_L0);
	auto resDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R8G8B8A8_UNORM, 4, 4);
	auto result = mDev->CreateCommittedResource(
		&texHeapProp,
		D3D12_HEAP_FLAG_NONE,
		&resDesc,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		nullptr,
		IID_PPV_ARGS(mBlackBuff.ReleaseAndGetAddressOf())
	);

	if (FAILED(result))
	{
		cout << "Create Black Texture Failed With Error Code: " << result << endl;
		return nullptr;
	}

	std::vector<unsigned char> data(4 * 4 * 4);
	std::fill(data.begin(), data.end(), 0x00); // 0�Ŗ��߂�

	// �f�[�^�]��
	result = mBlackBuff->WriteToSubresource(
		0,
		nullptr,
		data.data(),
		4 * 4,
		data.size()
	);

	return mBlackBuff;
}

// �f�t�H���g�̃O���f�[�V�����e�N�X�`��
ComPtr<ID3D12Resource> DX12::CreateGrayGradationTexture()
{
	if (mGradBuff != nullptr)
	{
		return mGradBuff;
	}
	//ComPtr<ID3D12Resource> grayGradBuff = nullptr;
	D3D12_HEAP_PROPERTIES texHeapProp = CD3DX12_HEAP_PROPERTIES(D3D12_CPU_PAGE_PROPERTY_WRITE_BACK, D3D12_MEMORY_POOL_L0);
	D3D12_RESOURCE_DESC resDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R8G8B8A8_UNORM, 4, 256);

	auto result = mDev->CreateCommittedResource(
		&texHeapProp,
		D3D12_HEAP_FLAG_NONE,
		&resDesc,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		nullptr,
		IID_PPV_ARGS(mGradBuff.ReleaseAndGetAddressOf())
	);

	if (FAILED(result))
	{
		cout << "Create Gradation Texture Failed With Error Code: " << result << endl;
		return nullptr;
	}

	std::vector<unsigned int> data(4 * 256);
	unsigned int c = 0xff;
	for (auto it = data.begin(); it != data.end(); it += 4)
	{
		auto color = c;
		std::fill(it, it + 4, color);
		c--;
	}

	// �f�[�^�]��
	result = mGradBuff->WriteToSubresource(
		0,
		nullptr,
		data.data(),
		4 * sizeof(unsigned int),
		sizeof(unsigned int) * data.size()
	);

	return mGradBuff;
}

HRESULT DX12::CreateCommittedResource(D3D12_HEAP_TYPE type) 
{

	//auto heapProp = CD3DX12_HEAP_PROPERTIES(type);
	//auto result = mDev.Get()->CreateCommittedResource(
	//	&heapProp,
	//);
	return S_OK;
}

HRESULT DX12::CreateSsaoBuffer()
{
	auto& backBuf = mBackBuffers[0];
	auto resDesc = backBuf->GetDesc();
	resDesc.Format = DXGI_FORMAT_R32_FLOAT;

	auto heapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	float clearColor[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
	auto clearValue = CD3DX12_CLEAR_VALUE(resDesc.Format, clearColor);

	HRESULT result = S_OK;

	return result;
}

void DX12::DrawSsao()
{

}