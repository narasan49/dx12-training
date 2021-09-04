
struct vertex
{
	float3 x;
	float2 v;
	float h;
};

StructuredBuffer<vertex> inputBuffer : register(t0);