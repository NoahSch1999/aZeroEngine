// Simple mesh shader that emits one triangle
struct CameraData
{
    float4x4 ViewProjectionMatrix;
};

ConstantBuffer<CameraData> VertexPerPassData : register(b0);

StructuredBuffer<float3> PositionBuffer : register(t0);

struct VertexOut
{
    float4 Position : SV_Position;
    float3 WorldPosition : WorldPosition;
    float2 UV : UV;
    float3 Normal : NORMAL;
    float3 Tangent : TANGENT;
    float3 Bitangent : BITANGENT;
    float3x3 TBN : TBN;
};

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
    verts[0].Position = float4(PositionBuffer[0].x, PositionBuffer[0].y, PositionBuffer[0].z, 1.0f);
    verts[1].Position = float4(0.5f, -0.5f, 0.0f, 1.0f);
    verts[2].Position = float4(-0.5f, -0.5f, 0.0f, 1.0f);

    // Per-vertex color
    verts[0].Normal = float3(1, 0, 0);
    verts[1].Normal = float3(0, 1, 0);
    verts[2].Normal = float3(VertexPerPassData.ViewProjectionMatrix[0][0], 0, 1);

    verts[0].WorldPosition = 1.f;
    verts[0].Bitangent = 1.f;
    verts[0].Normal = 1.f;
    verts[0].UV = 1.f;
    verts[0].Tangent = 1.f;
    verts[0].TBN = VertexPerPassData.ViewProjectionMatrix;
    // One triangle using the three vertices
    tris[0] = uint3(0, 1, 2);
}
