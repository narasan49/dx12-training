#include "BasicShaderHeader.hlsli"

PixelOutput BasicPS(Output input) : SV_TARGET
{
	//if (input.instNo == 1)
	//{
	//	return float4(0, 0, 0, 1);
	//}
	// �V���h�E�}�b�v
	// ���C�g�̃X�N���[���ɓ��e�������_���W(VS�Ōv�Z)�B
	float3 posFromLight = input.tpos.xyz / input.tpos.w;
	float2 shadowUV = (posFromLight.xy + float2(1.0f, -1.0f)) * float2(0.5, -0.5);
	// ���C�g���猩���[�x
	//float depthFromLight = lightDepthTex.Sample(smp, shadowUV);
	//float shadowWeight = 1.0f;
	//if (depthFromLight < posFromLight.z - 0.001f)
	//{
	//	shadowWeight = 0.5;
	//}
	float depthFromLight = lightDepthTex.SampleCmp(
		shadowSmp, 
		shadowUV, 
		posFromLight.z - 0.005f);
	float shadowWeight = lerp(0.5f, 1.0f, depthFromLight);

	float3 light = normalize(float3(1, -1, 1));
	// diffuse�v�Z
	float brightness = saturate(dot(-light, input.normal));
	float toonBrightness = toon.Sample(toonSmp, float2(0, 1.0 - brightness));

	// ���̔��˃x�N�g��
	float3 reflectLight = normalize(reflect(light, input.normal.xyz));
	float specularB = pow(saturate(dot(reflectLight, -input.ray)), specular.a);

	// �e�N�X�`���J���[
	float4 color = tex.Sample(smp, input.uv);
	
	// �X�t�B�A�}�b�vUV(0-1�ɋK�i��)
	float2 sphereMapUV = input.vnormal.xy;
	sphereMapUV = (sphereMapUV + float2(1, -1)) * float2(0.5, -0.5);

	float4 diffuseColor = toonBrightness * diffuse * color;
	float4 specularColor = float4(specularB * specular.rbg, 1);
	float4 ambientColor = float4(0.5 * ambient * color, 1);
	float4 sphColor = sph.Sample(smp, sphereMapUV);
	float4 spaColor = spa.Sample(smp, sphereMapUV);

	float4 resultantColor = max(diffuseColor * sphColor + specularColor + spaColor * color, ambientColor);
	float4 ret = saturate(resultantColor * shadowWeight);
	float gray = dot(resultantColor.rgb, float3(0.299, 0.587, 0.114));
	PixelOutput output;
	output.col = ret;
	output.normal.xyz = input.normal.xyz;
	output.normal.a = 1;
	output.highLum = (gray > .8f) ? ret : 0.0f;

	return output;
}
