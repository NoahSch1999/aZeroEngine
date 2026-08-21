#ifndef LIGHTS_INCLUDED
#define LIGHTS_INCLUDED

#include "Volumes.hlsli"
#include "Materials.hlsli"

float CalcAttenuation(float d, float falloffStart, float falloffEnd)
{
    // Linear falloff.
    return saturate((falloffEnd - d) / (falloffEnd - falloffStart));
}

float3 SchlickFresnel(float3 R0, float3 normal, float3 lightVec)
{
    float cosIncidentAngle = saturate(dot(normal, lightVec));

    float f0 = 1.0f - cosIncidentAngle;
    float3 reflectPercent = R0 + (1.0f - R0) * (f0 * f0 * f0 * f0 * f0);

    return reflectPercent;
}

float3 BlinnPhong(float3 lightStrength, float3 lightVec, float3 normal, float3 toEye, float shininess, float fresnel, float3 diffuseColor)
{
    //const float m = shininess * 256.0f;
    //float3 halfVec = normalize(toEye + lightVec);

    //float roughnessFactor = (m + 8.0f) * pow(max(dot(halfVec, normal), 0.0f), m) / 8.0f;
    //float3 fresnelFactor = SchlickFresnel(fresnel, halfVec, lightVec);

    //float3 specAlbedo = fresnelFactor * roughnessFactor;

    //// Our spec formula goes outside [0,1] range, but we are 
    //// doing LDR rendering.  So scale it down a bit.
    //specAlbedo = specAlbedo / (specAlbedo + 1.0f);

    //return (diffuseColor + specAlbedo) * lightStrength;

    return diffuseColor * lightStrength;
}

struct PointLight
{
    float3 Position;
    float3 Color;
    float Intensity;
    float FalloffStart;
    float FalloffEnd;
    
    BoundingSphere CreateBounds()
    {
        return CreateBoundingSphere(Position, 10000.f);
    }
    
    float3 CalculateLighting_BlinnPhong(float3 surfacePosition, float3 surfaceNormal, float3 cameraDirection, float3 toEye, float shininess, float fresnel, float3 diffuseColor)
    {
         // The vector from the surface to the light.
        float3 lightVec = Position - surfacePosition;

        // The distance from surface to light.
        float d = length(lightVec);

        // Range test.
        if (d > FalloffEnd)
        {
            return 0.0f;
        }

        // Normalize the light vector.
        lightVec /= d;

        // Scale light down by Lambert's cosine law.
        float ndotl = max(dot(lightVec, surfaceNormal), 0.0f);
        float3 lightStrength = Color * ndotl * Intensity;

        // Attenuate light by distance.
        float att = CalcAttenuation(d, FalloffStart, FalloffEnd);
        lightStrength *= att;

        return BlinnPhong(lightStrength, lightVec, surfaceNormal, toEye, shininess, fresnel, diffuseColor);
    }
};

struct SpotLight
{
    float3 Position;
    float3 Direction;
    float3 Color;
    float Intensity;
    float FalloffStart;
    float FalloffEnd;
    
    float SpotPower;
    
    BoundingSphere CreateBounds()
    {
        return CreateBoundingSphere(Position, 1000.f);
    }
    
    float3 CalculateLighting_BlinnPhong(float3 surfacePosition, float3 surfaceNormal, float3 cameraDirection, float3 toEye, float shininess, float fresnel, float3 diffuseColor)
    {
        // The vector from the surface to the light.
        float3 lightVec = Position - surfacePosition;

        // The distance from surface to light.
        float d = length(lightVec);

        // Range test.
        if (d > FalloffEnd)
            return 0.0f;

        // Normalize the light vector.
        lightVec /= d;

        // Scale light down by Lambert's cosine law.
        float ndotl = max(dot(lightVec, surfaceNormal), 0.0f);
        float3 lightStrength = Color * ndotl * Intensity;

        // Attenuate light by distance.
        float att = CalcAttenuation(d, FalloffStart, FalloffEnd);
        lightStrength *= att;

        // Scale by spotlight
        float spotFactor = pow(max(dot(-lightVec, Direction), 0.0f), SpotPower);
        lightStrength *= spotFactor;
        
        return BlinnPhong(lightStrength, lightVec, surfaceNormal, toEye, shininess, fresnel, diffuseColor);
    }
};

struct DirectionalLight
{
    float3 Direction;
    float3 Color;
    float Intensity;
    
    float3 CalculateLighting_BlinnPhong(float3 surfaceNormal, float3 cameraDirection, float3 toEye, float shininess, float fresnel, float3 diffuseColor)
    {
        // The light vector aims opposite the direction the light rays travel.
        float3 lightVec = -Direction;

        // Scale light down by Lambert's cosine law.
        float ndotl = max(dot(lightVec, surfaceNormal), 0.0f);
        float3 lightStrength = Color * ndotl * Intensity;
        
        return BlinnPhong(lightStrength, lightVec, surfaceNormal, toEye, shininess, fresnel, diffuseColor);
    }
};
#endif