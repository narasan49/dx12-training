#include "stdafx.h"
#include "PMDRenderer.h"
#include "DX12.h"
#include <d3dcompiler.h>
#include <string>
#include <iostream>
#include <d3dx12.h>

using namespace std;


PMDRenderer::PMDRenderer(DX12& dx12) : mDX12(dx12)
{
	CreateRootSignature();
	CreateGraphicsPipelineForPMD();
}

PMDRenderer::~PMDRenderer()
{

}

void PMDRenderer::SetShadowPipeLine()
{
	auto cmdList = mDX12.getCommandList();
	cmdList->SetPipelineState(mPipelineShadow.Get());
	cmdList->SetGraphicsRootSignature(mRootSignature.Get());
	cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

HRESULT PMDRenderer::CreateRootSignature()
{
	//ディスクリプタテーブル(ルートシグネチャに渡される)
	//ディスクリプタレンジ
	CD3DX12_DESCRIPTOR_RANGE descTblRange[5] = {};
	// ビュープロジェクション行列用定数: 0番
	descTblRange[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0);
	// Transform行列用定数: 1番
	descTblRange[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 1);
	// マテリアル用定数: 2番
	descTblRange[2].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 2);
	// テクスチャ0-3番
	descTblRange[3].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 4, 0);
	// テクスチャ4番
	descTblRange[4].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 4);


	// ルートパラメータ作成
	CD3DX12_ROOT_PARAMETER rootParam[4] = {};
	rootParam[0].InitAsDescriptorTable(1, &descTblRange[0]);
	rootParam[1].InitAsDescriptorTable(1, &descTblRange[1]);
	rootParam[2].InitAsDescriptorTable(2, &descTblRange[2]);
	rootParam[3].InitAsDescriptorTable(1, &descTblRange[4]);

	// サンプラー設定(uv値に応じたテクスチャから色をとってくる挙動に関する設定。ルートシグネチャに渡される)
	CD3DX12_STATIC_SAMPLER_DESC samplerDesc[3] = {};
	// 通常サンプラー
	samplerDesc[0].Init(0);
	// toonシェーディング用サンプラー
	samplerDesc[1].Init(1);
	samplerDesc[1].AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;	// 繰り返さない
	samplerDesc[1].AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	samplerDesc[1].AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;

	samplerDesc[2].Init(2);
	samplerDesc[2].ComparisonFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
	samplerDesc[2].Filter = D3D12_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR;
	samplerDesc[2].MaxAnisotropy = 1; // 深度傾斜

	//(空の)ルートシグネチャ作成
	D3D12_ROOT_SIGNATURE_DESC	rootSignatureDesc = {};
	//頂点情報のみある
	rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
	rootSignatureDesc.pParameters = rootParam;	//ルートパラメータの先頭アドレス
	rootSignatureDesc.NumParameters = 4;		//ルートパラメータの数
	rootSignatureDesc.pStaticSamplers = samplerDesc;	//サンプラーの先頭アドレス
	rootSignatureDesc.NumStaticSamplers = 3;

	//バイナリコード作成
	ComPtr<ID3DBlob> rootSigBlob = nullptr;
	ComPtr<ID3DBlob> errorBlob = nullptr;
	HRESULT result = D3D12SerializeRootSignature(
		&rootSignatureDesc,
		D3D_ROOT_SIGNATURE_VERSION_1_0,
		&rootSigBlob,
		&errorBlob
	);
	if (FAILED(result))
	{
		cout << "Serialize Root Signature Failed" << endl;
		return result;
	}
	
	//ルートシグネチャオブジェクト生成
	result = mDX12.getDevice()->CreateRootSignature(
		0,
		rootSigBlob->GetBufferPointer(),
		rootSigBlob->GetBufferSize(),
		IID_PPV_ARGS(mRootSignature.ReleaseAndGetAddressOf())
	);

	if (FAILED(result))
	{
		cout << "Create Root Signature Failed" << endl;
	}
	return result;
}

