#include "stdafx.h"
#include "PMDActor.h"
#include "DX12.h"
#include "PMDUtil.h"
#include <iostream>
#include <algorithm>

#pragma comment(lib, "winmm.lib")

PMDActor::PMDActor(string file, DX12& dx12, string vmdFile = "") : mDX12(dx12)
{
	mTransform.world = XMMatrixIdentity();
	LoadPMDFile(file);
	CreateVertexAndIndexView();
	CreateTransformView();
	CreateTextureAndMaterialView();
	angle = 0;

	LoadVMDFile(vmdFile);
}

void PMDActor::Update()
{
	//angle += 0.1;
	//mTransform.world = XMMatrixRotationY(angle);
	MotionUpdate();
	//*mMappedTransform = mTransform;
	mMappedMatrices[0] = mTransform.world;
}

void PMDActor::Draw(bool isShadow)
{
	auto cmdList = mDX12.getCommandList();
	cmdList->IASetVertexBuffers(0, 1, &mVbView);
	cmdList->IASetIndexBuffer(&mIbView);
	// transform
	ID3D12DescriptorHeap* transformDescHeaps[] = { mTransformDescHeap.Get() };
	auto heapHandle = mTransformDescHeap->GetGPUDescriptorHandleForHeapStart();
	cmdList->SetDescriptorHeaps(1, transformDescHeaps);
	cmdList->SetGraphicsRootDescriptorTable(1, heapHandle);

	if (isShadow)
	{
		cmdList->DrawIndexedInstanced(mNumIndices, 1, 0, 0, 0);
		return;
	}

	// �}�e���A���̃Z�b�g
	ID3D12DescriptorHeap* materialDescHeaps[] = { mMaterialDescHeap.Get() };
	cmdList->SetDescriptorHeaps(1, materialDescHeaps);
	// �q�[�v�擪�A�h���X
	auto materialH = mMaterialDescHeap->GetGPUDescriptorHandleForHeapStart();
	unsigned int idxOffset = 0;

	// �}�e���A���ƃe�N�X�`�����ЂƂ܂Ƃ߂ɂ��Ă���̂�5���i�߂�
	auto cbvSrvIncSize = 5 * mDX12.getDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	for (auto& m : mMaterials)
	{
		// �}�e���A�����ƂɃe�N�X�`����؂�ւ��Ē��_��`�悷��B
		cmdList->SetGraphicsRootDescriptorTable(2, materialH);
		cmdList->DrawIndexedInstanced(m.indicesNum, 2, idxOffset, 0, 0);

		materialH.ptr += cbvSrvIncSize;
		idxOffset += m.indicesNum;
	}
}

