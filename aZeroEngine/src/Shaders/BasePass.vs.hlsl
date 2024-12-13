struct PrimitiveRenderData
{
    row_major float4x4 WorldMatrix;
    unsigned int MeshEntryIndex;
};

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

struct ViewMatrices
{
    row_major float4x4 ViewMatrix;
    row_major float4x4 ProjectionMatrix;
};

struct PerDrawConstants
{
    int PrimitiveIndex;
};

ConstantBuffer<ViewMatrices> ViewMatricesBuffer : register(b0);
ConstantBuffer<PerDrawConstants> PerDrawConstantsBuffer : register(b1);
StructuredBuffer<PrimitiveRenderData> PrimitiveRenderDataBuffer : register(t0);
StructuredBuffer<VertexData> VertexBuffer : register(t1);
StructuredBuffer<unsigned int> IndexBuffer : register(t2);
StructuredBuffer<MeshEntry> MeshEntries : register(t3);

VertexData GetVertexFromID(
    in const StructuredBuffer<VertexData> VertexBuffer, 
    in const StructuredBuffer<uint> IndexBuffer,
    in MeshEntry Mesh, unsigned int ID)
{
    unsigned int Index = IndexBuffer.Load(ID + Mesh.IndexStartOffset);
    return VertexBuffer.Load(Index + Mesh.VertexStartOffset);
}

// Expects column-major matrix by default
// Simplemath stores in row-major

struct OutputData
{
    float4 Position : SV_Position;
    float3 WorldPosition : WorldPosition;
    float2 UV : UV;
    float3 Normal : NORMAL;
    float3x3 TBN : TBN;
};

OutputData main(uint VertexID : SV_VertexID)
{
    const PrimitiveRenderData PrimitiveData = PrimitiveRenderDataBuffer.Load(PerDrawConstantsBuffer.PrimitiveIndex);
    
    const MeshEntry Mesh = MeshEntries.Load(PrimitiveData.MeshEntryIndex);
    const VertexData Vertex = GetVertexFromID(VertexBuffer, IndexBuffer, Mesh, VertexID); // 0 should be VS index
    
    const row_major float4x4 WorldMatrix = PrimitiveData.WorldMatrix;
    
    OutputData Output;
    Output.Position = mul(float4(Vertex.Position, 1.f), WorldMatrix);
    Output.WorldPosition = Output.Position;
    Output.Position = mul(Output.Position, ViewMatricesBuffer.ViewMatrix);
    Output.Position = mul(Output.Position, ViewMatricesBuffer.ProjectionMatrix);
    
    Output.UV = Vertex.UV;
    
    // Can be removed and the normal can be retrieved from the TBN matrix in the PS
    const float3 Normal = normalize(mul(WorldMatrix, float4(Vertex.Normal, 0.f))).xyz;
    const float3 Tangent = normalize(mul(WorldMatrix, float4(Vertex.Tangent, 0.f))).xyz;
    const float3 Bitangent = normalize(cross(Tangent, Normal));
    
    Output.Normal = Normal;
    Output.TBN = float3x3(Tangent, Bitangent, Normal);
    
	return Output;
}