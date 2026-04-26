struct VertexOut
{
    float4 Position : SV_Position;
    float2 UV : UV;
#if !NORMAL_MAP
    float3 Normal : NORMAL;
    float3x3 TBN : TBN;
#endif
    nointerpolation min16uint MaterialID : Material;
    float3 MeshletColor : MESHLET_COLOR;
};