HRESULT PMDActor::LoadPMDFile(string file)
{
	// �V�O�l�`��
	char signature[3] = {};

	FILE* fp;
	auto err = fopen_s(&fp, file.c_str(), "rb");

	fread(signature, sizeof(signature), 1, fp);
	fread(&mPmdHeader, sizeof(mPmdHeader), 1, fp);

	fread(&mNumVertices, sizeof(mNumVertices), 1, fp);
	// �o�b�t�@�̊m��
	mVertices.resize(mNumVertices * pmdvertex_size);
	fread(mVertices.data(), mVertices.size(), 1, fp);

	// �C���f�b�N�X��
	fread(&mNumIndices, sizeof(mNumIndices), 1, fp);

	mIndices.resize(mNumIndices);
	fread(mIndices.data(), mIndices.size() * sizeof(mIndices[0]), 1, fp);

	// �}�e���A���ǂݍ���
	fread(&mNumMaterial, sizeof(mNumMaterial), 1, fp);

	vector<PMDMaterial> pmdMaterials(mNumMaterial);
	fread(pmdMaterials.data(), pmdMaterials.size() * sizeof(PMDMaterial), 1, fp);


	// �{�[���ǂݍ���
	unsigned short boneNum = 0;
	fread(&boneNum, sizeof(boneNum), 1, fp);
	vector<PMDBone> pmdBone(boneNum);
	fread(pmdBone.data(), sizeof(PMDBone), boneNum, fp);

	fclose(fp);

	mMaterials.resize(mNumMaterial);
	for (int i = 0; i < pmdMaterials.size(); i++)
	{
		mMaterials[i].indicesNum = pmdMaterials[i].indicesNum;
		mMaterials[i].material.diffuse = pmdMaterials[i].diffuse;
		mMaterials[i].material.alpha = pmdMaterials[i].alpha;
		mMaterials[i].material.specular = pmdMaterials[i].specular;
		mMaterials[i].material.specularity = pmdMaterials[i].specularity;
		mMaterials[i].material.ambient = pmdMaterials[i].ambient;
	}

	mTextureResource.resize(mNumMaterial);
	mSphResource.resize(mNumMaterial);
	mSpaResource.resize(mNumMaterial);
	mToonResource.resize(mNumMaterial);
	for (int i = 0; i < mNumMaterial; i++)
	{
		string fileName = pmdMaterials[i].texFilePath;
		string texFileName = "";
		string sphFileName = "";
		string spaFileName = "";
		if (fileName.length() != 0)
		{
			for (auto& name : SplitFileName(fileName))
			{
				if (GetExtention(name) == "sph")
				{
					sphFileName = name;
				}
				else if (GetExtention(name) == "spa")
				{
					spaFileName = name;
				}
				else if (GetExtention(name) == "bmp")
				{
					texFileName = name;
				}
			}
		}

		if (texFileName.length() == 0)
		{
			mTextureResource[i] = nullptr;
		}
		else
		{
			auto texFilePath = GetTexturePathFromModelAndTexPath(file, texFileName.c_str());
			mTextureResource[i] = mDX12.LoadTextureFromFile(texFilePath);
		}

		if (sphFileName.length() == 0)
		{
			mSphResource[i] = nullptr;
		}
		else
		{
			auto sphFilePath = GetTexturePathFromModelAndTexPath(file, sphFileName.c_str());
			mSphResource[i] = mDX12.LoadTextureFromFile(sphFilePath);
		}

		if (spaFileName.length() == 0)
		{
			mSpaResource[i] = nullptr;
		}
		else
		{
			auto spaFilePath = GetTexturePathFromModelAndTexPath(file, spaFileName.c_str());
			mSpaResource[i] = mDX12.LoadTextureFromFile(spaFilePath);
		}

		// �g�D�[�����\�[�X�ǂݍ���
		std::string toonFilePath = "toon/";
		char toonFileName[16];

		sprintf_s(
			toonFileName,
			"toon%02d.bmp",
			pmdMaterials[i].toonIdx + 1
		);
		toonFilePath += toonFileName;
		mToonResource[i] = mDX12.LoadTextureFromFile(toonFilePath);
	}

	// ���O�ƃ{�[���̃}�b�v���\�z
	vector<string> boneNames(pmdBone.size());

	for (int idx = 0; idx < pmdBone.size(); idx++)
	{
		auto& bone = pmdBone[idx];
		boneNames[idx] = bone.boneName;
		auto& node = mBoneNodeTable[bone.boneName];
		node.boneIdx = idx;
		node.startPos = bone.pos;
	}

	// �e�q�֌W�̍\�z
	for (auto& bone : pmdBone)
	{
		// �e�C���f�b�N�X���`�F�b�N
		if (bone.parentNo >= pmdBone.size())
		{
			continue;
		}

		auto parentName = boneNames[bone.parentNo];
		mBoneNodeTable[parentName].children.emplace_back(&mBoneNodeTable[bone.boneName]);
	}

	// �{�[���s��̔z���������
	mBoneMatrices.resize(pmdBone.size());
	std::fill(mBoneMatrices.begin(), mBoneMatrices.end(), XMMatrixIdentity());

	return S_OK;
}

