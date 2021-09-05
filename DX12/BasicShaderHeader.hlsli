struct Output {
	float4 svpos : SV_POSITION;
	float4 pos : POSITION;
	float4 tpos : TPOS;
	float4 normal : NORMAL0;
	float4 vnormal : NORMAL1;
	float3 ray : VECTOR;
	float2 uv : TEXCOORD;
	uint instNo : SV_InstanceID;
};

struct PixelOutput
{
	float4 col : SV_TARGET0;
	float4 normal : SV_TARGET1;
	float4 highLum : SV_TARGET2;
};

//0�ԃX���b�g�ɐݒ肳�ꂽ�e�N�X�`��
Texture2D<float4> tex : register(t0);
Texture2D<float4> sph : register(t1);
Texture2D<float4> spa : register(t2);
Texture2D<float4> toon : register(t3);
Texture2D<float> lightDepthTex : register(t4);

//0�ԃX���b�g�ɐݒ肳�ꂽ�T���v���[
SamplerState smp : register(s0);
//1�ԃX���b�g�ɐݒ肳�ꂽ�T���v���[	
SamplerState toonSmp : register(s1);
SamplerComparisonState shadowSmp : register(s2);

// �萔�o�b�t�@
cbuffer SceneData : register(b0)
{
	matrix view;
	matrix proj;
	matrix invproj;
	matrix lightCamera;
	matrix shadow;
	float3 eye;
}
cbuffer Transform : register(b1)
{
	// ���[���h�s��
	matrix world;
	// �{�[���s��
	matrix bones[256];
}

cbuffer Material : register(b2)
{
	float4 diffuse;
	float4 specular;
	float3 ambient;
}