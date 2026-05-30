#ifndef MESH_DEFINITIONS_RENDERDATA_INCLUDED
#define MESH_DEFINITIONS_RENDERDATA_INCLUDED

#include "Volumes.hlsli"
#include "Util.hlsli"

struct RasterVertex
{
    float4 Position : SV_Position;
    float3 WorldPosition : WORLD_POSITION;
    float2 UV : UV;
    float3 Normal : NORMAL;
};

struct MeshVertex
{
    uint UV;
    uint Normal;
};

struct Meshlet
{
    uint VertexOffset;
    uint VertexCount;
    uint PrimitiveOffset;
    uint PrimitiveCount;
    BoundingSphere Bounds;
};

struct MeshInstance
{
    float4x4 WorldTransform;
    uint MeshBufferIndex;
    uint MeshletOffset;
    uint MeshletCount;
    uint MaterialIndex;
};

struct MeshInstanceData
{
    MeshInstance Instance;
    BoundingSphere MeshBounds;
};

float2 UnpackOct16(uint2 p)
{
    float2 n;
    n.x = (p.x / 65535.0) * 2.0 - 1.0;
    n.y = (p.y / 65535.0) * 2.0 - 1.0;
    return n;
}

float2 UnpackUV16(uint2 p)
{
    return p / 65535.0;
}

#endif