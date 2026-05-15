struct Camera
{
    float4x4 VP;
};
ConstantBuffer<Camera> VP_CONSTANT : register(b0);

struct Vertex
{
    float3 Position : POSITION;
    float3 Color : COLOR;
};

struct Out
{
    float4 Position : SV_Position;
    float3 Color : COLOR;
};

Out main(Vertex vert) 
{
    Out output;
    output.Position = mul(VP_CONSTANT.VP, float4(vert.Position, 1.f));
    output.Color = vert.Color;
    return output;
}