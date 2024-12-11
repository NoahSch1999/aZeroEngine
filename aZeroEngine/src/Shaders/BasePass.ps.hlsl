struct PrimitiveRenderData
{
    float4x4 WorldMatrix;
    unsigned int MeshEntryIndex;
    unsigned int MaterialIndex;
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

struct PixelShaderConstants
{
    int SamplerIndex;
};

struct PerDrawConstants
{
    int PrimitiveIndex;
};

struct Material
{
    float3 Color;
    uint AlbedoIndex;
    uint NormalMapIndex;
};

ConstantBuffer<PixelShaderConstants> PixelShaderConstantsBuffer : register(b0);
ConstantBuffer<PerDrawConstants> PerDrawConstantsBuffer : register(b1);
StructuredBuffer<PrimitiveRenderData> PrimitiveRenderDataBuffer : register(t0);
StructuredBuffer<Material> MaterialBuffer : register(t1);

FragmentOutput main(FragmentInput Input)
{
    FragmentOutput Output;
    
    const PrimitiveRenderData PrimData = PrimitiveRenderDataBuffer.Load(PerDrawConstantsBuffer.PrimitiveIndex);
    const Material Mat = MaterialBuffer.Load(PrimData.MaterialIndex);
    
    const SamplerState Sampler = SamplerDescriptorHeap[0/*PixelShaderConstantsBuffer.SamplerIndex*/];
    
    Texture2D<float4> Texture;
    if(/*Mat.AlbedoIndex != 0*/false)
    {
        Texture = ResourceDescriptorHeap[Mat.AlbedoIndex];
        Output.FragmentColor = float4(Texture.Sample(Sampler, Input.UV).xyz, 1.f);
    }
    else
    {
        Output.FragmentColor = float4(Mat.Color, 1.f);
    }
    
    return Output;
}