HRESULT PMDActor::CreateVertexAndIndexView()
{
	//���_�o�b�t�@�̐���
	auto heapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	auto resDesc = CD3DX12_RESOURCE_DESC::Buffer(mVertices.size());
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
	

	//GPU�փ}�b�v
	unsigned char* vertMap = nullptr;
	result = mVertexBuff->Map(0, nullptr, (void**)&vertMap);
	copy(mVertices.begin(), mVertices.end(), vertMap);
	mVertexBuff->Unmap(0, nullptr);

	//���_�o�b�t�@�r���[�쐬
	mVbView = {};
	mVbView.BufferLocation = mVertexBuff->GetGPUVirtualAddress();	//�o�b�t�@�[�̉��z�A�h���X
	mVbView.SizeInBytes = mVertices.size(); //�S�o�C�g��
	mVbView.StrideInBytes = pmdvertex_size; //1���_������̃o�C�g��

	//�C���f�b�N�X�o�b�t�@�쐬
	auto heapProp2 = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	auto resDesc2 = CD3DX12_RESOURCE_DESC::Buffer(mIndices.size() * sizeof(mIndices[0]));
	result = mDX12.getDevice()->CreateCommittedResource(
		&heapProp2,
		D3D12_HEAP_FLAG_NONE,
		&resDesc2,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(mIndexBuff.ReleaseAndGetAddressOf())
	);

	if (FAILED(result))
	{
		cout << "Create Index Buffer Failed." << endl;
		return result;
	}

	//GPU�փ}�b�v
	unsigned short* idxMap = nullptr;
	result = mIndexBuff->Map(0, nullptr, (void**)&idxMap);
	copy(mIndices.begin(), mIndices.end(), idxMap);
	mIndexBuff->Unmap(0, nullptr);

	//�C���f�b�N�X�o�b�t�@�r���[�쐬
	mIbView = {};
	mIbView.BufferLocation = mIndexBuff->GetGPUVirtualAddress();
	mIbView.Format = DXGI_FORMAT_R16_UINT;
	mIbView.SizeInBytes = mIndices.size() * sizeof(mIndices[0]);

	return result;
}

