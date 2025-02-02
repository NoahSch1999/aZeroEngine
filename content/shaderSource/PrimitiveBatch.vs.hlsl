struct Input
{
    float3 Position : POSITION;
    float3 Color : COLOR;
};

struct Output
{
    float4 Position : SV_Position;
    float3 Color : COLOR;
};

struct CameraData
{
    /*row_major*/ float4x4 VPMatrix;
};

ConstantBuffer<CameraData> CameraDataBuffer : register(b0);

Output main(Input input)
{
    Output output;
   // output.Position = mul(float4(input.Position, 1.f), CameraDataBuffer.VPMatrix);
    output.Position = mul(CameraDataBuffer.VPMatrix, float4(input.Position, 1.f));
    output.Color = input.Color;
    return output;
}