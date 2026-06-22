#ifndef LIGHTS_INCLUDED
#define LIGHTS_INCLUDED

#include "Volumes.hlsli"

struct PointLight
{
    float3 Position;
    float3 Color;
    float Intensity;
    float Attenuation;
    float BoundsRadius;
    
    BoundingSphere CreateBounds()
    {
        return CreateBoundingSphere(Position, BoundsRadius);
    }
    
    float3 CalculateLighting_BlinnPhong(float3 surfacePosition, float3 surfaceNormal, float3 cameraDirection)
    {
        float3 surfaceToLight = Position - surfacePosition;
        float3 color = Color * Intensity * dot(normalize(surfaceToLight), surfaceNormal);
        return color; // TODO impl
    }
};

struct SpotLight
{
    float3 Position;
    float3 Direction;
    float3 Color;
    float Intensity;
    float BoundsRadius;
    float ConeAngle;
    
    BoundingSphere CreateBounds()
    {
        return CreateBoundingSphere(Position, BoundsRadius);
    }
    
    float3 CalculateLighting_BlinnPhong(float3 surfacePosition, float3 surfaceNormal, float3 cameraDirection)
    {
        return Color; // TODO impl
    }
};

struct DirectionalLight
{
    float3 Direction;
    float3 Color;
    float Intensity;
    
    float3 CalculateLighting_BlinnPhong(float3 surfaceNormal, float3 cameraDirection)
    {
        return Color; // TODO impl
    }
};
#endif