
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
    int pad1;
    int pad2;
};

ConstantBuffer<CameraData> VertexPerPassData : register(b0);
ConstantBuffer<VSPerBatchConstants> PerBatchConstantsBuffer : register(b1);

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
    unsigned int Index = IndexBuffer.Load(ID);
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
    float3 Bitangent : BITANGENT;
    float3x3 TBN : TBN;
};

OutputData main(InputData Input)
{
    float f = 1.0 / tan(1.047 * 0.5);
    float zn = 0.1;
    float zf = 1000;
    float4x4 Projection =
    {
        f / (1080/1920), 0,  0,                        0,
        0,          f,  0,                        0,
        0,          0,  zf / (zf - zn),           1,
        0,          0, (-zn * zf) / (zf - zn),    0
    };

    const MeshEntry Mesh = MeshEntryBuffer.Load(PerBatchConstantsBuffer.MeshEntryIndex);
    const VertexData Vertex = GetVertexFromID(Mesh, Input.VertexID);
    const float4x4 WorldMatrix = InstanceBuffer.Load(PerBatchConstantsBuffer.StartInstanceOffset + Input.InstanceID).WorldMatrix;
    //const float4x4 WorldMatrix = {1, 0, 0, 0,0, 1, 0, 0,0, 0, 1, 0,0, 0, 100, 1};

    OutputData Output;
    Output.Position = mul(WorldMatrix, float4(Vertex.Position, 1.f));
    Output.WorldPosition = Output.Position.xyz;
    Output.Position = mul(VertexPerPassData.ViewProjectionMatrix, Output.Position);
    //Output.Position = mul(Projection, Output.Position);
    
    Output.UV = Vertex.UV;
    
    float3 Normal = normalize(mul(WorldMatrix, float4(Vertex.Normal, 0.f))).xyz;
    float3 Tangent = normalize(mul(WorldMatrix, float4(Vertex.Tangent, 0.f))).xyz;
    
    //Tangent = Tangent - Normal * dot(Normal, Tangent);
    //Tangent = normalize(Tangent);
    
    const float3 BiTangent = normalize(cross(Tangent, Normal));
    Output.Normal = Normal;
    Output.TBN = float3x3(Tangent, BiTangent, Normal);
    Output.Tangent = Tangent;
    Output.Bitangent = BiTangent;
    
    return Output;
}