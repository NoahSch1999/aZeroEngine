#include "SceneRenderCommon.hlsli"

struct ConstantData
{
    float4x4 CameraVP;
};

//ConstantBuffer<MeshletDrawConstantsData> MeshletDrawConstants : register(b0); // Passed from MeshCull compute shader pass
ConstantBuffer<ConstantData> ConstantsMS : register(b1);

[NumThreads(THREADS_PER_MESHLET, 1, 1)]
[OutputTopology("triangle")]
void main(
    uint vertexIndex : SV_GroupThreadID, // One per vertex in the meshlet
    uint meshletIndex : SV_GroupID,                    // One per meshlet
    in payload MeshletPayload payload,
    out vertices PipelineVertex verts[63],
    out indices uint3 tris[21]
)
{
    if (payload.VisibilityCount > meshletIndex)
    {
        MeshletDrawConstantsData d;
        d.WorldTransform = payload.Constants.WorldTransform;
        d.MaterialIndex = payload.Constants.MaterialIndex;
        d.MeshletCount = payload.Constants.MeshletCount;
        d.MeshBuffer_Bindless = payload.Constants.MeshBuffer_Bindless;
        uint triangleCount = payload.TriangleCount[meshletIndex];
    
        SetMeshOutputCounts(triangleCount * 3, triangleCount);
    
        if (vertexIndex < triangleCount)
        {
            uint index = vertexIndex * 3;
            tris[vertexIndex] = uint3(index, index + 1, index + 2);
        }
    
        if (vertexIndex < triangleCount * 3)
        {
            PipelineVertex newVertex;
            const StructuredBuffer<MeshVertex> vertexPositionBuffer = ResourceDescriptorHeap[payload.Constants.MeshBuffer_Bindless + 1]; // The descriptors for the meshlet and vertex buffers are fetched n and n+1
            GetVertex(newVertex, payload.VertexOffset[meshletIndex] + vertexIndex, ConstantsMS.CameraVP, vertexPositionBuffer, payload.Constants.WorldTransform, payload.Constants.MaterialIndex);
            verts[vertexIndex] = newVertex;
        }
    }
}