#ifndef TEMP_DEFINITIONS_INCLUDED
#define TEMP_DEFINITIONS_INCLUDED
#include "Volumes.hlsli"
#include "Util.hlsli"

#define TREADS_PER_WAVE 32

struct Payload
{
    uint MeshletIndex[32];
};

// MESH ASSET STRUCTS

#define g_VerticesPerMeshlet 64
#define g_PrimitivesPerMeshlet 84

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
    uint PrimitiveCount;
    uint Primitives[g_PrimitivesPerMeshlet];
};

// GPU_Drive_Structs.hpp MIRROR

struct CameraData
{
    float4x4 ViewMatrix;
    float4x4 ViewProjectionMatrix;
    BoundingFrustum Frustum;
};

struct IndirectArgumentCounter
{
    uint Count;
};

struct InstanceData
{
    float4x4 Transform;
};

struct IndirectArgumentConstantData
{
    float4x4 Instance;
    uint GlobalMeshletOffset;
    uint GlobalVertexOffset;
    uint MaterialIndex;
    uint MeshletCount;
    uint3 Pad;
    // This will waste 3xuint...
    // Maybe move the material index into the pixel shader constant data struct?
};

struct ObjectCullData
{
    BoundingSphere Bounds;
    uint InstanceDataIndex;
    uint GlobalMeshletOffset;
    uint GlobalVertexOffset;
    uint MeshletCount;
    uint MaterialIndex;
};

struct MeshCullConstantsData
{
    uint MeshInstanceCount;
    uint3 Pad;
};

struct IndirectArguments
{
    IndirectArgumentConstantData Data;
    uint GroupX;
    uint GroupY;
    uint GroupZ;
};

struct PhongPixelConstantData
{
    uint SamplerIndex;
    uint MaterialBuffer;
    uint PointLightBuffer;
    uint SpotLightBuffer;
    uint DirectionalLightBuffer;
    float Time;
};

#endif