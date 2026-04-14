#include "MeshletCommon.hlsli"
#include "Materials.hlsli"

struct PixelShaderConstantsData
{
    uint SamplerIndex;
    uint MaterialBuffer;
};

ConstantBuffer<PixelShaderConstantsData> PixelShaderConstants : register(b0);

struct Output
{
    float4 color : SV_TARGET0;
};

Output main(VertexOut pin)
{
    const SamplerState samplerState = SamplerDescriptorHeap[PixelShaderConstants.SamplerIndex];
    const StructuredBuffer<DefaultMaterial> MaterialBuffer = ResourceDescriptorHeap[PixelShaderConstants.MaterialBuffer];
    
    const DefaultMaterial material = MaterialBuffer[pin.MaterialID];
    
    const Texture2D<float4> albedoTexture = ResourceDescriptorHeap[material.AlbedoTexture];
    const float3 surfaceColor = albedoTexture.Sample(samplerState, pin.UV).xyz;
    
    float3 fragmentNormal;
#if !NORMAL_MAP
    const Texture2D<float4> normalMap = ResourceDescriptorHeap[material.NormalMap];
    fragmentNormal = normalMap.Sample(samplerState, pin.UV).xyz;
    fragmentNormal = normalize(fragmentNormal * 2.f - 1.f);
    fragmentNormal = normalize(mul(fragmentNormal, pin.TBN));
#else
    fragmentNormal = normalize(pin.Normal);
#endif
    
    // TODO: Calc lighting
    
    Output output;
    output.color = float4(surfaceColor, 1.f);
    return output;
}