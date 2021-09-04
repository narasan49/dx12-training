#include "PeraHeader.hlsli"

float4 blur(texture2D<float4> tex, SamplerState smp, float2 uv, float dx, float dy)
{
	float4 filtered = float4(0, 0, 0, 0);
	filtered += tex.Sample(smp, uv + float2(-2 * dx, -2 * dy));
	filtered += tex.Sample(smp, uv + float2(0, -2 * dy));
	filtered += tex.Sample(smp, uv + float2(2 * dx, -2 * dy));

	filtered += tex.Sample(smp, uv + float2(-2 * dx, 0));
	filtered += tex.Sample(smp, uv);
	filtered += tex.Sample(smp, uv + float2(2 * dx, 0));

	filtered += tex.Sample(smp, uv + float2(-2 * dx, 2 * dy));
	filtered += tex.Sample(smp, uv + float2(0, 2 * dy));
	filtered += tex.Sample(smp, uv + float2(2 * dx, 2 * dy));
	return filtered / 9.0f;
}

float4 blurByShrinkTex(texture2D<float4> tex, SamplerState smp, float2 uv)
{
	float w, h, levels;
	tex.GetDimensions(0, w, h, levels);
	float dx = 1.0f / w;
	float dy = 1.0f / h;

	float4 accumulated = float4(0, 0, 0, 0);
	float2 uvSize = float2(1, 0.5);
	float2 uvOffset = float2(0, 0);

	for (int i = 0; i < 8; i++)
	{
		accumulated += blur(tex, smp, uv * uvSize + uvOffset, dx, dy) / 8.0f;
		uvOffset.y += uvSize.y;
		uvSize *= 0.5f;
	}

	return accumulated;
}

float4 dofFilter(float2 uv)
{
	float depthFromCenter = abs(depthTex.Sample(smp, float2(.5, .5)) - depthTex.Sample(smp, uv));
	depthFromCenter = pow(depthFromCenter, 0.6f);
	float t = depthFromCenter * 8;
	float intPart;
	t = modf(t, intPart);

	float w, h, levels;
	tex.GetDimensions(0, w, h, levels);
	float dx = 1.0f / w;
	float dy = 1.0f / h;
	float2 uvOffset = float2(0, 0);
	float2 uvSize = float2(1, 0.5);
	for (int i = 0; i < intPart; i++)
	{
		uvOffset.y += uvSize.y;
		uvSize *= 0.5f;
	}

	float4 lerpX;
	float4 lerpY;
	if (intPart == 0)
	{
		lerpX = tex.Sample(smp, uv);
		lerpY = blur(texShrink, smp, uv * uvSize + uvOffset, dx, dy);
	}
	else
	{
		lerpX = blur(texShrink, smp, uv * uvSize + uvOffset, dx, dy);
		uvOffset.y += uvSize.y;
		uvSize *= 0.5f;
		lerpY = blur(texShrink, smp, uv * uvSize + uvOffset, dx, dy);
	}

	return lerp(lerpX, lerpY, t);
}

float4 ps(Output input) : SV_TARGET
{
	if (input.uv.x < 0.2)
	{
		if (input.uv.y < 0.2)
		{
			// 深度出力
			float depth = depthTex.Sample(smp, input.uv * 5);
			depth = pow(depth, 20);
			return float4(depth, depth, depth, 1);
		}
		else if (input.uv.y < 0.4)
		{
			// ライトからの深度
			float depth = depth = lightDepthTex.Sample(smp, (input.uv - float2(0, 0.2)) * 5);
			return float4(depth, depth, depth, 1);
		}
		else if (input.uv.y < 0.6)
		{
			// 法線
			return texNormal.Sample(smp, (input.uv - float2(0, 0.4)) * 5);
		}
		else if (input.uv.y < 0.8)
		{
			return texHighLum.Sample(smp, (input.uv - float2(0, 0.6)) * 5);
		}
		else if (input.uv.y < 1.0)
		{
			return texShrinkHighLum.Sample(smp, (input.uv - float2(0, 0.8)) * 5);
		}
	}
	else if (input.uv.x < 0.4)
	{
		if (input.uv.y < 0.2)
		{
			return texShrink.Sample(smp, input.uv * 5);
		}
	}

	float4 color = tex.Sample(smp, input.uv);
	float gray = dot(color.rgb, float3(0.299, 0.587, 0.114));
	
	// テクスチャのサイズ情報
	float w, h, levels;
	tex.GetDimensions(0, w, h, levels);
	float dx = 1.0f / w;
	float dy = 1.0f / h;

	//float4 bloom = blur(texHighLum, smp, input.uv, dx, dy);
	float4 bloomAccumulated = blurByShrinkTex(texShrinkHighLum, smp, input.uv);

	return saturate(bloomAccumulated) + dofFilter(input.uv);
}

BlurOutput BlurPS(Output input) : SV_TARGET
{
	float w, h, miplevels;
	tex.GetDimensions(0, w, h, miplevels);
	float dx = 1.0f / w;
	float dy = 1.0f / h;
	BlurOutput outCol;
	outCol.col = blur(tex, smp, input.uv, dx, dy);
	outCol.highLum = blur(texHighLum, smp, input.uv, dx, dy);

	return outCol;
}