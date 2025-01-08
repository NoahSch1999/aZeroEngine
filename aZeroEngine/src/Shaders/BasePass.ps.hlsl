struct FragmentInput
{
    float4 Position : SV_Position;
    float3 WorldPosition : WorldPosition;
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

struct PerBatchConstants
{
    unsigned int MaterialEntryIndex;
};

struct Material
{
    float3 Color;
    int AlbedoIndex;
    int NormalMapIndex;
};

ConstantBuffer<PixelShaderConstants> PixelShaderConstantsBuffer : register(b0);
ConstantBuffer<PerBatchConstants> PerBatchConstantsBuffer : register(b1);
StructuredBuffer<Material> MaterialBuffer : register(t1);

FragmentOutput main(FragmentInput Input)
{
    FragmentOutput Output;
    
    const Material Mat = MaterialBuffer.Load(PerBatchConstantsBuffer.MaterialEntryIndex);
    
    const SamplerState Sampler = SamplerDescriptorHeap[0 /*PixelShaderConstantsBuffer.SamplerIndex*/];
    
    float3 Color = float3(1, 0, 0);
    Texture2D<float4> AlbedoTexture;
    if (Mat.AlbedoIndex != -1)
    {
        AlbedoTexture = ResourceDescriptorHeap[Mat.AlbedoIndex];
        Color = AlbedoTexture.Sample(Sampler, Input.UV).xyz;
    }
    else
    {
        Color = Mat.Color;
    }
    
    float3 Normal;
    if (Mat.NormalMapIndex != -1)
    {
        Texture2D<float4> NormalMapTexture;
        NormalMapTexture = ResourceDescriptorHeap[Mat.NormalMapIndex];
        Normal = NormalMapTexture.Sample(Sampler, Input.UV).xyz;
        Normal = Normal * 2.f - 1.f;
        Normal = normalize(mul(Input.TBN, Normal));
    }
    else
    {
        Normal = normalize(Input.Normal);
    }
    
    float3 LightPos = float3(0, 0, -5);
    float3 LightColor = float3(1, 1, 1);
    float3 LightToPosDir = normalize(LightPos - Input.WorldPosition);
    
    float Delta = dot(Normal, LightToPosDir);
    Color *= Delta;
    Output.FragmentColor = float4(Color, 1.f);
    
    return Output;
}