
// Expects column-major matrix by default
// Simplemath stores in row-major

struct VertexData
{
    float3 Position;
    float2 UV;
    float3 Normal;
    float3 Tangent;
};

struct MeshEntry
{
    unsigned int VertexStartOffset;
    unsigned int IndexStartOffset;
    unsigned int NumIndices;
};

struct CameraData
{
    row_major float4x4 ViewProjectionMatrix;
};

struct InstanceData
{
    row_major float4x4 WorldMatrix;
};

struct PerBatchConstants
{
    unsigned int StartInstanceOffset;
    unsigned int MeshEntryIndex;
};

ConstantBuffer<CameraData> CameraDataBuffer : register(b0);
ConstantBuffer<PerBatchConstants> PerBatchConstantsBuffer : register(b1);

StructuredBuffer<VertexData> VertexBuffer : register(t1);
StructuredBuffer<unsigned int> IndexBuffer : register(t2);
StructuredBuffer<MeshEntry> MeshEntries : register(t3);
StructuredBuffer<InstanceData> InstanceBuffer : register(t4);

VertexData GetVertexFromID(
    in const StructuredBuffer<VertexData> VertexBuffer,
    in const StructuredBuffer<uint> IndexBuffer,
    in MeshEntry Mesh, unsigned int ID)
{
    unsigned int Index = IndexBuffer.Load(ID + Mesh.IndexStartOffset);
    return VertexBuffer.Load(Index + Mesh.VertexStartOffset);
}

struct InputData
{
    uint VertexID : SV_VertexID;
    uint InstanceID : SV_InstanceID;
};

struct OutputData
{
    float4 Position : SV_Position;
    float3 WorldPosition : WorldPosition;
    float2 UV : UV;
    float3 Normal : NORMAL;
    float3x3 TBN : TBN;
};

OutputData main(InputData Input)
{
    const MeshEntry Mesh = MeshEntries.Load(PerBatchConstantsBuffer.MeshEntryIndex);
    const VertexData Vertex = GetVertexFromID(VertexBuffer, IndexBuffer, Mesh, Input.VertexID);
    
    const row_major float4x4 WorldMatrix = InstanceBuffer.Load(PerBatchConstantsBuffer.StartInstanceOffset + Input.InstanceID).WorldMatrix;
    
    OutputData Output;
    Output.Position = mul(float4(Vertex.Position, 1.f), WorldMatrix);
    Output.WorldPosition = Output.Position.xyz;
    Output.Position = mul(Output.Position, CameraDataBuffer.ViewProjectionMatrix);
    
    Output.UV = Vertex.UV;
    
    const float3 Normal = mul(float4(Vertex.Normal, 0.f), WorldMatrix).xyz;
    const float3 Tangent = mul(float4(Vertex.Tangent, 0.f), WorldMatrix).xyz;
    const float3 Bitangent = cross(Tangent, Normal);
    
    Output.Normal = Normal;
    Output.TBN = float3x3(Tangent, Bitangent, Normal);
    
    return Output;
}