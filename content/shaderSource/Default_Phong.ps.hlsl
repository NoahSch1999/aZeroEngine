#include "GPU_Structs.hlsli"
#include "Materials.hlsli"

ConstantBuffer<IndirectArgumentConstantData> Input_CONSTANT : register(b0);
ConstantBuffer<RenderMode> RenderMode_CONSTANT : register(b2);

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
    float3 sampledAlbedo = albedoTexture.Sample(samplerState, pin.UV).xyz;
    
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
    
    if (RenderMode_CONSTANT.Mode == RENDER_MODE_LIT)
    {
        // todo Normal pass calculations
        output.color = float4(sampledAlbedo, 1.f);
        return output;
    }
    else
    {
    
        // -------------------------------   DEBUG ALBEDO   -------------------------------------------
        if (RenderMode_CONSTANT.Mode == RENDER_MODE_UNLIT)
        {
            output.color = float4(sampledAlbedo, 1);
            return output;
        }
        // --------------------------------------------------------------------------------------------
        
        if (material.NormalMap != 0xffffffff)
        {
            // -------------------------------   DEBUG NORMAL MAP   -----------------------------------
            if (RenderMode_CONSTANT.Mode == RENDER_MODE_NORMAL_MAP)
            {
                const Texture2D<float4> normalMap = ResourceDescriptorHeap[material.NormalMap];
                const float3 sampledNormal = normalMap.Sample(samplerState, pin.UV).xyz;
                output.color = float4(sampledNormal * 0.5 + 0.5, 1);
                return output;
            }
            // ----------------------------------------------------------------------------------------
        
            // -------------------------------   DEBUG NORMALS WITH MAPS   ----------------------------
            if (RenderMode_CONSTANT.Mode == RENDER_MODE_NORMAL_MAP_NORMALS)
            {
                output.color = float4(fragmentNormal * 0.5 + 0.5, 1);
                return output;
            }
            // ----------------------------------------------------------------------------------------
        }
    
        // -------------------------------   DEBUG UV   -----------------------------------------------
        if (RenderMode_CONSTANT.Mode == RENDER_MODE_UVS)
        {
            output.color = float4(pin.UV, 0, 1);
            return output;
        }
        // --------------------------------------------------------------------------------------------
    
        // -------------------------------   DEBUG NORMALS  -------------------------------------------
        if (RenderMode_CONSTANT.Mode == RENDER_MODE_GEOMETRY_NORMALS)
        {
            output.color = float4(pin.Normal * 0.5 + 0.5, 1);
            return output;
        }
        // --------------------------------------------------------------------------------------------
    
        // -------------------------------   DEBUG MESHLETS   -----------------------------------------
        if (RenderMode_CONSTANT.Mode == RENDER_MODE_MESHLET_IDS)
        {
            output.color = float4(pin.Meshletid, 1);
            return output;
        }
        // --------------------------------------------------------------------------------------------
    
        // -------------------------------   DEBUG MIP LEVEL   ----------------------------------------
        if (RenderMode_CONSTANT.Mode == RENDER_MODE_MIP_LEVEL)
        {
            float lod = albedoTexture.CalculateLevelOfDetail(samplerState, pin.UV);
            float t = saturate(lod / 13.f);
            output.color = float4(1.f - t, 1.f - t, 1.f - t, 1.f);
            return output;
        }
        // --------------------------------------------------------------------------------------------
    }
    
    return output;
}