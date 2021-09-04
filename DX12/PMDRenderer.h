#pragma once
#include <tchar.h>
#include <d3d12.h>
#include <wrl.h>

using namespace Microsoft::WRL;
class DX12;

class PMDRenderer
{
public:
	PMDRenderer(DX12& dx12);
	~PMDRenderer();

	// パイプライン初期化
	HRESULT CreateRootSignature();
	HRESULT CreateGraphicsPipelineForPMD();

	HRESULT CompileShader(LPCWSTR fileName, LPCSTR entryPoint, LPCSTR shaderModel, ID3DBlob** blob);

	ComPtr<ID3D12PipelineState> getPipeline() { return mPipeline; }
	ComPtr<ID3D12RootSignature> getRootSignature() { return mRootSignature; }

	// 描画関連メソッド
	void SetShadowPipeLine();

private:
	DX12& mDX12;
	ComPtr<ID3D12PipelineState> mPipeline;
	ComPtr<ID3D12PipelineState> mPipelineShadow;
	ComPtr<ID3D12RootSignature> mRootSignature;
};

