#include "VertexDefinitions.hlsli"
#include "Materials.hlsli"
#include "Lights.hlsli"

struct PixelShaderConstantsData
{
    uint SamplerIndex;
    uint MaterialBuffer;
    uint PointLightBuffer;
    uint SpotLightBuffer;
    uint DirectionalLightBuffer;
    float Time;
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
    
    float3 surfaceColor = float3(1, 0, 1);
    
    if (material.AlbedoTexture != 65535)
    {
        const Texture2D<float4> albedoTexture = ResourceDescriptorHeap[material.AlbedoTexture];
        surfaceColor = albedoTexture.Sample(samplerState, pin.UV).xyz;
    }
    
    float3 fragmentNormal;
#if !NORMAL_MAP
    if (material.NormalMap != 65535)
    {
        const Texture2D<float4> normalMap = ResourceDescriptorHeap[material.NormalMap];
        fragmentNormal = normalMap.Sample(samplerState, pin.UV).xyz;
        fragmentNormal = normalize(fragmentNormal * 2.f - 1.f);
        fragmentNormal = normalize(mul(fragmentNormal, pin.TBN));
    }
    else
    {
        fragmentNormal = normalize(pin.Normal);
    }
#else
    fragmentNormal = normalize(pin.Normal);
#endif
    
    // TODO: Calc lighting
    surfaceColor = float3(0, 0, 0);
    float3 ambient = float3(0.4, 0.4, 0.4);
    surfaceColor *= ambient;
    
    PointLight p;
    p.Color = float3(1, 0, 0);
    p.Position = float3(sin(PixelShaderConstants.Time * 8), 3, 0);
    p.Intensity = 1.f;
    surfaceColor += p.CalculateLighting_BlinnPhong(pin.WorldPosition.xyz, fragmentNormal, float3(0, 0, 0));
    
    Output output;
    output.color = float4(surfaceColor, 1); // normal
    //output.color = float4(pin.MeshletColor, 1.f);
    
    return output;
}