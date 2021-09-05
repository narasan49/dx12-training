#include "PeraHeader.hlsli"

float random(float2 uv, int seed)
{
	return frac(sin(dot(uv, float2(12.9898, 78.233)) + seed) * 43758.5453);
}

float SsaoPs(Output input) : SV_TARGET
{
	float depth = depthTex.Sample(smp, input.uv);
	float w, h, miplevels;
	depthTex.GetDimensions(0, w, h, miplevels);
	float dx = 1.0f / w;
	float dy = 1.0f / h;
	float4 screenPos = float4(input.uv * float2(2, -2) + float2(-1, 1), depth, 1);
	float4 restoredPos = mul(invproj, screenPos);
	restoredPos.xyz = restoredPos.xyz / restoredPos.w;

	float total = 0.0f;
	float ao = 0.0f;
	float3 norm = normalize((texNormal.Sample(smp, input.uv).xyz * 2) - 1);
	const int tryCount = 256;
	const float radius = 0.5f;

	if (depth >= 1.0f)
	{
		return 1;
	}

	for (int i = 0; i < tryCount; i++)
	{
		float rnd1 = random(float2(i * dx, i * dy), i) * 2 - 1;
		float rnd2 = random(float2(rnd1, i * dy), i + 1) * 2 - 1;
		float rnd3 = random(float2(rnd1, rnd2), i + 2) * 2 - 1;
		float3 omega = normalize(float3(rnd1, rnd2, rnd3));

		float dt = dot(norm, omega);
		float sgn = sign(dt);
		omega *= sgn;
		dt *= sgn;

		float4 screePosOfAmbient = mul(proj, float4(restoredPos.xyz + omega * radius, 1));
		screePosOfAmbient.xyz /= screePosOfAmbient.w;
		total += dt;

		float2 screenToUV = (screePosOfAmbient.xy + float2(1, -1)) * float2(0.5, -0.5);
		float depthAmbientScreenPos = depthTex.Sample(smp, screenToUV);
		//if (depthTex.Sample(smp, (screePosOfAmbient.xy, float2(1, -1)) * float2(0.5, -0.5)) > screePosOfAmbient.z)
		//{
		//	ao += dt;
		//}
		ao += step(depthAmbientScreenPos, screePosOfAmbient.z) * dt;
	}

	ao /= total;

	return 1.0f - ao;
}