HRESULT PMDActor::CreateTextureAndMaterialView()
{
	// �}�e���A���o�b�t�@�쐬
	auto materialBuffSize = sizeof(MaterialForHlsl);
	materialBuffSize = (materialBuffSize + 0xff) & ~0xff;

	//ID3D12Resource* mMaterialBuff = nullptr;
	auto heapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	auto resDesc = CD3DX12_RESOURCE_DESC::Buffer(materialBuffSize * mNumMaterial);
	HRESULT result = mDX12.getDevice()->CreateCommittedResource(
		&heapProp,
		D3D12_HEAP_FLAG_NONE,
		&resDesc, // �}�e���A��1����256�A���C�����g���Ă���
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(mMaterialBuff.ReleaseAndGetAddressOf())
	);
	cout << "Create Material Buffer: " << result << endl;

	// �}�b�v
	char* mapMaterial = nullptr;
	result = mMaterialBuff->Map(0, nullptr, (void**)&mapMaterial);

	for (auto& m : mMaterials)
	{
		*((MaterialForHlsl*)mapMaterial) = m.material;
		mapMaterial += materialBuffSize;
	}

	mMaterialBuff->Unmap(0, nullptr);

	// �f�B�X�N���v�^�q�[�v�̍쐬
	//ID3D12DescriptorHeap* materialDescHeap = nullptr;
	D3D12_DESCRIPTOR_HEAP_DESC matDescHeapDesc = {};
	matDescHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	matDescHeapDesc.NodeMask = 0;
	matDescHeapDesc.NumDescriptors = mNumMaterial * 5;	// �}�e���A����*2���̃r���[�����
	matDescHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;

	result = mDX12.getDevice()->CreateDescriptorHeap(&matDescHeapDesc, IID_PPV_ARGS(mMaterialDescHeap.ReleaseAndGetAddressOf()));

	cout << "Create Material Buffer View: " << result << endl;

	// �r���[�̍쐬
	D3D12_CONSTANT_BUFFER_VIEW_DESC matCBVDesc = {};
	matCBVDesc.BufferLocation = mMaterialBuff->GetGPUVirtualAddress(); // �o�b�t�@�̃A�h���X
	matCBVDesc.SizeInBytes = materialBuffSize;	// 256�A���C�����g���ꂽ�T�C�Y

	// �e�N�X�`���r���[���ʕ���
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1;

	// �擪�A�h���X���擾
	auto matDescHeapH = mMaterialDescHeap->GetCPUDescriptorHandleForHeapStart();
	auto inc = mDX12.getDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	// ���e�N�X�`��
	ComPtr<ID3D12Resource> whiteTex = mDX12.CreateWhiteTexture();
	// ��
	ComPtr<ID3D12Resource> blackTex = mDX12.CreateBlackTexture();
	// �O���f�[�V����
	ComPtr<ID3D12Resource> gradTex = mDX12.CreateGrayGradationTexture();


	for (int i = 0; i < mNumMaterial; i++)
	{
		// �}�e���A���p�萔�o�b�t�@�r���[
		mDX12.getDevice()->CreateConstantBufferView(&matCBVDesc, matDescHeapH);
		matDescHeapH.ptr += inc;
		matCBVDesc.BufferLocation += materialBuffSize;

		// �V�F�[�_�[���\�[�X�r���[
		if (mTextureResource[i] == nullptr)
		{
			srvDesc.Format = whiteTex->GetDesc().Format;
			mDX12.getDevice()->CreateShaderResourceView(whiteTex.Get(), &srvDesc, matDescHeapH);
		}
		else
		{
			srvDesc.Format = mTextureResource[i]->GetDesc().Format;
			mDX12.getDevice()->CreateShaderResourceView(mTextureResource[i].Get(), &srvDesc, matDescHeapH);
		}

		matDescHeapH.ptr += inc;

		// sph�p�V�F�[�_�[���\�[�X�r���[
		if (mSphResource[i] == nullptr)
		{
			srvDesc.Format = whiteTex->GetDesc().Format;
			mDX12.getDevice()->CreateShaderResourceView(whiteTex.Get(), &srvDesc, matDescHeapH);
		}
		else
		{
			srvDesc.Format = mSphResource[i]->GetDesc().Format;
			mDX12.getDevice()->CreateShaderResourceView(mSphResource[i].Get(), &srvDesc, matDescHeapH);
		}

		matDescHeapH.ptr += inc;

		// spa�p�V�F�[�_�[���\�[�X�r���[
		if (mSpaResource[i] == nullptr)
		{
			srvDesc.Format = blackTex->GetDesc().Format;
			mDX12.getDevice()->CreateShaderResourceView(blackTex.Get(), &srvDesc, matDescHeapH);
		}
		else
		{
			srvDesc.Format = mSpaResource[i]->GetDesc().Format;
			mDX12.getDevice()->CreateShaderResourceView(mSpaResource[i].Get(), &srvDesc, matDescHeapH);
		}

		matDescHeapH.ptr += inc;

		// toon�p�V�F�[�_�[���\�[�X�r���[
		if (mToonResource[i] == nullptr)
		{
			srvDesc.Format = gradTex->GetDesc().Format;
			mDX12.getDevice()->CreateShaderResourceView(blackTex.Get(), &srvDesc, matDescHeapH);
		}
		else
		{
			srvDesc.Format = mToonResource[i]->GetDesc().Format;
			mDX12.getDevice()->CreateShaderResourceView(mToonResource[i].Get(), &srvDesc, matDescHeapH);
		}

		matDescHeapH.ptr += inc;
	}

	return result;
}

