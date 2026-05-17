#include "Materials.hlsli"
#include "Lights.hlsli"
#include "MeshDefinitions.hlsli"

struct Input_IA
{
    MeshInstance Instance; // Since we use vis-all it should be almost no extra cost than using a smaller root constant
};
ConstantBuffer<Input_IA> Input_CONSTANT : register(b0);

struct PassConstantData
{
    uint SamplerIndex;
    uint MaterialBuffer;
    uint PointLightBuffer;
    uint SpotLightBuffer;
    uint DirectionalLightBuffer;
    float Time;
};

ConstantBuffer<PassConstantData> Default_Phong_CONSTANT : register(b1);

struct Output
{
    float4 color : SV_TARGET0;
};

Output main(PipelineVertex pin)
{
    Output output;
    output.color = float4(pin.Normal.xy, Input_CONSTANT.Instance.MaterialIndex, 1);
//output.color = float4(surfaceColor, 1); // normal
//output.color = float4(pin.MeshletColor, 1.f);

    return output;
//    const SamplerState samplerState = SamplerDescriptorHeap[Default_Phong_Constants.SamplerIndex];
//    const StructuredBuffer<DefaultMaterial> MaterialBuffer = ResourceDescriptorHeap[Default_Phong_Constants.MaterialBuffer];
    
//    const DefaultMaterial material = MaterialBuffer[pin.MaterialIndex];
    
//    float3 surfaceColor = float3(1, 0, 1);
    
//    if (material.AlbedoTexture != 65535)
//    {
//        const Texture2D<float4> albedoTexture = ResourceDescriptorHeap[material.AlbedoTexture];
//        surfaceColor = albedoTexture.Sample(samplerState, pin.UV).xyz;
//    }
    
//    float3 fragmentNormal;
//#if !NORMAL_MAP
//    if (material.NormalMap != 65535)
//    {
//        const Texture2D<float4> normalMap = ResourceDescriptorHeap[material.NormalMap];
//        fragmentNormal = normalMap.Sample(samplerState, pin.UV).xyz;
//        fragmentNormal = normalize(fragmentNormal * 2.f - 1.f);
//        fragmentNormal = normalize(mul(fragmentNormal, pin.TBN));
//    }
//    else
//    {
//        fragmentNormal = normalize(pin.Normal);
//    }
//#else
//    fragmentNormal = normalize(pin.Normal);
//#endif
    
//    // TODO: Calc lighting
//    //surfaceColor = float3(0, 0, 0);
//    //float3 ambient = float3(0.4, 0.4, 0.4);
//    //surfaceColor *= ambient;
    
//    //PointLight p;
//    //p.Color = float3(1, 0, 0);
//    //p.Position = float3(sin(Default_Phong_Constants.Time * 8), 3, 0);
//    //p.Intensity = 1.f;
//    //surfaceColor += p.CalculateLighting_BlinnPhong(pin.WorldPosition.xyz, fragmentNormal, float3(0, 0, 0));
    
//    Output output;
//    output.color = float4(fragmentNormal.xyz, 1);
//    //output.color = float4(surfaceColor, 1); // normal
//    //output.color = float4(pin.MeshletColor, 1.f);
    
//    return output;
}