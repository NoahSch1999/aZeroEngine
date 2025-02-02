struct FragmentInput
{
    float4 Position : SV_Position;
    float3 WorldPosition : WorldPosition;
    float2 UV : UV;
    float3 Normal : NORMAL;
    float3 Tangent : TANGENT;
    float3 BiTangent : BITANGENT;
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
    unsigned int NumDirectionalLights;
    unsigned int NumPointLights;
    unsigned int NumSpotLights;
};

struct Material
{
    float3 Color;
    int AlbedoIndex;
    int NormalMapIndex;
};

float Sqr(float x)
{
    return x * x;
}

struct LightBase
{
    float3 Color;
    float Intensity;
};

struct DirectionalLight : LightBase
{
    float3 Direction;
};

struct PointLight : LightBase
{
    float3 Position;
    float FalloffFactor;
};

struct SpotLight : LightBase
{
    float3 Position;
    float3 Direction;
    float Range;
    
    // Cos of angle that is valid between light vector to point and direction
    // 1 is smaller cone, 0 is 90 deg cone
    float CutoffAngle;
};

float3 CalcPointLight(PointLight Light, float3 Position, float3 Normal)
{
    float3 PositionToLight = Light.Position - Position;
    float Distance = length(PositionToLight);
    PositionToLight = normalize(PositionToLight);
    float AngleCos = dot(Normal, PositionToLight);
    if (AngleCos <= 0)
    {
        return float3(0, 0, 0);
    }
    
    float Attenuation = 1.f / (1.f + Sqr(Distance / Light.FalloffFactor));
    float3 Color = (Light.Color * Light.Intensity * AngleCos) * Attenuation;
    return Color;
}

float3 CalcSpotLight(SpotLight Light, float3 Position, float3 Normal)
{
    float3 PositionToLight = Light.Position - Position;
    float Distance = length(PositionToLight);
    PositionToLight = normalize(PositionToLight);
    float AngleCos = dot(Normal, PositionToLight);
    if (AngleCos <= 0)
    {
        return float3(0, 0, 0);
    }
    
    float SpotLightAngle = dot(-PositionToLight, Light.Direction);
    if (SpotLightAngle <= Light.CutoffAngle)
    {
        return float3(0, 0, 0);
    }
    float Intensity = 1.f - (1.f - SpotLightAngle) / (1.f - Light.CutoffAngle);
    float Attenuation = 1.f / (1.f + Sqr(Distance / Light.Range));
    float3 Color = Light.Color * AngleCos * Intensity * Light.Intensity * Attenuation;
    return Color;
}

float3 CalcDirectionalLight(DirectionalLight Light, float3 Normal)
{
    float AngleCos = dot(Normal, -Light.Direction);
    
    if (AngleCos <= 0)
    {
        return float3(0, 0, 0);
    }
    return Light.Color * Light.Intensity * AngleCos;
}

ConstantBuffer<PixelShaderConstants> PixelShaderConstantsBuffer : register(b0);
ConstantBuffer<PerBatchConstants> PerBatchConstantsBuffer : register(b1);

StructuredBuffer<Material> MaterialBuffer : register(t1);

StructuredBuffer<DirectionalLight> DirectionalLightBuffer : register(t2);
StructuredBuffer<PointLight> PointLightBuffer : register(t3);
StructuredBuffer<SpotLight> SpotLightBuffer : register(t4);

FragmentOutput main(FragmentInput Input)
{
    FragmentOutput Output;
    
    const Material Mat = MaterialBuffer.Load(PerBatchConstantsBuffer.MaterialEntryIndex);
    
    const SamplerState Sampler = SamplerDescriptorHeap[PixelShaderConstantsBuffer.SamplerIndex];
    
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
        //Normal = Normal * 0.5 + 0.5;
        float3x3 TBN = float3x3(Input.Tangent, Input.BiTangent, Input.Normal);
        Normal = normalize(mul(TBN, Normal));
        //Normal = normalize(mul(Input.TBN, Normal));
        //Normal = normalize(mul(Normal, Input.TBN));
    }
    else
    {
        Normal = normalize(Input.Normal);
    }
       
    float3 LightFactor = float3(0, 0, 0);
    
    for (int DLightIndex = 0; DLightIndex < PerBatchConstantsBuffer.NumDirectionalLights; DLightIndex++)
    {
        LightFactor += CalcDirectionalLight(DirectionalLightBuffer.Load(DLightIndex), Normal);
    }
    
    for (int PLightIndex = 0; PLightIndex < PerBatchConstantsBuffer.NumPointLights; PLightIndex++)
    {
        LightFactor += CalcPointLight(PointLightBuffer.Load(PLightIndex), Input.WorldPosition, Normal);
    }
    
    for (int SLightIndex = 0; SLightIndex < PerBatchConstantsBuffer.NumSpotLights; SLightIndex++)
    {
        LightFactor += CalcSpotLight(SpotLightBuffer.Load(SLightIndex), Input.WorldPosition, Normal);
    }
    
    float3 Ambient = float3(0.1, 0.1, 0.1);
    Color *= Ambient + LightFactor; 
    Output.FragmentColor = float4(Color, 1.f);
    
    return Output;
}