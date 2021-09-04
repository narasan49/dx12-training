#pragma once
class Mesh
{
	unsigned int mNumVertices;
	vector<unsigned char> mVertices;
	unsigned int mNumIndices;
	vector<unsigned short> mIndices;
	unsigned int mNumMaterial;
	vector<Material> mMaterials;

	// ƒ}ƒeƒŠƒAƒ‹
	vector<ComPtr<ID3D12Resource>> mTextureResource;
	vector<ComPtr<ID3D12Resource>> mSphResource;
	vector<ComPtr<ID3D12Resource>> mSpaResource;
	vector<ComPtr<ID3D12Resource>> mToonResource;

	ComPtr<ID3D12Resource> mMaterialBuff = nullptr;
	ComPtr<ID3D12DescriptorHeap> mMaterialDescHeap = nullptr;
};

