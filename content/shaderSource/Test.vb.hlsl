#include "GPU_Structs.hlsli"

RasterVertex main(Vertex vert)
{
    RasterVertex output;
    output.Position = mul(VP_CONSTANT.VP, float4(vert.Position, 1.f));
    output.Color = vert.Color;
    return output;
}