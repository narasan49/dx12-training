#include "Fluid2D.hlsli"

[numthreads(1, 1, 1)]
void main( uint3 DTid : SV_DispatchThreadID )
{
	uint idx = DTid.x;
	inputBuffer[idx].x;
}