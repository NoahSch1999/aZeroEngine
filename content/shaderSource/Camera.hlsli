#ifndef CAMERA_INCLUDED
#define CAMERA_INCLUDED

#include "Volumes.hlsli"

struct Camera
{
    float4x4 ViewMatrix;
    float4x4 ViewProjectionMatrix;
    BoundingFrustum Frustum;
};

#endif