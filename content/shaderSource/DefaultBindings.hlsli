#include "Volumes.hlsli"

struct CameraData
{
    float4x4 View;
    float4x4 Projection;
    Frustum BoundingFrustum;
};