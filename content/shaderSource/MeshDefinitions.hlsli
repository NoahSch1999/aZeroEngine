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
#if !NORMAL_MAP
    float3x3 TBN : TBN;
#endif
};

struct MeshVertex
{
    float3 Position;
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

void GetVertex(out RasterVertex output, in float4x4 vpMatrix, in MeshVertex vertex, in float4x4 transform)
{
    float4 position = mul(transform, float4(vertex.Position, 1.f));
    output.WorldPosition = position.xyz;
    position = mul(vpMatrix, position);
    output.Position = position;
    
    const float3 normal = normalize(mul(transform, float4(DecodeNormalOctahedral(UnpackOct16(Unpack32To16(vertex.Normal))), 0.f))).xyz;
    //const float3 normal = normalize(mul(transform, float4(float3(vertex.Normal, 0), 0.f))).xyz;
    output.Normal = normal;
    
#if !NORMAL_MAP
    //float3 tangent = normalize(mul(transform, float4(vertex.Tangent, 0.f))).xyz;
    float3 tangent = float3(1, 0, 0);
    // Re-ortogonalize the tangent since they might not be ortogonal anymore after transform and precision changes. 
    // n * dot(n, t) creates a vector which when you subtract from the tangent creates the new ortogonalized tangent. So its like the "error" vector.
    tangent = tangent - normal * dot(normal, tangent);
    tangent = normalize(tangent);
    
    const float3 biTangent = normalize(cross(normal, tangent));
    output.TBN = float3x3(tangent, biTangent, normal);
#endif
    
    output.UV = UnpackUV16(Unpack32To16(vertex.UV));
}

#endif