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

// ���_�������̃T�C�Y
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

	// ���[�v������
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

	// ���W
	Transform mTransform;
	Transform* mMappedTransform = nullptr;
	// ���[���h�ϊ��ƃ{�[���ϊ�
	XMMATRIX* mMappedMatrices = nullptr;
	ComPtr<ID3D12Resource> mTransformBuff = nullptr;
	ComPtr<ID3D12DescriptorHeap> mTransformDescHeap = nullptr;

	// ���_
	ComPtr<ID3D12Resource> mVertexBuff = nullptr;
	ComPtr<ID3D12Resource> mIndexBuff = nullptr;
	D3D12_VERTEX_BUFFER_VIEW mVbView = {};
	D3D12_INDEX_BUFFER_VIEW mIbView = {};

	// �}�e���A��
	vector<ComPtr<ID3D12Resource>> mTextureResource;
	vector<ComPtr<ID3D12Resource>> mSphResource;
	vector<ComPtr<ID3D12Resource>> mSpaResource;
	vector<ComPtr<ID3D12Resource>> mToonResource;

	ComPtr<ID3D12Resource> mMaterialBuff = nullptr;
	ComPtr<ID3D12DescriptorHeap> mMaterialDescHeap = nullptr;

	// �{�[��
	vector<XMMATRIX> mBoneMatrices;
	// �{�[�����ƃ{�[�����̃}�b�v
	map<string, BoneNode> mBoneNodeTable;

	// ���[�V����
	unsigned int mNumMotionData;
	vector<VMDMotion> mVmdMotionData;
	// �{�[�����ƃ��[�V�����z��̃}�b�v
	unordered_map<string, vector<Motion>> mMotionData;

	void RecursiveMatrixMultiply(BoneNode* node, const XMMATRIX& mat);

	float angle;

	void MotionUpdate();
	// �A�j���[�V�����J�n���̎���
	DWORD mStartTime;
	unsigned int mMaxFrame = 0;
};

