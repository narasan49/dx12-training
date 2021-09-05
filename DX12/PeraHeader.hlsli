Texture2D<float4> tex : register(t0);
Texture2D<float4> texNormal : register(t1);
Texture2D<float4> texHighLum : register(t2);
Texture2D<float4> texShrinkHighLum : register(t3);
Texture2D<float4> texShrink : register(t4);

Texture2D<float> depthTex : register(t5);
Texture2D<float> lightDepthTex : register(t6);

// ルートパラメータ4番
Texture2D<float> texSsao : register(t7);

SamplerState smp : register(s0);

// 定数バッファ
cbuffer SceneData : register(b0)
{
	matrix view;
	matrix proj;
	matrix invproj;
	matrix lightCamera;
	matrix shadow;
	float3 eye;
}

struct Output
{
	float4 svpos : SV_POSITION;
	float4 pos : POSITION;
	float2 uv : TEXCOORD;
};

struct BlurOutput
{
	float4 highLum : SV_TARGET0;
	float4 col : SV_TARGET1;
};

