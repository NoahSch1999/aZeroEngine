
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
    float4x4 ViewProjectionMatrix;
};

struct InstanceData
{
    float4x4 WorldMatrix;
};

struct VSPerBatchConstants
{
    unsigned int StartInstanceOffset;
    unsigned int MeshEntryIndex;
};

ConstantBuffer<CameraData> VertexPerPassData : register(b0);
ConstantBuffer<PerBatchConstants> PerBatchConstantsBuffer : register(b1);

StructuredBuffer<float3> PositionBuffer : register(t0);
StructuredBuffer<float2> UVBuffer : register(t1);
StructuredBuffer<float3> NormalBuffer : register(t2);
StructuredBuffer<float3> TangentBuffer : register(t3);
StructuredBuffer<unsigned int> IndexBuffer : register(t4);
StructuredBuffer<MeshEntry> MeshEntryBuffer : register(t5);
StructuredBuffer<InstanceData> InstanceBuffer : register(t6);

VertexData GetVertexFromChannels(unsigned int Index)
{
    VertexData vertex;
    vertex.Position = PositionBuffer.Load(Index);
    vertex.UV = UVBuffer.Load(Index);
    vertex.Normal = NormalBuffer.Load(Index);
    vertex.Tangent = TangentBuffer.Load(Index);
    return vertex;
}

VertexData GetVertexFromID(
    in MeshEntry Mesh, unsigned int ID)
{
    unsigned int Index = IndexBuffer.Load(ID + Mesh.IndexStartOffset);
    return GetVertexFromChannels(Index + Mesh.VertexStartOffset);
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
    float3 Tangent : TANGENT;
    float3 BiTangent : BITANGENT;
};

OutputData main(InputData Input)
{
    const MeshEntry Mesh = MeshEntryBuffer.Load(PerBatchConstantsBuffer.MeshEntryIndex);
    const VertexData Vertex = GetVertexFromID(Mesh, Input.VertexID);
    //VertexData Vertex;
    //Vertex.Position = float3(0, 0, 0);
    //Vertex.Normal = float3(1, 0, 0);
    //Vertex.Tangent = float3(0, 1, 0);
    //Vertex.UV = float2(1, 0);
    const float4x4 WorldMatrix = InstanceBuffer.Load(PerBatchConstantsBuffer.StartInstanceOffset + Input.InstanceID).WorldMatrix;
    
    OutputData Output;
    Output.Position = mul(WorldMatrix, float4(Vertex.Position, 1.f));
    Output.WorldPosition = Output.Position.xyz;
    Output.Position = mul(VertexPerPassData.ViewProjectionMatrix, Output.Position);
    
    Output.UV = Vertex.UV;
    
    const float3 Normal = mul(WorldMatrix, float4(Vertex.Normal, 0.f)).xyz;
    const float3 Tangent = mul(WorldMatrix, float4(Vertex.Tangent, 0.f)).xyz;
    const float3 BiTangent = cross(Normal, Tangent);
    //const float3 BiTangent = cross(Tangent, Normal);
    
    Output.Normal = Normal;
    Output.Tangent = Tangent;
    Output.BiTangent = BiTangent;
    //Output.TBN = float3x3(Tangent, BiTangent, Normal);
    
    return Output;
}