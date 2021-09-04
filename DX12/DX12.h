#pragma once
using namespace Microsoft::WRL;
using namespace std;
using namespace DirectX;

struct SceneMatrix {
	//XMMATRIX world;
	XMMATRIX view;
	XMMATRIX proj;
	XMMATRIX lightCamera; // ライトから見たビュー
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

	//ビュープロジェクション用ビューの生成
	HRESULT CreateSceneView();
	ComPtr<ID3D12Resource> LoadTextureFromFile(string& texPath);
	// ループ内処理
	void Update();

	// ライトから深度情報を得るための描画
	void DrawToLightDepth();

	void BeginDraw();
	void Draw();
	void EndDraw();

	void BeginDrawPera();
	void DrawPera();
	void EndDrawPera();

	// 縮小バッファへの書き込み
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
	// 深度値テクスチャ用ヒープ
	ComPtr<ID3D12DescriptorHeap> mDepthSrvHeap = nullptr;
	vector<ID3D12Resource*> mBackBuffers;

	ComPtr<ID3D12Fence> mFence = nullptr;
	UINT64 mFenceVal;

	std::unique_ptr<D3D12_VIEWPORT> mViewport;
	std::unique_ptr<D3D12_RECT> mScissorRect;

	map<string, ComPtr<ID3D12Resource>> mResourceTable;

	// テクスチャロード用
	using LoadLambda_t = function<HRESULT(const wstring, TexMetadata*, ScratchImage&)>;
	map<string, LoadLambda_t> mLoadLamdaTable;
	// テクスチャテーブル
	unordered_map<string, ComPtr<ID3D12Resource>> mTextureTable;

	void InitializeLoadTable();

	// ビュープロジェクション行列
	SceneMatrix mSceneMatrix;
	SceneMatrix* mMappedMatrix = nullptr;
	// 並行ライトの向き
	XMFLOAT3 mParallelLightVec;

	ComPtr<ID3D12Resource> mSceneBuffer = nullptr;
	ComPtr<ID3D12DescriptorHeap> mSceneDescHeap = nullptr;

	ComPtr<ID3D12Resource> mWhiteBuff;
	ComPtr<ID3D12Resource> mBlackBuff;
	ComPtr<ID3D12Resource> mGradBuff;

	// マルチパスレンダリング
	std::array<ComPtr<ID3D12Resource>, 2> mPeraResources;
	ComPtr<ID3D12DescriptorHeap> mPeraRTVHeap;
	ComPtr<ID3D12DescriptorHeap> mPeraSRVHeap;
	ComPtr<ID3D12Resource> mPeraVertexBuff;
	ComPtr<ID3D12Resource> mPeraIndexBuff;

	D3D12_VERTEX_BUFFER_VIEW mPeraVBView = {};
	D3D12_VERTEX_BUFFER_VIEW mPeraIBView = {};

	ComPtr<ID3D12PipelineState> mPeraPipeline;
	ComPtr<ID3D12RootSignature> mPeraRootSignature;

	// 被写界深度用ぼかし
	ComPtr<ID3D12Resource> mDofBuffer;

	// 画面全体をぼかす用のパイプライン
	ComPtr<ID3D12PipelineState> mBlurPipeline;

	struct PeraVertex
	{
		XMFLOAT3 pos;
		XMFLOAT2 uv;
	};

	// ブルーム用バッファ
	std::array<ComPtr<ID3D12Resource>, 2> mBloomBuffer;

	// SSAO
	ComPtr<ID3D12Resource> mSsaoBuffer;
	ComPtr<ID3D12PipelineState> mSsaoPipeline;
};
