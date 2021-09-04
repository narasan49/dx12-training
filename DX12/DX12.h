#pragma once
using namespace Microsoft::WRL;
using namespace std;
using namespace DirectX;

struct SceneMatrix {
	//XMMATRIX world;
	XMMATRIX view;
	XMMATRIX proj;
	XMMATRIX lightCamera; // ���C�g���猩���r���[
	XMMATRIX shadow;
	XMFLOAT3 eye;
};

class DX12
{
public:
	DX12(HWND hwnd);
	HRESULT Init(HWND hwnd);

	HRESULT CreateGPUDevice();
	HRESULT InitializeCommand();
	HRESULT CreateSwapChain(HWND hwnd);
	HRESULT CreateRenderTargetView();
	HRESULT CreateDepthStencilView();
	HRESULT CreateDepthSRV();

	HRESULT CreatePeraResource();
	HRESULT CreatePeraVertexAndIndexBufferView();
	//HRESULT CreateBloomResource();
	HRESULT CreateDofResource();

	HRESULT CreateGraphicsPipeline();
	HRESULT CreateRootSignature();

	HRESULT CreateCommittedResource(D3D12_HEAP_TYPE type);

	//�r���[�v���W�F�N�V�����p�r���[�̐���
	HRESULT CreateSceneView();
	ComPtr<ID3D12Resource> LoadTextureFromFile(string& texPath);
	// ���[�v������
	void Update();

	// ���C�g����[�x���𓾂邽�߂̕`��
	void DrawToLightDepth();

	void BeginDraw();
	void Draw();
	void EndDraw();

	void BeginDrawPera();
	void DrawPera();
	void EndDrawPera();

	// �k���o�b�t�@�ւ̏�������
	void DrawShrinkTextureForBlur();

	ComPtr<ID3D12Device> getDevice() { return mDev; }
	ComPtr<ID3D12GraphicsCommandList> getCommandList() { return mCmdList; }
	ComPtr<IDXGISwapChain4> getSwapChain() { return mSwapChain; }

	ComPtr<ID3D12Resource> CreateWhiteTexture();
	ComPtr<ID3D12Resource> CreateBlackTexture();
	ComPtr<ID3D12Resource> CreateGrayGradationTexture();

	HRESULT CreateSsaoBuffer();
	void DrawSsao();
private:
	RECT mWindowRect;
	ComPtr<ID3D12Device> mDev = nullptr;
	ComPtr<IDXGIFactory6> mDxgiFactory = nullptr;
	ComPtr<ID3D12CommandAllocator> mCmdAllocator = nullptr;
	ComPtr<ID3D12CommandQueue> mCmdQueue = nullptr;
	ComPtr<ID3D12GraphicsCommandList> mCmdList = nullptr;
	ComPtr<IDXGISwapChain4> mSwapChain;

	ComPtr<ID3D12DescriptorHeap> mRtvHeap = nullptr;
	ComPtr<ID3D12Resource> mDsvBuffer = nullptr;
	ComPtr<ID3D12Resource> mLightDepthBuffer;
	ComPtr<ID3D12DescriptorHeap> mDsvHeap = nullptr;
	// �[�x�l�e�N�X�`���p�q�[�v
	ComPtr<ID3D12DescriptorHeap> mDepthSrvHeap = nullptr;
	vector<ID3D12Resource*> mBackBuffers;

	ComPtr<ID3D12Fence> mFence = nullptr;
	UINT64 mFenceVal;

	std::unique_ptr<D3D12_VIEWPORT> mViewport;
	std::unique_ptr<D3D12_RECT> mScissorRect;

	map<string, ComPtr<ID3D12Resource>> mResourceTable;

	// �e�N�X�`�����[�h�p
	using LoadLambda_t = function<HRESULT(const wstring, TexMetadata*, ScratchImage&)>;
	map<string, LoadLambda_t> mLoadLamdaTable;
	// �e�N�X�`���e�[�u��
	unordered_map<string, ComPtr<ID3D12Resource>> mTextureTable;

	void InitializeLoadTable();

	// �r���[�v���W�F�N�V�����s��
	SceneMatrix mSceneMatrix;
	SceneMatrix* mMappedMatrix = nullptr;
	// ���s���C�g�̌���
	XMFLOAT3 mParallelLightVec;

	ComPtr<ID3D12Resource> mSceneBuffer = nullptr;
	ComPtr<ID3D12DescriptorHeap> mSceneDescHeap = nullptr;

	ComPtr<ID3D12Resource> mWhiteBuff;
	ComPtr<ID3D12Resource> mBlackBuff;
	ComPtr<ID3D12Resource> mGradBuff;

	// �}���`�p�X�����_�����O
	std::array<ComPtr<ID3D12Resource>, 2> mPeraResources;
	ComPtr<ID3D12DescriptorHeap> mPeraRTVHeap;
	ComPtr<ID3D12DescriptorHeap> mPeraSRVHeap;
	ComPtr<ID3D12Resource> mPeraVertexBuff;
	ComPtr<ID3D12Resource> mPeraIndexBuff;

	D3D12_VERTEX_BUFFER_VIEW mPeraVBView = {};
	D3D12_VERTEX_BUFFER_VIEW mPeraIBView = {};

	ComPtr<ID3D12PipelineState> mPeraPipeline;
	ComPtr<ID3D12RootSignature> mPeraRootSignature;

	// ��ʊE�[�x�p�ڂ���
	ComPtr<ID3D12Resource> mDofBuffer;

	// ��ʑS�̂��ڂ����p�̃p�C�v���C��
	ComPtr<ID3D12PipelineState> mBlurPipeline;

	struct PeraVertex
	{
		XMFLOAT3 pos;
		XMFLOAT2 uv;
	};

	// �u���[���p�o�b�t�@
	std::array<ComPtr<ID3D12Resource>, 2> mBloomBuffer;

	// SSAO
	ComPtr<ID3D12Resource> mSsaoBuffer;
	ComPtr<ID3D12PipelineState> mSsaoPipeline;
};