HRESULT PMDRenderer::CreateGraphicsPipelineForPMD()
{
	//シェーダーコンパイル
	ID3DBlob* vsBlob = nullptr;
	ID3DBlob* psBlob = nullptr;

	HRESULT result;
	result = CompileShader(L"BasicVertexShader.hlsl", "BasicVS", "vs_5_0", &vsBlob);
	if (FAILED(result))
	{
		cout << "Compile Vertex Shader Failed." << endl;
		return result;
	}
	result = CompileShader(L"BasicPixelShader.hlsl", "BasicPS", "ps_5_0", &psBlob);
	if (FAILED(result))
	{
		cout << "Compile Pixel Shader Failed." << endl;
		return result;
	}


	// 頂点レイアウト
	D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
		{	// 座標情報
			"POSITION",									//セマンティックス
			0,											//セマンティクスのインデックス
			DXGI_FORMAT_R32G32B32_FLOAT,				//フォーマット
			0,											//入力スロットのインデックス
			D3D12_APPEND_ALIGNED_ELEMENT,				//データが連続している
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,	//1頂点ごとのレイアウト
			0
		},
		{	// 法線
			"NORMAL",
			0,
			DXGI_FORMAT_R32G32B32_FLOAT,
			0,
			D3D12_APPEND_ALIGNED_ELEMENT,
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
			0
		},
		{	// uv情報
			"TEXCOORD",
			0,
			DXGI_FORMAT_R32G32_FLOAT,
			0,
			D3D12_APPEND_ALIGNED_ELEMENT,
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
			0
		},
		{	// 
			"BONE_NO",
			0,
			DXGI_FORMAT_R16G16_UINT,
			0,
			D3D12_APPEND_ALIGNED_ELEMENT,
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
			0
		},
		{	// 
			"WEIGHT",
			0,
			DXGI_FORMAT_R8_UINT,
			0,
			D3D12_APPEND_ALIGNED_ELEMENT,
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
			0
		},
	};
	//グラフィックスパイプラインステート
	D3D12_GRAPHICS_PIPELINE_STATE_DESC gPipeline = {};
	gPipeline.pRootSignature = mRootSignature.Get();
	//シェーダーのセット
	gPipeline.VS = CD3DX12_SHADER_BYTECODE(vsBlob);
	gPipeline.PS = CD3DX12_SHADER_BYTECODE(psBlob);
	//サンプルマスク
	gPipeline.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
	//ラスタライザーステート
	gPipeline.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	gPipeline.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;	//カリングはしない
	//ブレンドステート
	gPipeline.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);

	//レンダーターゲットの設定
	D3D12_RENDER_TARGET_BLEND_DESC renderTargetBlendDesc = {};
	renderTargetBlendDesc.BlendEnable = false;
	renderTargetBlendDesc.LogicOpEnable = false;
	renderTargetBlendDesc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
	gPipeline.BlendState.RenderTarget[0] = renderTargetBlendDesc;

	//入力レイアウトの設定
	gPipeline.InputLayout.pInputElementDescs = inputLayout;	//レイアウトの先頭アドレス
	gPipeline.InputLayout.NumElements = _countof(inputLayout);	//レイアウト配列の要素数
	gPipeline.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;	//カット無
	gPipeline.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;	//三角形で構成

	//レンダーターゲットの設定
	gPipeline.NumRenderTargets = 1;	//今は一つのみ
	gPipeline.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;

	//アンチエイリアシング設定(今回はアンチエイリアシングしない)
	gPipeline.SampleDesc.Count = 1;		//サンプル数は1ピクセルにつき1つ
	gPipeline.SampleDesc.Quality = 0;	//最低クオリティ

	// デプスステンシル設定
	gPipeline.DepthStencilState.DepthEnable = true;
	gPipeline.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;	// 書き込む
	gPipeline.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;		// 小さい方を採用
	gPipeline.DepthStencilState.StencilEnable = false;
	gPipeline.DSVFormat = DXGI_FORMAT_D32_FLOAT;

	//グラフィックスパイプラインオブジェクト生成
	result = mDX12.getDevice()->CreateGraphicsPipelineState(&gPipeline, IID_PPV_ARGS(mPipeline.ReleaseAndGetAddressOf()));
	if (FAILED(result))
	{
		cout << "Create Graphics Pipeline Failed." << endl;
	}

	//result = CompileShader(L"BasicVertexShader.hlsl", "ShadowVS", "vs_5_0", &vsBlob);
	//if (FAILED(result))
	//{
	//	cout << "Compile Vertex Shader Shadow Failed." << endl;
	//	return result;
	//}
	ID3DBlob* errorBlob;
	result = D3DCompileFromFile(
		L"BasicVertexShader.hlsl",
		nullptr,
		D3D_COMPILE_STANDARD_FILE_INCLUDE,
		"ShadowVS",
		"vs_5_0",
		0,
		0,
		&vsBlob,
		&errorBlob
	);
	gPipeline.VS = CD3DX12_SHADER_BYTECODE(vsBlob);
	gPipeline.PS.BytecodeLength = 0;
	gPipeline.PS.pShaderBytecode = nullptr;
	gPipeline.NumRenderTargets = 0;
	gPipeline.RTVFormats[0] = DXGI_FORMAT_UNKNOWN;

	result = mDX12.getDevice()->CreateGraphicsPipelineState(&gPipeline, IID_PPV_ARGS(mPipelineShadow.ReleaseAndGetAddressOf()));
	if (FAILED(result))
	{
		cout << "Create Graphics Pipeline for Shadow Failed." << endl;
		return result;
	}
	return result;
}

HRESULT PMDRenderer::CompileShader(LPCWSTR fileName, LPCSTR entryPoint, LPCSTR shaderModel, ID3DBlob** blob)
{
	ID3DBlob* errorBlob;
	HRESULT result = D3DCompileFromFile(
		fileName,
		nullptr,
		D3D_COMPILE_STANDARD_FILE_INCLUDE,
		entryPoint,
		shaderModel,
		D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,
		0,
		blob,
		&errorBlob
	);

	if (FAILED(result))
	{
		if (result == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND))
		{
			::OutputDebugStringA("ファイルが見つかりません");

		}
		else
		{
			std::string errStr;
			errStr.resize(errorBlob->GetBufferSize());
			std::copy_n(
				(char*)errorBlob->GetBufferPointer(),
				errorBlob->GetBufferSize(),
				errStr.begin()
			);
			errStr += "\n";

			::OutputDebugStringA(errStr.c_str());
		}
	}

	return result;
}