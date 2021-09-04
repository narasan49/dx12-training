#pragma once

#include <string>
#include <vector>
#include <DirectXMath.h>
using namespace DirectX;
using namespace std;

// PMDヘッダー
struct PMDHeader
{
	float version;
	char model_name[20];
	char comment[256];
};

struct PMDVertex
{
	// 頂点座標 12 bytes
	XMFLOAT3 pos;
	// 法線ベクトル 12 bytes
	XMFLOAT3 normal;
	// uv座標 8 bytes
	XMFLOAT2 uv;
	// ボーン番号 4byte
	unsigned short boneNo[2];
	// ボーン重み 1 byte
	unsigned char boneWeight;
	// 輪郭線フラグ 1 byte
	unsigned char edgeFlg;
};
#pragma pack(1)

// PMDマテリアル構造体
struct PMDMaterial
{
	// ディヒューズ色
	XMFLOAT3 diffuse;
	// ディフーズアルファ
	float alpha;
	// すぺきゅらの強さ
	float specularity;
	// スぺキュラ色
	XMFLOAT3 specular;
	// アンビエント色
	XMFLOAT3 ambient;
	// トゥーン番号
	unsigned char toonIdx;
	// マテリアルごとの輪郭線フラグ
	unsigned char edgeFlg;

	// ここに2バイトのパディング

	// このマテリアルが割り当てられるインデックス数
	unsigned int indicesNum;
	// テクスチャファイルパス
	char texFilePath[20];
};
#pragma pack()

// 読み込み用ボーン構造体
#pragma pack(1)
struct PMDBone
{
	// ボーン名
	char boneName[20];
	// 親ボーン番号
	unsigned short parentNo;
	// 先端のボーン番号
	unsigned short nextNo;
	// ボーン種別
	unsigned char type;
	// IKボーン番号
	unsigned short ikBoneNo;
	// ボーンの基準点座標
	XMFLOAT3 pos;
};
#pragma pack()

struct VMDMotion
{
	// ボーン名
	char boneName[15];
	// フレーム番号
	unsigned int frameNo;
	// 位置
	XMFLOAT3 location;
	// クォターニオン
	XMFLOAT4 quaternion;
	// ベジェ補間パラメータ
	unsigned char bezier[64];
};

struct Motion
{
	// アニメーション開始からのフレーム数
	unsigned int frameNo;
	// クォターニオン
	XMVECTOR quaternion;

	Motion(unsigned int fno, const XMVECTOR& q) 
		: frameNo(fno), quaternion(q)
	{ }
};

// シェーダーに送られるマテリアルデータ
struct MaterialForHlsl
{
	// ディヒューズ色
	XMFLOAT3 diffuse;
	// ディヒューズアルファ
	float alpha;
	// スぺキュラ色
	XMFLOAT3 specular;
	// スぺキュラ強度
	float specularity;
	// アンビエント色
	XMFLOAT3 ambient;
};

// それ以外のマテリアルデータ
struct AdditionalMaterial
{
	// テクスチャファイルパス
	string texPath;
	// トゥーン番号
	int toonIdx;
	// マテリアルごとの輪郭線フラグ
	bool edgeFlg;
};

// マテリアル
struct Material
{
	unsigned int indicesNum;
	MaterialForHlsl	material;
	AdditionalMaterial additional;
};

struct Transform 
{
	XMMATRIX world;

	// XMMATRIXは16バイトアライメントされている。
	void* operator new(size_t size)
	{
		return _aligned_malloc(size, 16);
	}
};

struct BoneNode
{
	// ボーンインデックス
	int boneIdx;
	// ボーン基準点
	XMFLOAT3 startPos;
	// ボーン先端
	XMFLOAT3 endPos;
	// 子ノード
	vector<BoneNode*> children;
};

string GetTexturePathFromModelAndTexPath(const string& modelPath, const char* texPath);
wstring GetWideStringFromString(const string& str);
string GetExtention(const string& path);
std::vector<string> SplitFileName(const string& path, const char splitter = '*');