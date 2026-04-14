struct CameraData
{
    float4x4 mat;
};

struct InputData
{
    uint VertexID : SV_VertexID;
    uint InstanceID : SV_InstanceID;
};

struct VertexOut
{
    float4 position : SV_Position;
};

ConstantBuffer<CameraData> CameraMatrix : register(b0);
StructuredBuffer<float3> PositionBuffer : register(t0);

[numthreads(2, 3, 4)]
[outputtopology("triangle")]
void main(
    uint3 dispatchThreadID : SV_DispatchThreadID,
    out vertices VertexOut verts[3],
    out indices uint3 tris[1]
)
{
    // Declare how many vertices and primitives we will emit
    SetMeshOutputCounts(3, 1);

    // Triangle vertices in clip space
    verts[0].position = float4(PositionBuffer[0].x, PositionBuffer[0].y, PositionBuffer[0].z, 1.f);
    tris[0] = uint3(0, 1, 2);
}
