#include "SceneRenderCommon.hlsli"

struct ConstantData
{
    float4x4 CameraVP;
};

ConstantBuffer<MeshletDrawConstantsData> MeshletDrawConstants : register(b0); // Passed from MeshCull compute shader pass
ConstantBuffer<ConstantData> Constants : register(b1);

[NumThreads(THREADS_PER_MESHLET, 1, 1)]
[OutputTopology("triangle")]
void main(
    uint gtid : SV_GroupThreadID,
    uint gid : SV_GroupID,
    in payload MeshletPayload payload,
    out vertices PipelineVertex verts[63],
    out indices uint3 tris[21]
)
{
    SetMeshOutputCounts(payload.TriangleCount[gtid] * 3, payload.TriangleCount[gtid]);
    
    if (gtid < payload.TriangleCount[gtid])
    {
        uint index = gtid * 3;
        tris[gtid] = uint3(index, index + 1, index + 2);
    }
    
    if (gtid < payload.TriangleCount[gtid] * 3)
    {
        const StructuredBuffer<MeshVertex> vertexPositionBuffer = ResourceDescriptorHeap[MeshletDrawConstants.MeshletBuffer_Bindless + 1]; // The descriptors for the meshlet and vertex buffers are fetched n and n+1
        PipelineVertex newVertex;
        GetVertex(newVertex, payload.VertexOffset[gtid] + gtid, Constants.CameraVP, vertexPositionBuffer, MeshletDrawConstants.WorldTransform, MeshletDrawConstants.MaterialIndex);
        verts[gtid] = newVertex;
    }
}