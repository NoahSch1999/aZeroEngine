#ifndef MESH_DEFS_INCLUDED
#define MESH_DEFS_INCLUDED

#include "Volumes.hlsli"

struct PipelineVertex
{
    float4 Position : SV_Position;
    float3 WorldPosition : WORLD_POSITION;
    float2 UV : UV;
    float3 Normal : NORMAL;
#if !NORMAL_MAP
    float3x3 TBN : TBN;
#endif
    uint MaterialIndex : MATERIAL;
};

struct MeshVertex
{
    float3 Position;
    float2 UV;
    float3 Normal;
    float3 Tangent;
};

struct MeshInstance
{
    float4x4 WorldTransform;
    uint MeshletBuffer_Bindless;
    uint MeshletCount;
    uint MaterialIndex;
    BoundingSphere MeshBounds;
};

struct Meshlet
{
    uint VertexOffset;
    //uint VertexCount;
    uint TriangleCount;
    BoundingSphere Bounds;
};

#define THREADS_PER_X 64
#define THREADS_PER_MESHLET 64

#endif