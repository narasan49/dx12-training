#include "stdafx.h"
#include "Fluid2D.h"
#include "DX12.h"

#include <d3dcompiler.h>
#include <d3dx12.h>
#include <iostream>

Fluid2D::Fluid2D(DX12& dx12, int nx, int ny) : mDX12(dx12), mNumX(nx), mNumY(ny)
{
	mNumVertices = mNumX * mNumY;
}

void Fluid2D::Init()
{
	mFluid.resize(mNumX * mNumY);
	mIndices.resize(mNumVertices * 3);
	int idx = 0;
	for (size_t i = 0; i < mNumX * mNumY; i++)
	{
		int xi = i / mNumX;
		int yi = i % mNumX;
		mFluid[i].pos = XMFLOAT3((float)(i / mNumX), (float)(i % mNumX), 1);
		//mFluid[i].normal = XMFLOAT2();
		//mFluid[i].h = 1;

		// 頂点レイアウト
		if (xi != 0 && yi != mNumY - 1)
		{
			mIndices[idx++] = i;
			mIndices[idx++] = i + mNumX;
			mIndices[idx++] = i + mNumX - 1;
		}
		if (xi != mNumX - 1 && yi != mNumY - 1)
		{
			mIndices[idx++] = i;
			mIndices[idx++] = i + 1;
			mIndices[idx++] = i + mNumX - 1;
		}
	}
	CreateUAVView();
}

HRESULT Fluid2D::CreateUAVView()
{
	// バッファ作成
	auto heapProp = CD3DX12_HEAP_PROPERTIES(D3D12_CPU_PAGE_PROPERTY_WRITE_COMBINE, D3D12_MEMORY_POOL_L0);
	auto bufferSize = sizeof(FluidNode) * mFluid.size();
	bufferSize = (bufferSize + 0xff) & ~0xff;
	auto resDesc = CD3DX12_RESOURCE_DESC::Buffer(bufferSize);
	resDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

	HRESULT result = mDX12.getDevice()->CreateCommittedResource(
		&heapProp,
		D3D12_HEAP_FLAG_NONE,
		&resDesc,
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
		nullptr,
		IID_PPV_ARGS(mFluidBuff.ReleaseAndGetAddressOf())
	);

	// マップ
	FluidNode* mappedFluid = nullptr;
	result = mFluidBuff->Map(0, nullptr, (void**)&mFluid);
	copy(mFluid.begin(), mFluid.end(), mappedFluid);
	mFluidBuff->Unmap(0, nullptr);

	// ディスクリプタヒープ作成
	D3D12_DESCRIPTOR_HEAP_DESC fluidDescHeapDesc = {};
	fluidDescHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	fluidDescHeapDesc.NodeMask = 0;
	fluidDescHeapDesc.NumDescriptors = mFluid.size();
	fluidDescHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;

	result = mDX12.getDevice()->CreateDescriptorHeap(&fluidDescHeapDesc, IID_PPV_ARGS(mFluidDescHeap.ReleaseAndGetAddressOf()));


	// ビュー作成
	D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
	uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
	uavDesc.Format = DXGI_FORMAT_UNKNOWN;
	uavDesc.Buffer.NumElements = mFluid.size();
	uavDesc.Buffer.StructureByteStride = sizeof(FluidNode);

	mDX12.getDevice()->CreateUnorderedAccessView(
		mFluidBuff.Get(),
		nullptr,
		&uavDesc,
		mFluidDescHeap->GetCPUDescriptorHandleForHeapStart()
	);

	return result;
}

