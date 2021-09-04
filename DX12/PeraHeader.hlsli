Texture2D<float4> tex : register(t0);
Texture2D<float4> texNormal : register(t1);
Texture2D<float4> texHighLum : register(t2);
Texture2D<float4> texShrinkHighLum : register(t3);
Texture2D<float4> texShrink : register(t4);

Texture2D<float> depthTex : register(t5);
Texture2D<float> lightDepthTex : register(t6);
SamplerState smp : register(s0);

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