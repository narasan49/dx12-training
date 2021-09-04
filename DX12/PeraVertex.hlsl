#include "PeraHeader.hlsli"

Output vs( float4 pos : POSITION, float2 uv : TEXCOORD )
{
	Output output;
	output.svpos = pos;
	output.pos = pos;
	output.uv = uv;
	return output;
}