//HRESULT Fluid2D::CreateRootSignature()
//{
//	//ディスクリプタテーブル(ルートシグネチャに渡される)
//	//ディスクリプタレンジ
//	CD3DX12_DESCRIPTOR_RANGE descTblRange[3] = {};
//	// ビュープロジェクション行列用定数: 0番
//	descTblRange[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0);
//	// Transform行列用定数: 1番
//	descTblRange[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 1);
//	// UAV 
//	//descTblRange[2].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0);
//	CD3DX12_ROOT_PARAMETER rootParams[3];
//	rootParams[0].InitAsDescriptorTable(1, &descTblRange[0]);
//	rootParams[1].InitAsDescriptorTable(1, &descTblRange[1]);
//	rootParams[2].InitAsUnorderedAccessView(0);
//
//	D3D12_ROOT_SIGNATURE_DESC rootSigDesc = {};
//	rootSigDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_NONE;
//	rootSigDesc.pParameters = rootParams;
//	rootSigDesc.NumParameters = 3;
//	rootSigDesc.pStaticSamplers = nullptr;
//	rootSigDesc.NumStaticSamplers = 0;
//
//	ComPtr<ID3DBlob> signature, errBlob;
//	D3D12SerializeRootSignature(
//		&rootSigDesc,
//		D3D_ROOT_SIGNATURE_VERSION_1_0,
//		&signature,
//		&errBlob
//	);
//
//	mDX12.getDevice()->CreateRootSignature(
//		0,
//		signature->GetBufferPointer(),
//		signature->GetBufferSize(),
//		IID_PPV_ARGS(mRootSignature.ReleaseAndGetAddressOf())
//	);
//}
//
//HRESULT Fluid2D::CreateComputePipeline()
//{
//	ID3DBlob* csBlob = nullptr;
//
//	HRESULT result;
//	result = CompileShader(L"Fluid2D.hlsl", "main", "cs_5_0", &csBlob);
//	if (FAILED(result))
//	{
//		cout << "Compile Compute Shader Failed." << endl;
//		return result;
//	}
//
//	D3D12_COMPUTE_PIPELINE_STATE_DESC cPipeline = {};
//	cPipeline.CS = CD3DX12_SHADER_BYTECODE(csBlob);
//	cPipeline.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
//	cPipeline.NodeMask = 0;
//	cPipeline.pRootSignature = mRootSignature.Get();
//
//	result = mDX12.getDevice()->CreateComputePipelineState(&cPipeline, IID_PPV_ARGS(mCPipeline.ReleaseAndGetAddressOf()));
//}
//
//HRESULT Fluid2D::CreateGraphicsPipeline()
//{
//	HRESULT result = S_OK;
//	// 流体頂点のレイアウト
//	D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
//		{
//			"POSITION", 0,
//			DXGI_FORMAT_R32G32B32_FLOAT, 0,
//			D3D12_APPEND_ALIGNED_ELEMENT,
//			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0
//		},
//		{
//			"NORMAL", 0,
//			DXGI_FORMAT_R32G32B32_FLOAT, 0,
//			D3D12_APPEND_ALIGNED_ELEMENT,
//			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0
//		},
//		{
//			"TEXCOORD", 0,
//			DXGI_FORMAT_R32G32_FLOAT, 0,
//			D3D12_APPEND_ALIGNED_ELEMENT,
//			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0
//		},
//		{
//			"VELOCITY", 0,
//			DXGI_FORMAT_R32G32_FLOAT, 0,
//			D3D12_APPEND_ALIGNED_ELEMENT,
//			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0
//		}
//	};
//	D3D12_GRAPHICS_PIPELINE_STATE_DESC gPipeline = {};
//	gPipeline.pRootSignature = mRootSignature.Get();
//	gPipeline.VS;
//	gPipeline.PS;
//	gPipeline.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
//	gPipeline.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
//	gPipeline.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
//	gPipeline.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
//
//	D3D12_RENDER_TARGET_BLEND_DESC renderTargetBlendDesc = {};
//	renderTargetBlendDesc.BlendEnable = false;
//	renderTargetBlendDesc.LogicOpEnable = false;
//	renderTargetBlendDesc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
//	gPipeline.BlendState.RenderTarget[0] = renderTargetBlendDesc;
//
//	gPipeline.InputLayout.pInputElementDescs = inputLayout;
//	gPipeline.InputLayout.NumElements = _countof(inputLayout);
//	gPipeline.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;
//	gPipeline.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
//
//	gPipeline.NumRenderTargets = 1;
//	gPipeline.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
//
//	gPipeline.SampleDesc.Count = 1;
//	gPipeline.SampleDesc.Quality = 0;
//
//	gPipeline.DepthStencilState.DepthEnable = true;
//	gPipeline.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
//	gPipeline.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
//	gPipeline.DepthStencilState.StencilEnable = false;
//	gPipeline.DSVFormat = DXGI_FORMAT_D32_FLOAT;
//
//	result = mDX12.getDevice()->CreateGraphicsPipelineState(&gPipeline, IID_PPV_ARGS(mGPipeline.ReleaseAndGetAddressOf()));
//}
//
//HRESULT Fluid2D::CompileShader(LPCWSTR fileName, LPCSTR entryPoint, LPCSTR shaderModel, ID3DBlob** blob)
//{
//	ID3DBlob* errorBlob;
//	HRESULT result = D3DCompileFromFile(
//		fileName,
//		nullptr,
//		D3D_COMPILE_STANDARD_FILE_INCLUDE,
//		entryPoint,
//		shaderModel,
//		D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,
//		0,
//		blob,
//		&errorBlob
//	);
//
//	if (FAILED(result))
//	{
//		if (result == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND))
//		{
//			::OutputDebugStringA("ファイルが見つかりません");
//
//		}
//		else
//		{
//			std::string errStr;
//			errStr.resize(errorBlob->GetBufferSize());
//			std::copy_n(
//				(char*)errorBlob->GetBufferPointer(),
//				errorBlob->GetBufferSize(),
//				errStr.begin()
//			);
//			errStr += "\n";
//
//			::OutputDebugStringA(errStr.c_str());
//		}
//	}
//
//	return result;
//}

