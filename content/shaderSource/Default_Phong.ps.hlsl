#include "GPU_Structs.hlsli"

ConstantBuffer<IndirectArgumentConstantData> Default_Phong_CONSTANT : register(b0);

struct Output
{
    float4 color : SV_TARGET0;
};

Output main(RasterVertex pin)
{
    Output output;
    const SamplerState samplerState = SamplerDescriptorHeap[0];
    
    float3 tangent;
    float3 bitangent;
    CalcTangentAndBitangent(pin.Normal, tangent, bitangent);
    float3x3 TBN = float3x3(tangent, bitangent, pin.Normal);
    
    const Texture2D<float4> normalMap = ResourceDescriptorHeap[2];
    float3 fragmentNormal = normalMap.Sample(samplerState, pin.UV).xyz;
    fragmentNormal = normalize(fragmentNormal * 2.f - 1.f);
    fragmentNormal = normalize(mul(fragmentNormal, TBN));
    
    const Texture2D<float4> albedoTexture = ResourceDescriptorHeap[1];
    output.color = float4(albedoTexture.Sample(samplerState, pin.UV).xyz, 1);
    output.color = float4(fragmentNormal.xyz, 1);
    
    return output;
}