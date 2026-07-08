#include "GPU_Structs.hlsli"
#include "Materials.hlsli"

ConstantBuffer<IndirectArgumentConstantData> Input_CONSTANT : register(b0);

struct Output
{
    float4 color : SV_TARGET0;
};

StructuredBuffer<DefaultMaterial> MaterialBuffer : register(t0);

Output main(RasterVertex pin)
{
    Output output;
    const SamplerState samplerState = SamplerDescriptorHeap[0];
    DefaultMaterial material = MaterialBuffer[Input_CONSTANT.MaterialIndex];
    float3 fragmentNormal;
    if (material.NormalMap == 0xffffffff)
    {
        fragmentNormal = normalize(pin.Normal);
    }
    else
    {
        float3 tangent;
        float3 bitangent;
        float3 normal = normalize(pin.Normal);
        CalcTangentAndBitangent(normal, tangent, bitangent);
        float3x3 TBN = float3x3(tangent, bitangent, normal);
        const Texture2D<float4> normalMap = ResourceDescriptorHeap[material.NormalMap];
        float3 fragmentNormal = normalMap.Sample(samplerState, pin.UV).xyz;
        fragmentNormal = normalize(fragmentNormal * 2.f - 1.f);
        fragmentNormal = normalize(mul(fragmentNormal, TBN));
    }

    if (material.AlbedoTexture == 0xffffffff)
    {
        output.color = float4(1, 0, 1, 1);
        return output;
    }
    
    const Texture2D<float4> albedoTexture = ResourceDescriptorHeap[material.AlbedoTexture];
    //output.color = float4(pin.UV, 0, 1);
    output.color = float4(albedoTexture.Sample(samplerState, pin.UV).xyz, 1);
    //output.color = float4(fragmentNormal.xyz, 1);
    
    return output;
}