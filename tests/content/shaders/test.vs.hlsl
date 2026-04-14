struct CameraData
{
    float4x4 mat;
};

ConstantBuffer<CameraData> CameraMatrix : register(b0);
StructuredBuffer<float3> PositionBuffer : register(t0);

struct InputData
{
    uint VertexID : SV_VertexID;
    uint InstanceID : SV_InstanceID;
};

struct OutputData
{
    float4 position : SV_Position;
};

OutputData main(InputData input)
{
    OutputData output;
    output.position = float4(PositionBuffer.Load(0), 1.f);
    output.position = mul(output.position, CameraMatrix.mat);
    return output;
}