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
    float3 surfaceColor = albedoTexture.Sample(samplerState, pin.UV).xyz;
    
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
    float3 dir = float3(0, -3, 1);
    dir = normalize(dir);
    
    float intensity = dot(-dir, fragmentNormal) * 3;
    float3 lightColor = float3(1, 1, 1) * intensity;
    
    float3 ambient = float3(0.1, 0.1, 0.1);
    surfaceColor += ambient * lightColor;
    
    Output output;
    output.color = float4(surfaceColor, 1.f);
    //output.color = float4(pin.MeshletColor, 1.f);
    return output;
}