HRESULT PMDActor::CreateTransformView()
{
	auto buffSize = sizeof(XMMATRIX) * (1 + mBoneMatrices.size());
	buffSize = (buffSize + 0xff) & ~0xff;
	//�s��p�萔�o�b�t�@�쐬
	auto heapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	auto resDesc = CD3DX12_RESOURCE_DESC::Buffer(buffSize);
	HRESULT result = mDX12.getDevice()->CreateCommittedResource(
		&heapProp,
		D3D12_HEAP_FLAG_NONE,
		&resDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(mTransformBuff.ReleaseAndGetAddressOf())
	);

	cout << "Create Constant Buffer: " << result << endl;

	// �}�b�v�ɂ��萔�̃R�s�[
	result = mTransformBuff->Map(0, nullptr, (void**)&mMappedMatrices);	// �}�b�v
	//*mMappedTransform = mTransform;
	//auto armNode = mBoneNodeTable["���r"];
	//auto& armPos = armNode.startPos;
	//auto armMat = XMMatrixTranslation(-armPos.x, -armPos.y, -armPos.z) *
	//	XMMatrixRotationZ(XM_PIDIV2) *
	//	XMMatrixTranslation(armPos.x, armPos.y, armPos.z);

	//auto elbowNode = mBoneNodeTable["���Ђ�"];
	//auto& elbowPos = elbowNode.startPos;
	//auto elbowMat = XMMatrixTranslation(-elbowPos.x, -elbowPos.y, -elbowPos.z) *
	//	XMMatrixRotationZ(-XM_PIDIV2) *
	//	XMMatrixTranslation(elbowPos.x, elbowPos.y, elbowPos.z);

	//mBoneMatrices[armNode.boneIdx] = armMat;
	//mBoneMatrices[elbowNode.boneIdx] = elbowMat;

	//RecursiveMatrixMultiply(&mBoneNodeTable["�Z���^�["], XMMatrixIdentity());
	//RecursiveMatrixMultiply(&elbowNode, armMat);
	// ���W���R�s�[
	mMappedMatrices[0] = mTransform.world;
	// �{�[�����R�s�[
	//copy(mBoneMatrices.begin(), mBoneMatrices.end(), mMappedMatrices + 1);

	// �f�B�X�N���v�^�q�[�v
	D3D12_DESCRIPTOR_HEAP_DESC descHeapDesc = {};
	descHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;	//�V�F�[�_�[���猩����悤��
	descHeapDesc.NodeMask = 0;	//�}�X�N��0
	descHeapDesc.NumDescriptors = 1;	//SRV��CBV
	descHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;	//�V�F�[�_�[���\�[�X�r���[�p
	//����
	result = mDX12.getDevice()->CreateDescriptorHeap(&descHeapDesc, IID_PPV_ARGS(mTransformDescHeap.ReleaseAndGetAddressOf()));

	auto transformHeapHandle = mTransformDescHeap->GetCPUDescriptorHandleForHeapStart();

	// �萔�o�b�t�@�r���[�쐬

	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
	cbvDesc.BufferLocation = mTransformBuff->GetGPUVirtualAddress();
	cbvDesc.SizeInBytes = mTransformBuff->GetDesc().Width;

	mDX12.getDevice()->CreateConstantBufferView(
		&cbvDesc,
		transformHeapHandle
	);
	return result;
}

void PMDActor::RecursiveMatrixMultiply(BoneNode* node, const XMMATRIX& mat)
{
	mBoneMatrices[node->boneIdx] *= mat;
	for (auto& child : node->children)
	{
		RecursiveMatrixMultiply(child, mBoneMatrices[node->boneIdx]);
	}
}