HRESULT Fluid2D::CreateVertexAndIndexView()
{
	unsigned int vertex_size = sizeof(FluidNode);

	//頂点バッファの生成
	auto heapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	auto resDesc = CD3DX12_RESOURCE_DESC::Buffer(vertex_size * mNumVertices);
	HRESULT result = mDX12.getDevice()->CreateCommittedResource(
		&heapProp,
		D3D12_HEAP_FLAG_NONE,
		&resDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(mVertexBuff.ReleaseAndGetAddressOf())
	);

	if (FAILED(result))
	{
		cout << "Create Vertices Buffer Failed." << endl;
		return result;
	}

	//GPUへマップ
	unsigned char* vertMap = nullptr;
	result = mVertexBuff->Map(0, nullptr, (void**)&mMappedFluid);
	copy(mFluid.begin(), mFluid.end(), mMappedFluid);
	mVertexBuff->Unmap(0, nullptr);

	//頂点バッファビュー作成
	mVbView = {};
	mVbView.BufferLocation = mVertexBuff->GetGPUVirtualAddress();	//バッファーの仮想アドレス
	mVbView.SizeInBytes = mFluid.size(); //全バイト数
	mVbView.StrideInBytes = vertex_size; //1頂点あたりのバイト数

	//インデックスバッファ作成
	auto heapProp2 = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	auto resDesc2 = CD3DX12_RESOURCE_DESC::Buffer(mIndices.size() * sizeof(mIndices[0]));
	result = mDX12.getDevice()->CreateCommittedResource(
		&heapProp,
		D3D12_HEAP_FLAG_NONE,
		&resDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(mIndexBuff.ReleaseAndGetAddressOf())
	);

	if (FAILED(result))
	{
		cout << "Create Index Buffer Failed." << endl;
		return result;
	}

	//GPUへマップ
	unsigned short* idxMap = nullptr;
	result = mIndexBuff->Map(0, nullptr, (void**)&idxMap);
	copy(mIndices.begin(), mIndices.end(), idxMap);
	mIndexBuff->Unmap(0, nullptr);

	//インデックスバッファビュー作成
	mIbView = {};
	mIbView.BufferLocation = mIndexBuff->GetGPUVirtualAddress();
	mIbView.Format = DXGI_FORMAT_R16_UINT;
	mIbView.SizeInBytes = mIndices.size() * sizeof(mIndices[0]);

	return result;
}