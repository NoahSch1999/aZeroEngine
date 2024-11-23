cbuffer PixelShaderConstants : register(b0)
{
    int SamplerIndex;
};

cbuffer PerDrawConstants : register(b1)
{
    int PrimitiveIndex;
};

struct PrimitiveRenderData
{
    float4x4 WorldMatrix;
    int MaterialIndex;
    int MeshIndex;
};

struct BasicMaterialRenderData
{
    int DiffuseTextureIndex;
    int NormalMapTextureIndex;
};

struct FragmentInput
{
    float4 Position : SV_Position;
    float2 UV : UV;
    float3 Normal : NORMAL;
    float3x3 TBN : TBN;
};

struct FragmentOutput
{
    float4 FragmentColor : SV_TARGET0;
};

StructuredBuffer<PrimitiveRenderData> PrimitiveRenderDataBuffer : register(t2);
StructuredBuffer<BasicMaterialRenderData> BasicMaterialRenderDataBuffer : register(t3);

FragmentOutput main(FragmentInput Input)
{
    const PrimitiveRenderData PrimitiveData = PrimitiveRenderDataBuffer.Load(PrimitiveIndex);
    const BasicMaterialRenderData BasicMaterialData = BasicMaterialRenderDataBuffer.Load(PrimitiveData.MaterialIndex);
    const SamplerState Sampler = SamplerDescriptorHeap[0];
    
    const Texture2D<float4> Texture = ResourceDescriptorHeap[/*BasicMaterialData.DiffuseTextureIndex*/1];
    const float3 Color = Texture.Sample(Sampler, Input.UV).xyz;
    FragmentOutput Output;
    Output.FragmentColor = float4(Color, 1.f);
    
    return Output;
}