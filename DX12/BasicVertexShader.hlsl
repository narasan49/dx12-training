#include "BasicShaderHeader.hlsli"

Output BasicVS(
	float4 pos : POSITION,
	float4 normal : NORMAL,
	float2 uv : TEXCOORD,
	min16uint2 boneno : BONE_NO,
	min16uint weight : WEIGHT,
	uint instNo : SV_InstanceID
)
{
	float w = weight / 100.0f;
	Output output;
	//output.pos = pos;
	// ボーン行列による変形
	matrix boneMat = bones[boneno.x] * w + bones[boneno.y] * (1 - w);
	//pos = mul(bones[boneno[0]], pos);
	pos = mul(boneMat, pos);
	output.pos = mul(world, pos);

	if (instNo == 1)
	{
		// 影をおとす
		output.pos = mul(shadow, output.pos);
	}

	output.svpos = mul(mul(proj, view), output.pos);
	//output.svpos = mul(lightCamera, output.pos);
	normal.w = 0; // 平行移動成分を0にする
	output.normal = mul(world, normal);
	output.vnormal = mul(view, output.normal);
	output.ray = normalize(pos.xyz - mul(view, eye));
	output.uv = uv;
	output.instNo = instNo;
	output.tpos = mul(lightCamera, output.pos);
	return output;
}

float4 ShadowVS(
	float4 pos : POSITION,
	float4 normal : NORMAL,
	float2 uv : TEXCOORD,
	min16uint2 boneno : BONE_NO,
	min16uint weight : WEIGHT) : SV_POSITION
{
	float w = float(weight) / 100.0f;
	matrix boneMat = bones[boneno.x] * w + bones[boneno.y] * (1.0f - w);
	pos = mul(world, mul(boneMat, pos));
	return mul(lightCamera, pos);
}