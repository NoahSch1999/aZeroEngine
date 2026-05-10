#ifndef SCENE_RENDERDATA_INCLUDED
#define SCENE_RENDERDATA_INCLUDED

#include "MeshDefinitions.hlsli"

struct GPUDrivenRenderConstants
{
    float4x4 CameraView; // Camera view matrix
    BoundingFrustum CameraFrustum; // Camera frustum
    uint MeshInstancesCount; // Num meshinstances to perform frustum-culling with
};

struct MeshCull_Count
{
    uint Count;
};

struct MeshletDrawConstantsData
{
    float4x4 WorldTransform;
    uint MeshletBuffer_Bindless;
    uint MeshletCount;
    uint MaterialIndex;
};

struct MeshletCull_IA
{
    MeshletDrawConstantsData MeshInstance;
    uint GroupsX; // Doesn't need to be reset since it's overwritten fully each time it's used
    uint GroupsY; // Always 1
    uint GroupsZ; // Always 1
};

struct MeshletPayload
{
    uint VertexOffset[THREADS_PER_X];
    uint TriangleCount[THREADS_PER_X];
};

void GetVertex(out PipelineVertex output, uint vertexIndex, in float4x4 vpMatrix, in StructuredBuffer<MeshVertex> vertices, in float4x4 transform, uint materialIndex)
{
    float4 position = mul(transform, float4(vertices[vertexIndex].Position, 1.f));
    output.WorldPosition = position.xyz;
    position = mul(vpMatrix, position);
    output.Position = position;
    
    const float3 normal = normalize(mul(transform, float4(vertices[vertexIndex].Normal, 0.f))).xyz;
    output.Normal = normal;
    
#if !NORMAL_MAP
    float3 tangent = normalize(mul(transform, float4(vertices[vertexIndex].Tangent, 0.f))).xyz;
    
    // Re-ortogonalize the tangent since they might not be ortogonal anymore after transform and precision changes. 
    // n * dot(n, t) creates a vector which when you subtract from the tangent creates the new ortogonalized tangent. So its like the "error" vector.
    tangent = tangent - normal * dot(normal, tangent);
    tangent = normalize(tangent);
    
    const float3 biTangent = normalize(cross(normal, tangent));
    output.TBN = float3x3(tangent, biTangent, normal);
#endif
    
    output.UV = vertices[vertexIndex].UV;
    output.MaterialIndex = materialIndex;
}

#endif