void PMDActor::LoadVMDFile(string file)
{
	FILE* fp;
	auto err = fopen_s(&fp, file.c_str(), "rb");

	// �ŏ���50�o�C�g��ǂݔ�΂�
	fseek(fp, 50, SEEK_SET);

	fread(&mNumMotionData, sizeof(mNumMotionData), 1, fp);

	mVmdMotionData.resize(mNumMotionData);
	for (auto& motion : mVmdMotionData)
	{
		// �{�[����
		fread(motion.boneName, sizeof(motion.boneName), 1, fp);
		// �\���̂Ƀp�f�B���O�����邽�߁A��؂��ēǂݍ���
		fread(&motion.frameNo,
			sizeof(motion.frameNo) + sizeof(motion.location) + sizeof(motion.quaternion) + sizeof(motion.bezier)
			, 1, fp);

		mMaxFrame = std::max<unsigned int>(mMaxFrame, motion.frameNo);
	}

	// VMD���[�V�����f�[�^����A���ۂɎg�����[�V�����̃e�[�u���֕ϊ�
	for (auto& vmdMotion : mVmdMotionData)
	{
		mMotionData[vmdMotion.boneName].emplace_back(
			Motion(vmdMotion.frameNo, XMLoadFloat4(&vmdMotion.quaternion))
		);
	}

	for (auto& motion : mMotionData)
	{
		std::sort(
			motion.second.begin(), motion.second.end(),
			[](const Motion& left, const Motion& right)
			{
				return left.frameNo <= right.frameNo;
			}
		);
	}
	//for (auto& boneMotion : mMotionData)
	//{
	//	auto node = mBoneNodeTable[boneMotion.first];
	//	auto& pos = node.startPos;
	//	auto mat = XMMatrixTranslation(-pos.x, -pos.y, -pos.z) *
	//		XMMatrixRotationQuaternion(boneMotion.second[0].quaternion) *
	//		XMMatrixTranslation(pos.x, pos.y, pos.z);
	//	mBoneMatrices[node.boneIdx] = mat;
	//}
	//RecursiveMatrixMultiply(&mBoneNodeTable["�Z���^�["], XMMatrixIdentity());

	//copy(mBoneMatrices.begin(), mBoneMatrices.end(), mMappedMatrices + 1);
}

void PMDActor::PlayAnimation()
{
	mStartTime = timeGetTime();
}

void PMDActor::MotionUpdate()
{
	// �o�ߎ���
	auto elapsedTime = timeGetTime() - mStartTime;
	unsigned int frameNo = 30 * (elapsedTime / 1000.0f);

	// ���[�v����
	if (frameNo > mMaxFrame)
	{
		mStartTime = timeGetTime();
		frameNo = 0;
	}

	// �s����N���A
	std::fill(mBoneMatrices.begin(), mBoneMatrices.end(), XMMatrixIdentity());

	// ���[�V�����f�[�^�X�V
	for (auto& motion : mMotionData)
	{
		auto node = mBoneNodeTable[motion.first];

		// ���v������̂�T��
		auto motions = motion.second;
		auto rit = std::find_if(
			motions.rbegin(),
			motions.rend(),
			[frameNo](const Motion& motion)
			{
				return motion.frameNo <= frameNo;
			}
		);

		if (rit == motions.rend())
		{
			continue;
		}

		XMMATRIX rotation;
		auto it = rit.base();

		// ���[�V�����̕��
		if (it != motions.end())
		{
			auto t = static_cast<float>(frameNo - rit->frameNo) / static_cast<float>(it->frameNo - rit->frameNo);

			rotation = XMMatrixRotationQuaternion(XMQuaternionSlerp(rit->quaternion, it->quaternion, t));
		}
		else
		{
			rotation = XMMatrixRotationQuaternion(rit->quaternion);
		}

		auto& pos = node.startPos;

		// ���݂̃t���[���ƈ�v����t���[���ԍ��������[�V�������g�p����
		auto mat = XMMatrixTranslation(-pos.x, -pos.y, -pos.z)
			* rotation
			* XMMatrixTranslation(pos.x, pos.y, pos.z);

		mBoneMatrices[node.boneIdx] = mat;
	}
	RecursiveMatrixMultiply(&mBoneNodeTable["�Z���^�["], XMMatrixIdentity());
	copy(mBoneMatrices.begin(), mBoneMatrices.end(), mMappedMatrices + 1);
}