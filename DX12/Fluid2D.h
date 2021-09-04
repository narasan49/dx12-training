#pragma once
#include <wrl.h>
#include <d3d12.h>
#include <DirectXTex.h>
#include <vector>

using namespace DirectX;
using namespace Microsoft::WRL;
using namespace std;

class DX12;

// 流体の頂点情報
struct FluidNode
{
	// 2次元位置(定数)
	XMFLOAT3 pos;
	XMFLOAT3 normal;
	XMFLOAT2 uv;
	// 2次元速度
	XMFLOAT2 velocity;
};



class Fluid2D
{
public:
	Fluid2D(DX12& dx12, int nx, int ny);
	void Init();
	//HRESULT CreateRootSignature();
	//HRESULT CreateComputePipeline();
	//HRESULT CreateGraphicsPipeline();
	//HRESULT CompileShader(LPCWSTR fileName, LPCSTR entryPoint, LPCSTR shaderModel, ID3DBlob** blob);
	HRESULT CreateUAVView();

	HRESULT CreateVertexAndIndexView();
private:
	unsigned int mNumX;
	unsigned int mNumY;
	unsigned int mNumVertices;

	unsigned int mNumIndices;
	vector<unsigned short> mIndices;
	DX12& mDX12;

	FluidNode* mMappedFluid = nullptr;
	vector<FluidNode> mFluid;

	// 頂点
	ComPtr<ID3D12Resource> mFluidBuff = nullptr;
	ComPtr<ID3D12DescriptorHeap> mFluidDescHeap = nullptr;

	// 
	ComPtr<ID3D12Resource> mVertexBuff = nullptr;
	ComPtr<ID3D12Resource> mIndexBuff = nullptr;
	D3D12_VERTEX_BUFFER_VIEW mVbView = {};
	D3D12_INDEX_BUFFER_VIEW mIbView = {};

	ComPtr<ID3D12PipelineState> mCPipeline;
	ComPtr<ID3D12PipelineState> mGPipeline;
	ComPtr<ID3D12RootSignature> mRootSignature;
};

