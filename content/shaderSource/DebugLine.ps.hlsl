struct Vertex
{
    float4 Position : SV_Position;
    float3 Color : COLOR;
};

float4 main(Vertex vert) : SV_Target0
{
    return float4(vert.Color, 1);
}