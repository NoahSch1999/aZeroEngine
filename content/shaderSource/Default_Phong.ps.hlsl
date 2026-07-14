#include "GPU_Structs.hlsli"
#include "Materials.hlsli"

ConstantBuffer<IndirectArgumentConstantData> Input_CONSTANT : register(b0);

struct Output
{
    float4 color : SV_TARGET0;
};

StructuredBuffer<PBRMaterial> MaterialBuffer : register(t0);

Output main(RasterVertex pin)
{
    Output output;
    
    const SamplerState samplerState = SamplerDescriptorHeap[0];
    PBRMaterial material = MaterialBuffer[Input_CONSTANT.MaterialIndex];
    
    if (material.AlbedoTexture == 0xffffffff)
    {
        output.color = float4(1, 0, 1, 1);
        return output;
    }
    
    const Texture2D<float4> albedoTexture = ResourceDescriptorHeap[material.AlbedoTexture];
    float3 sampled = albedoTexture.Sample(samplerState, pin.UV).xyz;
    
    float3 fragmentNormal;
    if (material.NormalMap == 0xffffffff)
    {
        fragmentNormal = normalize(pin.Normal);
    }
    else
    {
        const Texture2D<float4> normalMap = ResourceDescriptorHeap[material.NormalMap];
        const float3 sampledNormal = normalMap.Sample(samplerState, pin.UV).xyz;
        float3 tangent;
        float3 bitangent;
        float3 normal = normalize(pin.Normal);
        CalcTangentAndBitangent(normal, tangent, bitangent);
        float3x3 TBN = float3x3(tangent, bitangent, normal);
        fragmentNormal = normalize(sampledNormal * 2.f - 1.f);
        fragmentNormal = normalize(mul(fragmentNormal, TBN));
    }
    
    //float idk = 0.f;
    //for (int i = 0; i < (int) pin.UV.x; i++)
    //{
    //    idk += sin(pin.Position.x);
    //}
    
    //float2 uv = pin.UV;

    //float2 dx = ddx(uv);
    //float2 dy = ddy(uv);

    //float texelSize = max(length(dx), length(dy));

    //float lod = log2(texelSize * 2048); // TextureWidth = texture width in pixels

    //output.color = float4(frac(lod / 10.0).xxx, 1.0);
    
    //output.color = float4(pin.UV, 0, 1);
    output.color = float4(sampled, 1);
    //output.color = float4(fragmentNormal * 0.5 + 0.5, 1);
    //output.color = float4(pin.Meshletid, 1);
    
    return output;
}