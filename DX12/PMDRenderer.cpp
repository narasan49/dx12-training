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
	//�f�B�X�N���v�^�e�[�u��(���[�g�V�O�l�`���ɓn�����)
	//�f�B�X�N���v�^�����W
	CD3DX12_DESCRIPTOR_RANGE descTblRange[5] = {};
	// �r���[�v���W�F�N�V�����s��p�萔: 0��
	descTblRange[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0);
	// Transform�s��p�萔: 1��
	descTblRange[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 1);
	// �}�e���A���p�萔: 2��
	descTblRange[2].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 2);
	// �e�N�X�`��0-3��
	descTblRange[3].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 4, 0);
	// �e�N�X�`��4��
	descTblRange[4].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 4);


	// ���[�g�p�����[�^�쐬
	CD3DX12_ROOT_PARAMETER rootParam[4] = {};
	rootParam[0].InitAsDescriptorTable(1, &descTblRange[0]);
	rootParam[1].InitAsDescriptorTable(1, &descTblRange[1]);
	rootParam[2].InitAsDescriptorTable(2, &descTblRange[2]);
	rootParam[3].InitAsDescriptorTable(1, &descTblRange[4]);

	// �T���v���[�ݒ�(uv�l�ɉ������e�N�X�`������F���Ƃ��Ă��鋓���Ɋւ���ݒ�B���[�g�V�O�l�`���ɓn�����)
	CD3DX12_STATIC_SAMPLER_DESC samplerDesc[3] = {};
	// �ʏ�T���v���[
	samplerDesc[0].Init(0);
	// toon�V�F�[�f�B���O�p�T���v���[
	samplerDesc[1].Init(1);
	samplerDesc[1].AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;	// �J��Ԃ��Ȃ�
	samplerDesc[1].AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	samplerDesc[1].AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;

	samplerDesc[2].Init(2);
	samplerDesc[2].ComparisonFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
	samplerDesc[2].Filter = D3D12_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR;
	samplerDesc[2].MaxAnisotropy = 1; // �[�x�X��

	//(���)���[�g�V�O�l�`���쐬
	D3D12_ROOT_SIGNATURE_DESC	rootSignatureDesc = {};
	//���_���݂̂���
	rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
	rootSignatureDesc.pParameters = rootParam;	//���[�g�p�����[�^�̐擪�A�h���X
	rootSignatureDesc.NumParameters = 4;		//���[�g�p�����[�^�̐�
	rootSignatureDesc.pStaticSamplers = samplerDesc;	//�T���v���[�̐擪�A�h���X
	rootSignatureDesc.NumStaticSamplers = 3;

	//�o�C�i���R�[�h�쐬
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
	
	//���[�g�V�O�l�`���I�u�W�F�N�g����
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
	//�V�F�[�_�[�R���p�C��
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


	// ���_���C�A�E�g
	D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
		{	// ���W���
			"POSITION",									//�Z�}���e�B�b�N�X
			0,											//�Z�}���e�B�N�X�̃C���f�b�N�X
			DXGI_FORMAT_R32G32B32_FLOAT,				//�t�H�[�}�b�g
			0,											//���̓X���b�g�̃C���f�b�N�X
			D3D12_APPEND_ALIGNED_ELEMENT,				//�f�[�^���A�����Ă���
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,	//1���_���Ƃ̃��C�A�E�g
			0
		},
		{	// �@��
			"NORMAL",
			0,
			DXGI_FORMAT_R32G32B32_FLOAT,
			0,
			D3D12_APPEND_ALIGNED_ELEMENT,
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
			0
		},
		{	// uv���
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
	//�O���t�B�b�N�X�p�C�v���C���X�e�[�g
	D3D12_GRAPHICS_PIPELINE_STATE_DESC gPipeline = {};
	gPipeline.pRootSignature = mRootSignature.Get();
	//�V�F�[�_�[�̃Z�b�g
	gPipeline.VS = CD3DX12_SHADER_BYTECODE(vsBlob);
	gPipeline.PS = CD3DX12_SHADER_BYTECODE(psBlob);
	//�T���v���}�X�N
	gPipeline.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
	//���X�^���C�U�[�X�e�[�g
	gPipeline.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	gPipeline.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;	//�J�����O�͂��Ȃ�
	//�u�����h�X�e�[�g
	gPipeline.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);

	//�����_�[�^�[�Q�b�g�̐ݒ�
	D3D12_RENDER_TARGET_BLEND_DESC renderTargetBlendDesc = {};
	renderTargetBlendDesc.BlendEnable = false;
	renderTargetBlendDesc.LogicOpEnable = false;
	renderTargetBlendDesc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
	gPipeline.BlendState.RenderTarget[0] = renderTargetBlendDesc;

	//���̓��C�A�E�g�̐ݒ�
	gPipeline.InputLayout.pInputElementDescs = inputLayout;	//���C�A�E�g�̐擪�A�h���X
	gPipeline.InputLayout.NumElements = _countof(inputLayout);	//���C�A�E�g�z��̗v�f��
	gPipeline.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;	//�J�b�g��
	gPipeline.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;	//�O�p�`�ō\��

	//�����_�[�^�[�Q�b�g�̐ݒ�
	gPipeline.NumRenderTargets = 1;	//���͈�̂�
	gPipeline.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;

	//�A���`�G�C���A�V���O�ݒ�(����̓A���`�G�C���A�V���O���Ȃ�)
	gPipeline.SampleDesc.Count = 1;		//�T���v������1�s�N�Z���ɂ�1��
	gPipeline.SampleDesc.Quality = 0;	//�Œ�N�I���e�B

	// �f�v�X�X�e���V���ݒ�
	gPipeline.DepthStencilState.DepthEnable = true;
	gPipeline.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;	// ��������
	gPipeline.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;		// �����������̗p
	gPipeline.DepthStencilState.StencilEnable = false;
	gPipeline.DSVFormat = DXGI_FORMAT_D32_FLOAT;

	//�O���t�B�b�N�X�p�C�v���C���I�u�W�F�N�g����
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
			::OutputDebugStringA("�t�@�C����������܂���");

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