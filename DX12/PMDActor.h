#pragma once
#include <wrl.h>
#include <d3d12.h>
#include <DirectXMath.h>
#include <string>
#include <vector>
#include <map>
#include <unordered_map>
#include "PMDUtil.h"

using namespace DirectX;
using namespace std;
using namespace Microsoft::WRL;

// 頂点一つ当たりのサイズ
constexpr unsigned int pmdvertex_size = 38;

class DX12;

class PMDActor
{
public:
	PMDActor(string file, DX12& dx12, string vmdFile);
	~PMDActor() {}
	HRESULT LoadPMDFile(string file);
	HRESULT CreateVertexAndIndexView();
	HRESULT CreateTextureAndMaterialView();
	HRESULT CreateTransformView();

	void LoadVMDFile(string file);
	void PlayAnimation();

	// ループ内処理
	void Update();
	void Draw(bool isShadow = false);

	void SetTransform(Transform transform)
	{
		mTransform = transform;
	}

private:
	DX12& mDX12;

	PMDHeader mPmdHeader;
	unsigned int mNumVertices;
	vector<unsigned char> mVertices;
	unsigned int mNumIndices;
	vector<unsigned short> mIndices;
	unsigned int mNumMaterial;
	vector<Material> mMaterials;

	// 座標
	Transform mTransform;
	Transform* mMappedTransform = nullptr;
	// ワールド変換とボーン変換
	XMMATRIX* mMappedMatrices = nullptr;
	ComPtr<ID3D12Resource> mTransformBuff = nullptr;
	ComPtr<ID3D12DescriptorHeap> mTransformDescHeap = nullptr;

	// 頂点
	ComPtr<ID3D12Resource> mVertexBuff = nullptr;
	ComPtr<ID3D12Resource> mIndexBuff = nullptr;
	D3D12_VERTEX_BUFFER_VIEW mVbView = {};
	D3D12_INDEX_BUFFER_VIEW mIbView = {};

	// マテリアル
	vector<ComPtr<ID3D12Resource>> mTextureResource;
	vector<ComPtr<ID3D12Resource>> mSphResource;
	vector<ComPtr<ID3D12Resource>> mSpaResource;
	vector<ComPtr<ID3D12Resource>> mToonResource;

	ComPtr<ID3D12Resource> mMaterialBuff = nullptr;
	ComPtr<ID3D12DescriptorHeap> mMaterialDescHeap = nullptr;

	// ボーン
	vector<XMMATRIX> mBoneMatrices;
	// ボーン名とボーン情報のマップ
	map<string, BoneNode> mBoneNodeTable;

	// モーション
	unsigned int mNumMotionData;
	vector<VMDMotion> mVmdMotionData;
	// ボーン名とモーション配列のマップ
	unordered_map<string, vector<Motion>> mMotionData;

	void RecursiveMatrixMultiply(BoneNode* node, const XMMATRIX& mat);

	float angle;

	void MotionUpdate();
	// アニメーション開始時の時刻
	DWORD mStartTime;
	unsigned int mMaxFrame = 0;
};

