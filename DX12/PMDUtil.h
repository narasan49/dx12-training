#pragma once

#include <string>
#include <vector>
#include <DirectXMath.h>
using namespace DirectX;
using namespace std;

// PMD�w�b�_�[
struct PMDHeader
{
	float version;
	char model_name[20];
	char comment[256];
};

struct PMDVertex
{
	// ���_���W 12 bytes
	XMFLOAT3 pos;
	// �@���x�N�g�� 12 bytes
	XMFLOAT3 normal;
	// uv���W 8 bytes
	XMFLOAT2 uv;
	// �{�[���ԍ� 4byte
	unsigned short boneNo[2];
	// �{�[���d�� 1 byte
	unsigned char boneWeight;
	// �֊s���t���O 1 byte
	unsigned char edgeFlg;
};
#pragma pack(1)

// PMD�}�e���A���\����
struct PMDMaterial
{
	// �f�B�q���[�Y�F
	XMFLOAT3 diffuse;
	// �f�B�t�[�Y�A���t�@
	float alpha;
	// ���؂����̋���
	float specularity;
	// �X�؃L�����F
	XMFLOAT3 specular;
	// �A���r�G���g�F
	XMFLOAT3 ambient;
	// �g�D�[���ԍ�
	unsigned char toonIdx;
	// �}�e���A�����Ƃ̗֊s���t���O
	unsigned char edgeFlg;

	// ������2�o�C�g�̃p�f�B���O

	// ���̃}�e���A�������蓖�Ă���C���f�b�N�X��
	unsigned int indicesNum;
	// �e�N�X�`���t�@�C���p�X
	char texFilePath[20];
};
#pragma pack()

// �ǂݍ��ݗp�{�[���\����
#pragma pack(1)
struct PMDBone
{
	// �{�[����
	char boneName[20];
	// �e�{�[���ԍ�
	unsigned short parentNo;
	// ��[�̃{�[���ԍ�
	unsigned short nextNo;
	// �{�[�����
	unsigned char type;
	// IK�{�[���ԍ�
	unsigned short ikBoneNo;
	// �{�[���̊�_���W
	XMFLOAT3 pos;
};
#pragma pack()

struct VMDMotion
{
	// �{�[����
	char boneName[15];
	// �t���[���ԍ�
	unsigned int frameNo;
	// �ʒu
	XMFLOAT3 location;
	// �N�H�^�[�j�I��
	XMFLOAT4 quaternion;
	// �x�W�F��ԃp�����[�^
	unsigned char bezier[64];
};

struct Motion
{
	// �A�j���[�V�����J�n����̃t���[����
	unsigned int frameNo;
	// �N�H�^�[�j�I��
	XMVECTOR quaternion;

	Motion(unsigned int fno, const XMVECTOR& q) 
		: frameNo(fno), quaternion(q)
	{ }
};

// �V�F�[�_�[�ɑ�����}�e���A���f�[�^
struct MaterialForHlsl
{
	// �f�B�q���[�Y�F
	XMFLOAT3 diffuse;
	// �f�B�q���[�Y�A���t�@
	float alpha;
	// �X�؃L�����F
	XMFLOAT3 specular;
	// �X�؃L�������x
	float specularity;
	// �A���r�G���g�F
	XMFLOAT3 ambient;
};

// ����ȊO�̃}�e���A���f�[�^
struct AdditionalMaterial
{
	// �e�N�X�`���t�@�C���p�X
	string texPath;
	// �g�D�[���ԍ�
	int toonIdx;
	// �}�e���A�����Ƃ̗֊s���t���O
	bool edgeFlg;
};

// �}�e���A��
struct Material
{
	unsigned int indicesNum;
	MaterialForHlsl	material;
	AdditionalMaterial additional;
};

struct Transform 
{
	XMMATRIX world;

	// XMMATRIX��16�o�C�g�A���C�����g����Ă���B
	void* operator new(size_t size)
	{
		return _aligned_malloc(size, 16);
	}
};

struct BoneNode
{
	// �{�[���C���f�b�N�X
	int boneIdx;
	// �{�[����_
	XMFLOAT3 startPos;
	// �{�[����[
	XMFLOAT3 endPos;
	// �q�m�[�h
	vector<BoneNode*> children;
};

string GetTexturePathFromModelAndTexPath(const string& modelPath, const char* texPath);
wstring GetWideStringFromString(const string& str);
string GetExtention(const string& path);
std::vector<string> SplitFileName(const string& path, const char splitter = '*');