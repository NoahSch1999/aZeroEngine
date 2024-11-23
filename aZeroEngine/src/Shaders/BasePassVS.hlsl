cbuffer VertexShaderConstants : register(b0)
{
    row_major float4x4 ViewMatrix;
    row_major float4x4 ProjectionMatrix;
};

cbuffer PerDrawConstants : register(b1)
{
    int PrimitiveIndex;
};

struct PrimitiveRenderData
{
    row_major float4x4 WorldMatrix;
    int MaterialIndex;
    int MeshIndex;
};

struct VertexData
{
    float3 Position : POSITION;
    float2 UV : UV;
    float3 Normal : NORMAL;
    float3 Tangent : TANGENT;
};

struct OutputData
{
    float4 Position : SV_Position;
    float2 UV : UV;
    float3 Normal : NORMAL;
    float3x3 TBN : TBN;
};

StructuredBuffer<PrimitiveRenderData> PrimitiveRenderDataBuffer : register(t2);

// Expects column-major matrix by default
// Simplemath stores in row-major

OutputData main(VertexData Input)
{
    OutputData Output;
    
    const PrimitiveRenderData PrimitiveData = PrimitiveRenderDataBuffer.Load(PrimitiveIndex);
    
    const row_major float4x4 WorldMatrix = PrimitiveData.WorldMatrix;
    
    Output.Position = mul(float4(Input.Position, 1.f), WorldMatrix);
    Output.Position = mul(Output.Position, ViewMatrix);
    Output.Position = mul(Output.Position, ProjectionMatrix);
    
    Output.UV = Input.UV;
    
    // Can be removed and the normal can be retrieved from the TBN matrix in the PS
    const float3 Normal = normalize(mul(WorldMatrix, float4(Input.Normal, 0.f))).xyz;
    const float3 Tangent = normalize(mul(WorldMatrix, float4(Input.Tangent, 0.f))).xyz;
    const float3 Bitangent = normalize(cross(Tangent, Normal));
    
    Output.Normal = Normal;
    Output.TBN = float3x3(Tangent, Bitangent, Normal);
    
	return Output;
}